#include "util/static_rtree.hpp"
#include "extractor/edge_based_node.hpp"
#include "engine/geospatial_query.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/exception.hpp"
#include "util/rectangle.hpp"
#include "util/typedefs.hpp"

#include "mocks/mock_datafacade.hpp"

#include <boost/functional/hash.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <cmath>
#include <cstdint>

#include <algorithm>
#include <memory>
#include <random>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <tbb/task_scheduler_init.h>

// explicit TBB scheduler init to register resources cleanup at exit
tbb::task_scheduler_init init(2);

BOOST_AUTO_TEST_SUITE(static_rtree)

using namespace osrm;
using namespace osrm::util;
using namespace osrm::test;

constexpr uint32_t TEST_BRANCHING_FACTOR = 8;
constexpr uint32_t TEST_LEAF_NODE_SIZE = 64;

using TestData = extractor::EdgeBasedNode;
using TestStaticRTree = StaticRTree<TestData,
                                    std::vector<Coordinate>,
                                    false,
                                    TEST_BRANCHING_FACTOR,
                                    TEST_LEAF_NODE_SIZE>;
using MiniStaticRTree = StaticRTree<TestData, std::vector<Coordinate>, false, 2, 128>;

// Choosen by a fair W20 dice roll (this value is completely arbitrary)
constexpr unsigned RANDOM_SEED = 42;
static const int32_t WORLD_MIN_LAT = -85 * COORDINATE_PRECISION;
static const int32_t WORLD_MAX_LAT = 85 * COORDINATE_PRECISION;
static const int32_t WORLD_MIN_LON = -180 * COORDINATE_PRECISION;
static const int32_t WORLD_MAX_LON = 180 * COORDINATE_PRECISION;

template <typename DataT> class LinearSearchNN
{
  public:
    LinearSearchNN(const std::vector<Coordinate> &coords, const std::vector<DataT> &edges)
        : coords(coords), edges(edges)
    {
    }

    std::vector<DataT> Nearest(const Coordinate &input_coordinate, const unsigned num_results)
    {
        std::vector<DataT> local_edges(edges);

        auto projected_input = web_mercator::fromWGS84(input_coordinate);
        const auto segment_comparator = [this, &projected_input](const DataT &lhs,
                                                                 const DataT &rhs) {
            using web_mercator::fromWGS84;
            const auto lhs_result = coordinate_calculation::projectPointOnSegment(
                fromWGS84(coords[lhs.u]), fromWGS84(coords[lhs.v]), projected_input);
            const auto rhs_result = coordinate_calculation::projectPointOnSegment(
                fromWGS84(coords[rhs.u]), fromWGS84(coords[rhs.v]), projected_input);
            const auto lhs_squared_dist = coordinate_calculation::squaredEuclideanDistance(
                lhs_result.second, projected_input);
            const auto rhs_squared_dist = coordinate_calculation::squaredEuclideanDistance(
                rhs_result.second, projected_input);
            return lhs_squared_dist < rhs_squared_dist;
        };

        std::nth_element(local_edges.begin(),
                         local_edges.begin() + num_results,
                         local_edges.end(),
                         segment_comparator);
        local_edges.resize(num_results);

        return local_edges;
    }

  private:
    const std::vector<Coordinate> &coords;
    const std::vector<TestData> &edges;
};

