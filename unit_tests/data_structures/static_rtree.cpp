/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "../../algorithms/coordinate_calculation.hpp"
#include "../../algorithms/geospatial_query.hpp"
#include "../../data_structures/static_rtree.hpp"
#include "../../data_structures/query_node.hpp"
#include "../../data_structures/edge_based_node.hpp"
#include "../../util/floating_point.hpp"
#include "../../typedefs.h"

#include <boost/functional/hash.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include <osrm/coordinate.hpp>

#include <cstdint>
#include <cmath>

#include <algorithm>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <unordered_set>
#include <vector>

BOOST_AUTO_TEST_SUITE(static_rtree)

constexpr uint32_t TEST_BRANCHING_FACTOR = 8;
constexpr uint32_t TEST_LEAF_NODE_SIZE = 64;

typedef EdgeBasedNode TestData;
using TestStaticRTree = StaticRTree<TestData,
                                    std::vector<FixedPointCoordinate>,
                                    false,
                                    TEST_BRANCHING_FACTOR,
                                    TEST_LEAF_NODE_SIZE>;
using MiniStaticRTree = StaticRTree<TestData, std::vector<FixedPointCoordinate>, false, 2, 3>;

// Choosen by a fair W20 dice roll (this value is completely arbitrary)
constexpr unsigned RANDOM_SEED = 42;
static const int32_t WORLD_MIN_LAT = -90 * COORDINATE_PRECISION;
static const int32_t WORLD_MAX_LAT = 90 * COORDINATE_PRECISION;
static const int32_t WORLD_MIN_LON = -180 * COORDINATE_PRECISION;
static const int32_t WORLD_MAX_LON = 180 * COORDINATE_PRECISION;

