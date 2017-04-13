#include "extractor/edge_based_node.hpp"
#include "extractor/external_memory_node.hpp"
#include "extractor/query_node.hpp"
#include "storage/io.hpp"
#include "engine/geospatial_query.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/status.hpp"
#include "util/coordinate.hpp"
#include "util/serialization.hpp"
#include "util/static_rtree.hpp"
#include "util/timing_util.hpp"

#include <iostream>
#include <random>

#include <boost/filesystem/fstream.hpp>
using namespace osrm;

// Choosen by a fair W20 dice roll (this value is completely arbitrary)
constexpr unsigned RANDOM_SEED = 13;

std::vector<util::Coordinate> loadCoordinates(const boost::filesystem::path &nodes_file)
{
    storage::io::FileReader nodes_path_file_reader(nodes_file,
                                                   storage::io::FileReader::VerifyFingerprint);

    std::vector<util::Coordinate> coords;
    storage::serialization::read(nodes_path_file_reader, coords);
    util::Log() << "Node file contains " << coords.size() << " nodes";
    return coords;
}

template <typename QueryT>
void benchmarkQuery(const std::vector<std::pair<util::Coordinate, util::Coordinate>> &queries,
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

void benchmark(OSRM &osrm, const std::vector<util::Coordinate> &coords, const unsigned num_queries)
{
    std::mt19937 mt_rand(RANDOM_SEED);
    std::uniform_int_distribution<> coord_udist(0, coords.size());
    std::vector<std::pair<util::Coordinate, util::Coordinate>> queries;
    for (unsigned i = 0; i < num_queries; i++)
    {
        queries.push_back(
            std::make_pair(coords[coord_udist(mt_rand)], coords[coord_udist(mt_rand)]));
    }

    benchmarkQuery(
        queries, "Route queries", [&osrm](const std::pair<util::Coordinate, util::Coordinate> &q) {
            RouteParameters params;
            params.overview = RouteParameters::OverviewType::False;
            params.steps = false;

            params.coordinates.push_back(q.first);
            params.coordinates.push_back(q.second);
            json::Object result;
            const auto rc = osrm.Route(params, result);
            return rc;
        });
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "./route-bench file.osrm" << std::endl;
        return EXIT_FAILURE;
    }
    osrm::util::LogPolicy::GetInstance().Unmute();

    const char *file_path = argv[1];

    using namespace osrm;

    // Configure based on a .osrm base path, and no datasets in shared mem from osrm-datastore
    EngineConfig config;
    config.storage_config = {file_path};
    config.use_shared_memory = false;

    // Routing machine with several services (such as Route, Table, Nearest, Trip, Match)
    OSRM osrm{config};

    auto coords = loadCoordinates(std::string(file_path) + ".nodes");

    benchmark(osrm, coords, 10000);

    return 0;
}