template <unsigned NUM_NODES, unsigned NUM_EDGES> struct RandomGraphFixture
{
    struct TupleHash
    {
        typedef std::pair<unsigned, unsigned> argument_type;
        typedef std::size_t result_type;

        result_type operator()(const argument_type &t) const
        {
            std::size_t val{0};
            boost::hash_combine(val, t.first);
            boost::hash_combine(val, t.second);
            return val;
        }
    };

    RandomGraphFixture()
    {
        std::mt19937 g(RANDOM_SEED);

        std::uniform_int_distribution<> lat_udist(WORLD_MIN_LAT, WORLD_MAX_LAT);
        std::uniform_int_distribution<> lon_udist(WORLD_MIN_LON, WORLD_MAX_LON);

        for (unsigned i = 0; i < NUM_NODES; i++)
        {
            int lon = lon_udist(g);
            int lat = lat_udist(g);
            coords.emplace_back(Coordinate(FixedLongitude{lon}, FixedLatitude{lat}));
        }

        std::uniform_int_distribution<> edge_udist(0, coords.size() - 1);

        std::unordered_set<std::pair<unsigned, unsigned>, TupleHash> used_edges;

        while (edges.size() < NUM_EDGES)
        {
            TestData data;
            data.u = edge_udist(g);
            data.v = edge_udist(g);
            if (used_edges.find(std::pair<unsigned, unsigned>(
                    std::min(data.u, data.v), std::max(data.u, data.v))) == used_edges.end())
            {
                data.component.id = 0;
                edges.emplace_back(data);
                used_edges.emplace(std::min(data.u, data.v), std::max(data.u, data.v));
            }
        }
    }

    std::vector<Coordinate> coords;
    std::vector<TestData> edges;
};

struct GraphFixture
{
    GraphFixture(const std::vector<std::pair<FloatLongitude, FloatLatitude>> &input_coords,
                 const std::vector<std::pair<unsigned, unsigned>> &input_edges)
    {

        for (unsigned i = 0; i < input_coords.size(); i++)
        {
            coords.emplace_back(input_coords[i].first, input_coords[i].second);
        }

        for (const auto &pair : input_edges)
        {
            TestData d;
            d.u = pair.first;
            d.v = pair.second;
            // We set the forward nodes to the target node-based-node IDs, just
            // so we have something to test against.  Because this isn't a real
            // graph, the actual values aren't important, we just need something
            // to examine during tests.
            d.forward_segment_id = {pair.second, true};
            d.reverse_segment_id = {pair.first, true};
            d.fwd_segment_position = 0;
            d.packed_geometry_id = 0;
            edges.emplace_back(d);
        }
    }

    std::vector<Coordinate> coords;
    std::vector<TestData> edges;
};

typedef RandomGraphFixture<TEST_LEAF_NODE_SIZE * 3, TEST_LEAF_NODE_SIZE / 2>
    TestRandomGraphFixture_LeafHalfFull;
typedef RandomGraphFixture<TEST_LEAF_NODE_SIZE * 5, TEST_LEAF_NODE_SIZE>
    TestRandomGraphFixture_LeafFull;
typedef RandomGraphFixture<TEST_LEAF_NODE_SIZE * 10, TEST_LEAF_NODE_SIZE * 2>
    TestRandomGraphFixture_TwoLeaves;
typedef RandomGraphFixture<TEST_LEAF_NODE_SIZE * TEST_BRANCHING_FACTOR * 3,
                           TEST_LEAF_NODE_SIZE * TEST_BRANCHING_FACTOR>
    TestRandomGraphFixture_Branch;
typedef RandomGraphFixture<TEST_LEAF_NODE_SIZE * TEST_BRANCHING_FACTOR * 3,
                           TEST_LEAF_NODE_SIZE * TEST_BRANCHING_FACTOR * 2>
    TestRandomGraphFixture_MultipleLevels;
typedef RandomGraphFixture<10, 30> TestRandomGraphFixture_10_30;

template <typename RTreeT>
void simple_verify_rtree(RTreeT &rtree,
                         const std::vector<Coordinate> &coords,
                         const std::vector<TestData> &edges)
{
    for (const auto &e : edges)
    {
        const Coordinate &pu = coords[e.u];
        const Coordinate &pv = coords[e.v];
        auto result_u = rtree.Nearest(pu, 1);
        auto result_v = rtree.Nearest(pv, 1);
        BOOST_CHECK(result_u.size() == 1 && result_v.size() == 1);
        BOOST_CHECK(result_u.front().u == e.u || result_u.front().v == e.u);
        BOOST_CHECK(result_v.front().u == e.v || result_v.front().v == e.v);
    }
}

