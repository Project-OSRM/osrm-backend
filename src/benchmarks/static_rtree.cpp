#include "util/static_rtree.hpp"
#include "extractor/edge_based_node_segment.hpp"
#include "extractor/files.hpp"
#include "extractor/packed_osm_ids.hpp"
#include "extractor/query_node.hpp"

#include "mocks/mock_datafacade.hpp"
#include "storage/io.hpp"
#include "engine/geospatial_query.hpp"
#include "util/coordinate.hpp"
#include "util/serialization.hpp"
#include "util/timing_util.hpp"

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

using RTreeLeaf = extractor::EdgeBasedNodeSegment;
using BenchStaticRTree = util::StaticRTree<RTreeLeaf, storage::Ownership::Container>;

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

void benchmark(BenchStaticRTree &rtree, unsigned num_queries)
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

    benchmarkQuery(queries, "raw RTree queries (1 result)", [&rtree](const util::Coordinate &q) {
        return rtree.Nearest(q, 1);
    });
    benchmarkQuery(queries, "raw RTree queries (10 results)", [&rtree](const util::Coordinate &q) {
        return rtree.Nearest(q, 10);
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

    std::vector<osrm::util::Coordinate> coords;
    osrm::extractor::files::readNodeCoordinates(nodes_path, coords);

    osrm::benchmarks::BenchStaticRTree rtree(file_path, coords);
    osrm::extractor::files::readRamIndex(ram_path, rtree);

    osrm::benchmarks::benchmark(rtree, 10000);

    return 0;
}
