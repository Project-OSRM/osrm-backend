#include "engine/geospatial_query.hpp"
#include "extractor/query_node.hpp"
#include "extractor/edge_based_node.hpp"
#include "util/static_rtree.hpp"
#include "util/timing_util.hpp"
#include "util/coordinate.hpp"
#include "mocks/mock_datafacade.hpp"

#include <iostream>
#include <random>

#include <boost/filesystem/fstream.hpp>

namespace osrm
{
namespace benchmarks
{

using namespace osrm::test;

// Choosen by a fair W20 dice roll (this value is completely arbitrary)
constexpr unsigned RANDOM_SEED = 13;
constexpr int32_t WORLD_MIN_LAT = -90 * COORDINATE_PRECISION;
constexpr int32_t WORLD_MAX_LAT = 90 * COORDINATE_PRECISION;
constexpr int32_t WORLD_MIN_LON = -180 * COORDINATE_PRECISION;
constexpr int32_t WORLD_MAX_LON = 180 * COORDINATE_PRECISION;

using RTreeLeaf = extractor::EdgeBasedNode;
using CoordinateListPtr = std::shared_ptr<std::vector<util::Coordinate>>;
using BenchStaticRTree =
    util::StaticRTree<RTreeLeaf, util::ShM<util::Coordinate, false>::vector, false>;
using BenchQuery = engine::GeospatialQuery<BenchStaticRTree, MockDataFacade>;

CoordinateListPtr loadCoordinates(const boost::filesystem::path &nodes_file)
{
    boost::filesystem::ifstream nodes_input_stream(nodes_file, std::ios::binary);

    extractor::QueryNode current_node;
    unsigned coordinate_count = 0;
    nodes_input_stream.read((char *)&coordinate_count, sizeof(unsigned));
    auto coords = std::make_shared<std::vector<Coordinate>>(coordinate_count);
    for (unsigned i = 0; i < coordinate_count; ++i)
    {
        nodes_input_stream.read((char *)&current_node, sizeof(extractor::QueryNode));
        coords->at(i) = util::Coordinate(current_node.lon, current_node.lat);
        BOOST_ASSERT((std::abs(coords->at(i).lat) >> 30) == 0);
        BOOST_ASSERT((std::abs(coords->at(i).lon) >> 30) == 0);
    }
    return coords;
}

template <typename QueryT>
void benchmarkQuery(const std::vector<util::Coordinate> &queries,
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
    std::vector<util::Coordinate> queries;
    for (unsigned i = 0; i < num_queries; i++)
    {
        queries.emplace_back(util::FixedLongitude{lon_udist(mt_rand)},
                             util::FixedLatitude{lat_udist(mt_rand)});
    }

    benchmarkQuery(queries, "raw RTree queries (1 result)", [&rtree](const util::Coordinate &q)
                   {
                       return rtree.Nearest(q, 1);
                   });
    benchmarkQuery(queries, "raw RTree queries (10 results)", [&rtree](const util::Coordinate &q)
                   {
                       return rtree.Nearest(q, 10);
                   });

    benchmarkQuery(queries, "big component alternative queries",
                   [&geo_query](const util::Coordinate &q)
                   {
                       return geo_query.NearestPhantomNodeWithAlternativeFromBigComponent(q);
                   });
    benchmarkQuery(queries, "max distance 1000", [&geo_query](const util::Coordinate &q)
                   {
                       return geo_query.NearestPhantomNodesInRange(q, 1000);
                   });
    benchmarkQuery(queries, "PhantomNode query (1 result)", [&geo_query](const util::Coordinate &q)
                   {
                       return geo_query.NearestPhantomNodes(q, 1);
                   });
    benchmarkQuery(queries, "PhantomNode query (10 result)", [&geo_query](const util::Coordinate &q)
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
    osrm::test::MockDataFacade mockfacade;
    osrm::benchmarks::BenchQuery query(rtree, coords, mockfacade);

    osrm::benchmarks::benchmark(rtree, query, 10000);

    return 0;
}
