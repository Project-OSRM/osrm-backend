#include "util/timing_util.hpp"

#include "osrm/table_parameters.hpp"

#include "storage/io.hpp"
#include "storage/serialization.hpp"
#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"

#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

#include <boost/assert.hpp>

#include <exception>
#include <iostream>
#include <random>
#include <string>
#include <utility>

#include <cstdlib>

int main(int argc, const char *argv[]) try
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " data.osrm\n";
        return EXIT_FAILURE;
    }

    using namespace osrm;

    std::vector<util::Coordinate> coordinates;
    // Loading list of coordinates
    {
        using namespace osrm::storage;
        io::FileReader nodes_file(std::string(argv[1]) + ".nodes",
                                  io::FileReader::VerifyFingerprint);
        storage::serialization::read(nodes_file, coordinates);
    }

    // Configure based on a .osrm base path, and no datasets in shared mem from osrm-datastore
    EngineConfig config;
    config.storage_config = {argv[1]};
    config.use_shared_memory = false;

    // Routing machine with several services (such as Route, Table, Nearest, Trip, Match)
    OSRM osrm{config};

    auto runtest = [&](const int coordcount) -> void {
        auto r = std::mt19937();

        TIMER_START(routes);
        auto NUM = 10;
        for (int i = 0; i < NUM; ++i)
        {
            TableParameters params;
            // Select a random sample
            for (int i = 0; i < coordcount; i++)
            {
                params.coordinates.push_back(coordinates[r() % coordinates.size()]);
            }
            json::Object result;
            const auto rc = osrm.Table(params, result);
            if (rc != Status::Ok)
            {
                return;
            }
        }
        TIMER_STOP(routes);
        std::cout << coordcount << "," << (TIMER_MSEC(routes) / 1000. / NUM) << std::endl;
    };

    std::cout << "Coordinates,Duration" << std::endl;
    for (int i = 10; i < 3000; i += 10)
    {
        runtest(i);
    }

    return EXIT_SUCCESS;
}
catch (const std::exception &e)
{
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
