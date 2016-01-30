#include "extractor/query_node.hpp"
#include "util/static_rtree.hpp"
#include "extractor/edge_based_node.hpp"
#include "engine/geospatial_query.hpp"
#include "util/timing_util.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "contractor/query_edge.hpp"

#include "osrm/coordinate.hpp"

#include <iostream>
#include <random>

namespace osrm
{
namespace benchmarks
{

template <class EdgeDataT> class MockDataFacadeT final : public osrm::engine::datafacade::BaseDataFacade<EdgeDataT>
{
    private:
        EdgeDataT foo;
    public:
    unsigned GetNumberOfNodes() const { return 0; }
    unsigned GetNumberOfEdges() const { return 0; }
    unsigned GetOutDegree(const NodeID /* n */) const { return 0; }
    NodeID GetTarget(const EdgeID /* e */) const { return SPECIAL_NODEID; }
    const EdgeDataT &GetEdgeData(const EdgeID /* e */) const { 
        return foo;
    }
    EdgeID BeginEdges(const NodeID /* n */) const { return SPECIAL_EDGEID; }
    EdgeID EndEdges(const NodeID /* n */) const { return SPECIAL_EDGEID; }
    osrm::engine::datafacade::EdgeRange GetAdjacentEdgeRange(const NodeID /* node */) const {
        return util::irange(static_cast<EdgeID>(0),static_cast<EdgeID>(0));
    }
    EdgeID FindEdge(const NodeID /* from */, const NodeID /* to */) const { return SPECIAL_EDGEID; }
    EdgeID FindEdgeInEitherDirection(const NodeID /* from */, const NodeID /* to */) const { return SPECIAL_EDGEID; }
    EdgeID
    FindEdgeIndicateIfReverse(const NodeID /* from */, const NodeID /* to */, bool & /* result */) const { return SPECIAL_EDGEID; }
    util::FixedPointCoordinate GetCoordinateOfNode(const unsigned /* id */) const {
        FixedPointCoordinate foo(0,0);
        return foo;
    }
    bool EdgeIsCompressed(const unsigned /* id */) const { return false; }
    unsigned GetGeometryIndexForEdgeID(const unsigned /* id */) const { return SPECIAL_NODEID; }
    void GetUncompressedGeometry(const EdgeID /* id */,
                                         std::vector<NodeID> &/* result_nodes */) const {}
    void GetUncompressedWeights(const EdgeID /* id */,
                                         std::vector<EdgeWeight> & /* result_weights */) const {}
    extractor::TurnInstruction GetTurnInstructionForEdgeID(const unsigned /* id */) const {
        return osrm::extractor::TurnInstruction::NoTurn;
    }
    extractor::TravelMode GetTravelModeForEdgeID(const unsigned /* id */) const 
    {
        return TRAVEL_MODE_DEFAULT;
    }
    std::vector<typename osrm::engine::datafacade::BaseDataFacade<EdgeDataT>::RTreeLeaf> GetEdgesInBox(const util::FixedPointCoordinate & /* south_west */,
                                                 const util::FixedPointCoordinate & /*north_east */) {
        std::vector<typename osrm::engine::datafacade::BaseDataFacade<EdgeDataT>::RTreeLeaf> foo;
        return foo;
    }
    std::vector<osrm::engine::PhantomNodeWithDistance>
    NearestPhantomNodesInRange(const util::FixedPointCoordinate /* input_coordinate */,
                               const float /* max_distance */,
                               const int /* bearing = 0 */,
                               const int /* bearing_range = 180 */) {
        std::vector<osrm::engine::PhantomNodeWithDistance> foo;
        return foo;
    }
    std::vector<osrm::engine::PhantomNodeWithDistance>
    NearestPhantomNodes(const util::FixedPointCoordinate /* input_coordinate */,
                        const unsigned /* max_results */,
                        const int /* bearing = 0 */,
                        const int /* bearing_range = 180 */) {
        std::vector<osrm::engine::PhantomNodeWithDistance> foo;
        return foo;
    }
    std::pair<osrm::engine::PhantomNode, osrm::engine::PhantomNode> NearestPhantomNodeWithAlternativeFromBigComponent(
        const util::FixedPointCoordinate /* input_coordinate */,
        const int /* bearing = 0 */,
        const int /* bearing_range = 180 */) {
        std::pair<osrm::engine::PhantomNode, osrm::engine::PhantomNode> foo;
        return foo;
    }
    unsigned GetCheckSum() const { return 0; }
    bool IsCoreNode(const NodeID /* id */) const { return false; }
    unsigned GetNameIndexFromEdgeID(const unsigned /* id */) const { return 0; }
    std::string get_name_for_id(const unsigned /* name_id */) const { return ""; }
    std::size_t GetCoreSize() const { return 0; }
    std::string GetTimestamp() const { return ""; }

};

using MockDataFacade = MockDataFacadeT<contractor::QueryEdge::EdgeData>;


// Choosen by a fair W20 dice roll (this value is completely arbitrary)
constexpr unsigned RANDOM_SEED = 13;
constexpr int32_t WORLD_MIN_LAT = -90 * COORDINATE_PRECISION;
constexpr int32_t WORLD_MAX_LAT = 90 * COORDINATE_PRECISION;
constexpr int32_t WORLD_MIN_LON = -180 * COORDINATE_PRECISION;
constexpr int32_t WORLD_MAX_LON = 180 * COORDINATE_PRECISION;

using RTreeLeaf = extractor::EdgeBasedNode;
using FixedPointCoordinateListPtr = std::shared_ptr<std::vector<util::FixedPointCoordinate>>;
using BenchStaticRTree =
    util::StaticRTree<RTreeLeaf, util::ShM<util::FixedPointCoordinate, false>::vector, false>;
using BenchQuery = engine::GeospatialQuery<BenchStaticRTree, MockDataFacade>;

FixedPointCoordinateListPtr loadCoordinates(const boost::filesystem::path &nodes_file)
{
    boost::filesystem::ifstream nodes_input_stream(nodes_file, std::ios::binary);

    extractor::QueryNode current_node;
    unsigned coordinate_count = 0;
    nodes_input_stream.read((char *)&coordinate_count, sizeof(unsigned));
    auto coords = std::make_shared<std::vector<FixedPointCoordinate>>(coordinate_count);
    for (unsigned i = 0; i < coordinate_count; ++i)
    {
        nodes_input_stream.read((char *)&current_node, sizeof(extractor::QueryNode));
        coords->at(i) = FixedPointCoordinate(current_node.lat, current_node.lon);
        BOOST_ASSERT((std::abs(coords->at(i).lat) >> 30) == 0);
        BOOST_ASSERT((std::abs(coords->at(i).lon) >> 30) == 0);
    }
    nodes_input_stream.close();
    return coords;
}

template <typename QueryT>
void benchmarkQuery(const std::vector<FixedPointCoordinate> &queries,
                    const std::string &name,
                    QueryT query)
{
    std::cout << "Running " << name << " with " << queries.size() << " coordinates: " << std::flush;

    TIMER_START(query);
    for (const auto &q : queries)
    {
        auto result = query(q);
        (void)result;
    }
    TIMER_STOP(query);

    std::cout << "Took " << TIMER_SEC(query) << " seconds "
              << "(" << TIMER_MSEC(query) << "ms"
              << ")  ->  " << TIMER_MSEC(query) / queries.size() << " ms/query "
              << "(" << TIMER_MSEC(query) << "ms"
              << ")" << std::endl;
}

void benchmark(BenchStaticRTree &rtree, BenchQuery &geo_query, unsigned num_queries)
{
    std::mt19937 mt_rand(RANDOM_SEED);
    std::uniform_int_distribution<> lat_udist(WORLD_MIN_LAT, WORLD_MAX_LAT);
    std::uniform_int_distribution<> lon_udist(WORLD_MIN_LON, WORLD_MAX_LON);
    std::vector<FixedPointCoordinate> queries;
    for (unsigned i = 0; i < num_queries; i++)
    {
        queries.emplace_back(lat_udist(mt_rand), lon_udist(mt_rand));
    }

    benchmarkQuery(queries, "raw RTree queries (1 result)", [&rtree](const FixedPointCoordinate &q)
                   {
                       return rtree.Nearest(q, 1);
                   });
    benchmarkQuery(queries, "raw RTree queries (10 results)",
                   [&rtree](const FixedPointCoordinate &q)
                   {
                       return rtree.Nearest(q, 10);
                   });

    benchmarkQuery(queries, "big component alternative queries",
                   [&geo_query](const FixedPointCoordinate &q)
                   {
                       return geo_query.NearestPhantomNodeWithAlternativeFromBigComponent(q);
                   });
    benchmarkQuery(queries, "max distance 1000", [&geo_query](const FixedPointCoordinate &q)
                   {
                       return geo_query.NearestPhantomNodesInRange(q, 1000);
                   });
    benchmarkQuery(queries, "PhantomNode query (1 result)",
                   [&geo_query](const FixedPointCoordinate &q)
                   {
                       return geo_query.NearestPhantomNodes(q, 1);
                   });
    benchmarkQuery(queries, "PhantomNode query (10 result)",
                   [&geo_query](const FixedPointCoordinate &q)
                   {
                       return geo_query.NearestPhantomNodes(q, 10);
                   });
}
}
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        std::cout << "./rtree-bench file.ramIndex file.fileIndx file.nodes"
                  << "\n";
        return 1;
    }

    const char *ram_path = argv[1];
    const char *file_path = argv[2];
    const char *nodes_path = argv[3];

    auto coords = osrm::benchmarks::loadCoordinates(nodes_path);

    osrm::benchmarks::BenchStaticRTree rtree(ram_path, file_path, coords);
    std::unique_ptr<osrm::benchmarks::MockDataFacade> mockfacade_ptr(new osrm::benchmarks::MockDataFacade);
    osrm::benchmarks::BenchQuery query(rtree, coords, *mockfacade_ptr);

    osrm::benchmarks::benchmark(rtree, query, 10000);

    return 0;
}