template <typename RTreeT>
void sampling_verify_rtree(RTreeT &rtree,
                           LinearSearchNN<TestData> &lsnn,
                           const std::vector<Coordinate> &coords,
                           unsigned num_samples)
{
    std::mt19937 g(RANDOM_SEED);
    std::uniform_int_distribution<> lat_udist(WORLD_MIN_LAT, WORLD_MAX_LAT);
    std::uniform_int_distribution<> lon_udist(WORLD_MIN_LON, WORLD_MAX_LON);
    std::vector<Coordinate> queries;
    for (unsigned i = 0; i < num_samples; i++)
    {
        queries.emplace_back(FixedLongitude{lon_udist(g)}, FixedLatitude{lat_udist(g)});
    }

    for (const auto &q : queries)
    {
        auto result_rtree = rtree.Nearest(q, 1);
        auto result_lsnn = lsnn.Nearest(q, 1);
        BOOST_CHECK(result_rtree.size() == 1);
        BOOST_CHECK(result_lsnn.size() == 1);
        auto rtree_u = result_rtree.back().u;
        auto rtree_v = result_rtree.back().v;
        auto lsnn_u = result_lsnn.back().u;
        auto lsnn_v = result_lsnn.back().v;

        const double rtree_dist =
            coordinate_calculation::perpendicularDistance(coords[rtree_u], coords[rtree_v], q);
        const double lsnn_dist =
            coordinate_calculation::perpendicularDistance(coords[lsnn_u], coords[lsnn_v], q);

        BOOST_CHECK_CLOSE(rtree_dist, lsnn_dist, 0.0001);
    }
}

template <typename FixtureT, typename RTreeT = TestStaticRTree>
void build_rtree(const std::string &prefix,
                 FixtureT *fixture,
                 std::string &leaves_path,
                 std::string &nodes_path)
{
    nodes_path = prefix + ".ramIndex";
    leaves_path = prefix + ".fileIndex";

    RTreeT r(fixture->edges, nodes_path, leaves_path, fixture->coords);
}

template <typename RTreeT = TestStaticRTree, typename FixtureT>
void construction_test(const std::string &prefix, FixtureT *fixture)
{
    std::string leaves_path;
    std::string nodes_path;
    build_rtree<FixtureT, RTreeT>(prefix, fixture, leaves_path, nodes_path);
    RTreeT rtree(nodes_path, leaves_path, fixture->coords);
    LinearSearchNN<TestData> lsnn(fixture->coords, fixture->edges);

    simple_verify_rtree(rtree, fixture->coords, fixture->edges);
    sampling_verify_rtree(rtree, lsnn, fixture->coords, 100);
}

BOOST_FIXTURE_TEST_CASE(construct_tiny, TestRandomGraphFixture_10_30)
{
    using TinyTestTree = StaticRTree<TestData, std::vector<Coordinate>, false, 2, 64>;
    construction_test<TinyTestTree>("test_tiny", this);
}

BOOST_FIXTURE_TEST_CASE(construct_half_leaf_test, TestRandomGraphFixture_LeafHalfFull)
{
    construction_test("test_1", this);
}

BOOST_FIXTURE_TEST_CASE(construct_full_leaf_test, TestRandomGraphFixture_LeafFull)
{
    construction_test("test_2", this);
}

BOOST_FIXTURE_TEST_CASE(construct_two_leaves_test, TestRandomGraphFixture_TwoLeaves)
{
    construction_test("test_3", this);
}

BOOST_FIXTURE_TEST_CASE(construct_branch_test, TestRandomGraphFixture_Branch)
{
    construction_test("test_4", this);
}

BOOST_FIXTURE_TEST_CASE(construct_multiple_levels_test, TestRandomGraphFixture_MultipleLevels)
{
    construction_test("test_5", this);
}