template <typename DataT> class LinearSearchNN
{
  public:
    LinearSearchNN(const std::shared_ptr<std::vector<FixedPointCoordinate>> &coords,
                   const std::vector<DataT> &edges)
        : coords(coords), edges(edges)
    {
    }

    std::vector<DataT> Nearest(const FixedPointCoordinate &input_coordinate,
                               const unsigned num_results)
    {
        std::vector<DataT> local_edges(edges);

        std::nth_element(
            local_edges.begin(), local_edges.begin() + num_results, local_edges.end(),
            [this, &input_coordinate](const DataT &lhs, const DataT &rhs)
            {
                float current_ratio = 0.;
                FixedPointCoordinate nearest;
                const float lhs_dist = coordinate_calculation::perpendicular_distance(
                    coords->at(lhs.u), coords->at(lhs.v), input_coordinate, nearest, current_ratio);
                const float rhs_dist = coordinate_calculation::perpendicular_distance(
                    coords->at(rhs.u), coords->at(rhs.v), input_coordinate, nearest, current_ratio);
                return lhs_dist < rhs_dist;
            });
        local_edges.resize(num_results);

        return local_edges;
    }

  private:
    const std::shared_ptr<std::vector<FixedPointCoordinate>> &coords;
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

    RandomGraphFixture() : coords(std::make_shared<std::vector<FixedPointCoordinate>>())
    {
        BOOST_TEST_MESSAGE("Constructing " << NUM_NODES << " nodes and " << NUM_EDGES << " edges.");

        std::mt19937 g(RANDOM_SEED);

        std::uniform_int_distribution<> lat_udist(WORLD_MIN_LAT, WORLD_MAX_LAT);
        std::uniform_int_distribution<> lon_udist(WORLD_MIN_LON, WORLD_MAX_LON);

        for (unsigned i = 0; i < NUM_NODES; i++)
        {
            int lat = lat_udist(g);
            int lon = lon_udist(g);
            nodes.emplace_back(QueryNode(lat, lon, OSMNodeID(i)));
            coords->emplace_back(FixedPointCoordinate(lat, lon));
        }

        std::uniform_int_distribution<> edge_udist(0, nodes.size() - 1);

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

    std::vector<QueryNode> nodes;
    std::shared_ptr<std::vector<FixedPointCoordinate>> coords;
    std::vector<TestData> edges;
};

struct GraphFixture
{
    GraphFixture(const std::vector<std::pair<float, float>> &input_coords,
                 const std::vector<std::pair<unsigned, unsigned>> &input_edges)
        : coords(std::make_shared<std::vector<FixedPointCoordinate>>())
    {

        for (unsigned i = 0; i < input_coords.size(); i++)
        {
            FixedPointCoordinate c(input_coords[i].first * COORDINATE_PRECISION,
                                   input_coords[i].second * COORDINATE_PRECISION);
            coords->emplace_back(c);
            nodes.emplace_back(QueryNode(c.lat, c.lon, OSMNodeID(i)));
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
            d.forward_edge_based_node_id = pair.second;
            d.reverse_edge_based_node_id = pair.first;
            edges.emplace_back(d);
        }
    }

    std::vector<QueryNode> nodes;
    std::shared_ptr<std::vector<FixedPointCoordinate>> coords;
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

template <typename RTreeT>
void simple_verify_rtree(RTreeT &rtree,
                         const std::shared_ptr<std::vector<FixedPointCoordinate>> &coords,
                         const std::vector<TestData> &edges)
{
    BOOST_TEST_MESSAGE("Verify end points");
    for (const auto &e : edges)
    {
        const FixedPointCoordinate &pu = coords->at(e.u);
        const FixedPointCoordinate &pv = coords->at(e.v);
        auto result_u = rtree.Nearest(pu, 1);
        auto result_v = rtree.Nearest(pv, 1);
        BOOST_CHECK(result_u.size() == 1 && result_v.size() == 1);
        BOOST_CHECK(result_u.front().u == e.u || result_u.front().v == e.u);
        BOOST_CHECK(result_v.front().u == e.v || result_v.front().v == e.v);
    }
}

template <typename RTreeT>
void sampling_verify_rtree(RTreeT &rtree, LinearSearchNN<TestData> &lsnn, const std::vector<FixedPointCoordinate>& coords, unsigned num_samples)
{
    std::mt19937 g(RANDOM_SEED);
    std::uniform_int_distribution<> lat_udist(WORLD_MIN_LAT, WORLD_MAX_LAT);
    std::uniform_int_distribution<> lon_udist(WORLD_MIN_LON, WORLD_MAX_LON);
    std::vector<FixedPointCoordinate> queries;
    for (unsigned i = 0; i < num_samples; i++)
    {
        queries.emplace_back(FixedPointCoordinate(lat_udist(g), lon_udist(g)));
    }

    BOOST_TEST_MESSAGE("Sampling queries");
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

        float current_ratio = 0.;
        FixedPointCoordinate nearest;
        const float rtree_dist = coordinate_calculation::perpendicular_distance(
            coords[rtree_u], coords[rtree_v], q, nearest, current_ratio);
        const float lsnn_dist = coordinate_calculation::perpendicular_distance(
            coords[lsnn_u], coords[lsnn_v], q, nearest, current_ratio);
        BOOST_CHECK_LE(std::abs(rtree_dist - lsnn_dist), std::numeric_limits<float>::epsilon());
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
    const std::string coords_path = prefix + ".nodes";

    boost::filesystem::ofstream node_stream(coords_path, std::ios::binary);
    const auto num_nodes = static_cast<unsigned>(fixture->nodes.size());
    node_stream.write((char *)&num_nodes, sizeof(unsigned));
    node_stream.write((char *)&(fixture->nodes[0]), num_nodes * sizeof(QueryNode));
    node_stream.close();

    RTreeT r(fixture->edges, nodes_path, leaves_path, fixture->nodes);
}

template <typename FixtureT, typename RTreeT = TestStaticRTree>
void construction_test(const std::string &prefix, FixtureT *fixture)
{
    std::string leaves_path;
    std::string nodes_path;
    build_rtree<FixtureT, RTreeT>(prefix, fixture, leaves_path, nodes_path);
    RTreeT rtree(nodes_path, leaves_path, fixture->coords);
    LinearSearchNN<TestData> lsnn(fixture->coords, fixture->edges);

    simple_verify_rtree(rtree, fixture->coords, fixture->edges);
    sampling_verify_rtree(rtree, lsnn, *fixture->coords, 100);
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
    using Coord = std::pair<float, float>;
    using Edge = std::pair<unsigned, unsigned>;
    GraphFixture fixture(
        {
            Coord(40.0, 0.0), Coord(35.0, 5.0),

            Coord(5.0, 5.0), Coord(0.0, 10.0),

            Coord(20.0, 10.0), Coord(20.0, 5.0),

            Coord(40.0, 100.0), Coord(35.0, 105.0),

            Coord(5.0, 105.0), Coord(0.0, 110.0),
        },
        {Edge(0, 1), Edge(2, 3), Edge(4, 5), Edge(6, 7), Edge(8, 9)});

    std::string leaves_path;
    std::string nodes_path;
    build_rtree<GraphFixture, MiniStaticRTree>("test_regression", &fixture, leaves_path,
                                               nodes_path);
    MiniStaticRTree rtree(nodes_path, leaves_path, fixture.coords);
    LinearSearchNN<TestData> lsnn(fixture.coords, fixture.edges);

    // query a node just right of the center of the gap
    FixedPointCoordinate input(20.0 * COORDINATE_PRECISION, 55.1 * COORDINATE_PRECISION);
    auto result_rtree = rtree.Nearest(input, 1);
    auto result_ls = lsnn.Nearest(input, 1);

    BOOST_CHECK(result_rtree.size() == 1);
    BOOST_CHECK(result_ls.size() == 1);

    BOOST_CHECK_EQUAL(result_ls.front().u, result_rtree.front().u);
    BOOST_CHECK_EQUAL(result_ls.front().v, result_rtree.front().v);
}

void TestRectangle(double width, double height, double center_lat, double center_lon)
{
    FixedPointCoordinate center(center_lat * COORDINATE_PRECISION,
                                center_lon * COORDINATE_PRECISION);

    TestStaticRTree::Rectangle rect;
    rect.min_lat = center.lat - height / 2.0 * COORDINATE_PRECISION;
    rect.max_lat = center.lat + height / 2.0 * COORDINATE_PRECISION;
    rect.min_lon = center.lon - width / 2.0 * COORDINATE_PRECISION;
    rect.max_lon = center.lon + width / 2.0 * COORDINATE_PRECISION;

    unsigned offset = 5 * COORDINATE_PRECISION;
    FixedPointCoordinate north(rect.max_lat + offset, center.lon);
    FixedPointCoordinate south(rect.min_lat - offset, center.lon);
    FixedPointCoordinate west(center.lat, rect.min_lon - offset);
    FixedPointCoordinate east(center.lat, rect.max_lon + offset);
    FixedPointCoordinate north_east(rect.max_lat + offset, rect.max_lon + offset);
    FixedPointCoordinate north_west(rect.max_lat + offset, rect.min_lon - offset);
    FixedPointCoordinate south_east(rect.min_lat - offset, rect.max_lon + offset);
    FixedPointCoordinate south_west(rect.min_lat - offset, rect.min_lon - offset);

    /* Distance to line segments of rectangle */
    BOOST_CHECK_EQUAL(rect.GetMinDist(north),
                      coordinate_calculation::great_circle_distance(
                          north, FixedPointCoordinate(rect.max_lat, north.lon)));
    BOOST_CHECK_EQUAL(rect.GetMinDist(south),
                      coordinate_calculation::great_circle_distance(
                          south, FixedPointCoordinate(rect.min_lat, south.lon)));
    BOOST_CHECK_EQUAL(rect.GetMinDist(west),
                      coordinate_calculation::great_circle_distance(
                          west, FixedPointCoordinate(west.lat, rect.min_lon)));
    BOOST_CHECK_EQUAL(rect.GetMinDist(east),
                      coordinate_calculation::great_circle_distance(
                          east, FixedPointCoordinate(east.lat, rect.max_lon)));

    /* Distance to corner points */
    BOOST_CHECK_EQUAL(rect.GetMinDist(north_east),
                      coordinate_calculation::great_circle_distance(
                          north_east, FixedPointCoordinate(rect.max_lat, rect.max_lon)));
    BOOST_CHECK_EQUAL(rect.GetMinDist(north_west),
                      coordinate_calculation::great_circle_distance(
                          north_west, FixedPointCoordinate(rect.max_lat, rect.min_lon)));
    BOOST_CHECK_EQUAL(rect.GetMinDist(south_east),
                      coordinate_calculation::great_circle_distance(
                          south_east, FixedPointCoordinate(rect.min_lat, rect.max_lon)));
    BOOST_CHECK_EQUAL(rect.GetMinDist(south_west),
                      coordinate_calculation::great_circle_distance(
                          south_west, FixedPointCoordinate(rect.min_lat, rect.min_lon)));
}

BOOST_AUTO_TEST_CASE(rectangle_test)
{
    TestRectangle(10, 10, 5, 5);
    TestRectangle(10, 10, -5, 5);
    TestRectangle(10, 10, 5, -5);
    TestRectangle(10, 10, -5, -5);
    TestRectangle(10, 10, 0, 0);
}

BOOST_AUTO_TEST_CASE(bearing_tests)
{
    using Coord = std::pair<float, float>;
    using Edge = std::pair<unsigned, unsigned>;
    GraphFixture fixture(
        {
            Coord(0.0, 0.0), Coord(10.0, 10.0),
        },
        {Edge(0, 1), Edge(1, 0)});

    std::string leaves_path;
    std::string nodes_path;
    build_rtree<GraphFixture, MiniStaticRTree>("test_bearing", &fixture, leaves_path, nodes_path);
    MiniStaticRTree rtree(nodes_path, leaves_path, fixture.coords);
    GeospatialQuery<MiniStaticRTree> query(rtree, fixture.coords);

    FixedPointCoordinate input(5.0 * COORDINATE_PRECISION, 5.1 * COORDINATE_PRECISION);

    {
        auto results = query.NearestPhantomNodes(input, 5);
        BOOST_CHECK_EQUAL(results.size(), 2);
        BOOST_CHECK_EQUAL(results.back().phantom_node.forward_node_id, 0);
        BOOST_CHECK_EQUAL(results.back().phantom_node.reverse_node_id, 1);
    }

    {
        auto results = query.NearestPhantomNodes(input, 5, 270, 10);
        BOOST_CHECK_EQUAL(results.size(), 0);
    }

    {
        auto results = query.NearestPhantomNodes(input, 5, 45, 10);
        BOOST_CHECK_EQUAL(results.size(), 2);
        BOOST_CHECK_EQUAL(results[0].phantom_node.forward_node_id, 1);
        BOOST_CHECK_EQUAL(results[0].phantom_node.reverse_node_id, SPECIAL_NODEID);
        BOOST_CHECK_EQUAL(results[1].phantom_node.forward_node_id, SPECIAL_NODEID);
        BOOST_CHECK_EQUAL(results[1].phantom_node.reverse_node_id, 1);
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
        BOOST_CHECK_EQUAL(results[0].phantom_node.forward_node_id, 1);
        BOOST_CHECK_EQUAL(results[0].phantom_node.reverse_node_id, SPECIAL_NODEID);
        BOOST_CHECK_EQUAL(results[1].phantom_node.forward_node_id, SPECIAL_NODEID);
        BOOST_CHECK_EQUAL(results[1].phantom_node.reverse_node_id, 1);
    }
}

BOOST_AUTO_TEST_SUITE_END()