// Bug: If you querry a point that lies between two BBs that have a gap,
// one BB will be pruned, even if it could contain a nearer match.
BOOST_AUTO_TEST_CASE(regression_test)
{
    using Coord = std::pair<FloatLongitude, FloatLatitude>;
    using Edge = std::pair<unsigned, unsigned>;
    GraphFixture fixture(
        {
            Coord{FloatLongitude{0.0}, FloatLatitude{40.0}},   //
            Coord{FloatLongitude{5.0}, FloatLatitude{35.0}},   //
            Coord{FloatLongitude{5.0}, FloatLatitude{5.0}},    //
            Coord{FloatLongitude{10.0}, FloatLatitude{0.0}},   //
            Coord{FloatLongitude{10.0}, FloatLatitude{20.0}},  //
            Coord{FloatLongitude{5.0}, FloatLatitude{20.0}},   //
            Coord{FloatLongitude{100.0}, FloatLatitude{40.0}}, //
            Coord{FloatLongitude{105.0}, FloatLatitude{35.0}}, //
            Coord{FloatLongitude{105.0}, FloatLatitude{5.0}},  //
            Coord{FloatLongitude{110.0}, FloatLatitude{0.0}},  //
        },
        {Edge(0, 1), Edge(2, 3), Edge(4, 5), Edge(6, 7), Edge(8, 9)});

    std::string leaves_path;
    std::string nodes_path;
    build_rtree<GraphFixture, MiniStaticRTree>(
        "test_regression", &fixture, leaves_path, nodes_path);
    MiniStaticRTree rtree(nodes_path, leaves_path, fixture.coords);
    LinearSearchNN<TestData> lsnn(fixture.coords, fixture.edges);

    // query a node just right of the center of the gap
    Coordinate input(FloatLongitude{55.1}, FloatLatitude{20.0});
    auto result_rtree = rtree.Nearest(input, 1);
    auto result_ls = lsnn.Nearest(input, 1);

    BOOST_CHECK(result_rtree.size() == 1);
    BOOST_CHECK(result_ls.size() == 1);

    BOOST_CHECK_EQUAL(result_ls.front().u, result_rtree.front().u);
    BOOST_CHECK_EQUAL(result_ls.front().v, result_rtree.front().v);
}

// Bug: If you querry a point with a narrow radius, no result should be returned
BOOST_AUTO_TEST_CASE(radius_regression_test)
{
    using Coord = std::pair<FloatLongitude, FloatLatitude>;
    using Edge = std::pair<unsigned, unsigned>;
    GraphFixture fixture(
        {
            Coord(FloatLongitude{0.0}, FloatLatitude{0.0}),
            Coord(FloatLongitude{10.0}, FloatLatitude{10.0}),
        },
        {Edge(0, 1), Edge(1, 0)});

    std::string leaves_path;
    std::string nodes_path;
    build_rtree<GraphFixture, MiniStaticRTree>("test_angle", &fixture, leaves_path, nodes_path);
    MiniStaticRTree rtree(nodes_path, leaves_path, fixture.coords);
    MockDataFacade mockfacade;
    engine::GeospatialQuery<MiniStaticRTree, MockDataFacade> query(
        rtree, fixture.coords, mockfacade);

    Coordinate input(FloatLongitude{5.2}, FloatLatitude{5.0});

    {
        auto results = query.NearestPhantomNodesInRange(input, 0.01);
        BOOST_CHECK_EQUAL(results.size(), 0);
    }
}

BOOST_AUTO_TEST_CASE(bearing_tests)
{
    using Coord = std::pair<FloatLongitude, FloatLatitude>;
    using Edge = std::pair<unsigned, unsigned>;
    GraphFixture fixture(
        {
            Coord(FloatLongitude{0.0}, FloatLatitude{0.0}),
            Coord(FloatLongitude{10.0}, FloatLatitude{10.0}),
        },
        {Edge(0, 1), Edge(1, 0)});

    std::string leaves_path;
    std::string nodes_path;
    build_rtree<GraphFixture, MiniStaticRTree>("test_bearing", &fixture, leaves_path, nodes_path);
    MiniStaticRTree rtree(nodes_path, leaves_path, fixture.coords);
    MockDataFacade mockfacade;
    engine::GeospatialQuery<MiniStaticRTree, MockDataFacade> query(
        rtree, fixture.coords, mockfacade);

    Coordinate input(FloatLongitude{5.1}, FloatLatitude{5.0});

    {
        auto results = query.NearestPhantomNodes(input, 5);
        BOOST_CHECK_EQUAL(results.size(), 2);
        BOOST_CHECK_EQUAL(results.back().phantom_node.forward_segment_id.id, 0);
        BOOST_CHECK_EQUAL(results.back().phantom_node.reverse_segment_id.id, 1);
    }

    {
        auto results = query.NearestPhantomNodes(input, 5, 270, 10);
        BOOST_CHECK_EQUAL(results.size(), 0);
    }

    {
        auto results = query.NearestPhantomNodes(input, 5, 45, 10);
        BOOST_CHECK_EQUAL(results.size(), 2);

        BOOST_CHECK(results[0].phantom_node.forward_segment_id.enabled);
        BOOST_CHECK(!results[0].phantom_node.reverse_segment_id.enabled);
        BOOST_CHECK_EQUAL(results[0].phantom_node.forward_segment_id.id, 1);

        BOOST_CHECK(!results[1].phantom_node.forward_segment_id.enabled);
        BOOST_CHECK(results[1].phantom_node.reverse_segment_id.enabled);
        BOOST_CHECK_EQUAL(results[1].phantom_node.reverse_segment_id.id, 1);
    }

    {
        auto results = query.NearestPhantomNodesInRange(input, 11000);
        BOOST_CHECK_EQUAL(results.size(), 2);
    }

    {
        auto results = query.NearestPhantomNodesInRange(input, 11000, 270, 10);
        BOOST_CHECK_EQUAL(results.size(), 0);
    }

    {
        auto results = query.NearestPhantomNodesInRange(input, 11000, 45, 10);
        BOOST_CHECK_EQUAL(results.size(), 2);

        BOOST_CHECK(results[0].phantom_node.forward_segment_id.enabled);
        BOOST_CHECK(!results[0].phantom_node.reverse_segment_id.enabled);
        BOOST_CHECK_EQUAL(results[0].phantom_node.forward_segment_id.id, 1);

        BOOST_CHECK(!results[1].phantom_node.forward_segment_id.enabled);
        BOOST_CHECK(results[1].phantom_node.reverse_segment_id.enabled);
        BOOST_CHECK_EQUAL(results[1].phantom_node.reverse_segment_id.id, 1);
    }
}

BOOST_AUTO_TEST_CASE(bbox_search_tests)
{
    using Coord = std::pair<FloatLongitude, FloatLatitude>;
    using Edge = std::pair<unsigned, unsigned>;

    GraphFixture fixture(
        {
            Coord(FloatLongitude{0.0}, FloatLatitude{0.0}),
            Coord(FloatLongitude{1.0}, FloatLatitude{1.0}),
            Coord(FloatLongitude{2.0}, FloatLatitude{2.0}),
            Coord(FloatLongitude{3.0}, FloatLatitude{3.0}),
            Coord(FloatLongitude{4.0}, FloatLatitude{4.0}),
        },
        {Edge(0, 1), Edge(1, 2), Edge(2, 3), Edge(3, 4)});

    std::string leaves_path;
    std::string nodes_path;
    build_rtree<GraphFixture, MiniStaticRTree>("test_bbox", &fixture, leaves_path, nodes_path);
    MiniStaticRTree rtree(nodes_path, leaves_path, fixture.coords);
    MockDataFacade mockfacade;
    engine::GeospatialQuery<MiniStaticRTree, MockDataFacade> query(
        rtree, fixture.coords, mockfacade);

    {
        RectangleInt2D bbox = {
            FloatLongitude{0.5}, FloatLongitude{1.5}, FloatLatitude{0.5}, FloatLatitude{1.5}};
        auto results = query.Search(bbox);
        BOOST_CHECK_EQUAL(results.size(), 2);
    }

    {
        RectangleInt2D bbox = {
            FloatLongitude{1.5}, FloatLongitude{3.5}, FloatLatitude{1.5}, FloatLatitude{3.5}};
        auto results = query.Search(bbox);
        BOOST_CHECK_EQUAL(results.size(), 3);
    }
}

BOOST_AUTO_TEST_SUITE_END()
