#include "engine/engine_config.hpp"
#include "util/coordinate.hpp"
#include "util/timing_util.hpp"

#include "osrm/route_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"

#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

#include <boost/assert.hpp>

#include <boost/optional/optional.hpp>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, const char *argv[])
try
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " data.osrm\n";
        return EXIT_FAILURE;
    }

    using namespace osrm;

    // Configure based on a .osrm base path, and no datasets in shared mem from osrm-datastore
    EngineConfig config;
    config.storage_config = {argv[1]};
    config.algorithm = (argc > 2 && std::string{argv[2]} == "mld") ? EngineConfig::Algorithm::MLD
                                                                   : EngineConfig::Algorithm::CH;
    config.use_shared_memory = false;

    // Routing machine with several services (such as Route, Table, Nearest, Trip, Match)
    OSRM osrm{config};

    struct Benchmark
    {
        std::string name;
        std::vector<util::Coordinate> coordinates;
        RouteParameters::OverviewType overview;
        bool steps = false;
        std::optional<size_t> alternatives = std::nullopt;
        std::optional<double> radius = std::nullopt;
    };

    auto run_benchmark = [&](const Benchmark &benchmark)
    {
        RouteParameters params;
        params.overview = benchmark.overview;
        params.steps = benchmark.steps;
        params.coordinates = benchmark.coordinates;
        if (benchmark.alternatives)
        {
            params.alternatives = *benchmark.alternatives;
        }

        if (benchmark.radius)
        {
            params.radiuses = std::vector<std::optional<double>>(
                params.coordinates.size(), std::make_optional(*benchmark.radius));
        }

        TIMER_START(routes);
        auto NUM = 1000;
        for (int i = 0; i < NUM; ++i)
        {
            engine::api::ResultT result = json::Object();
            const auto rc = osrm.Route(params, result);
            auto &json_result = std::get<json::Object>(result);
            if (rc != Status::Ok || json_result.values.find("routes") == json_result.values.end())
            {
                throw std::runtime_error{"Couldn't route"};
            }
        }
        TIMER_STOP(routes);
        std::cout << benchmark.name << std::endl;
        std::cout << TIMER_MSEC(routes) << "ms" << std::endl;
        std::cout << TIMER_MSEC(routes) / NUM << "ms/req" << std::endl;
    };

    std::vector<Benchmark> benchmarks = {
        {"1000 routes, 3 coordinates, no alternatives, overview=full, steps=true",
         {{FloatLongitude{7.437602352715465}, FloatLatitude{43.75030522209604}},
          {FloatLongitude{7.421844922513342}, FloatLatitude{43.73690777888953}},
          {FloatLongitude{7.412303912230966}, FloatLatitude{43.72851046529198}}},
         RouteParameters::OverviewType::Full,
         true,
         std::nullopt},
        {"1000 routes, 2 coordinates, 3 alternatives, overview=full, steps=true",
         {{FloatLongitude{7.437602352715465}, FloatLatitude{43.75030522209604}},
          {FloatLongitude{7.412303912230966}, FloatLatitude{43.72851046529198}}},
         RouteParameters::OverviewType::Full,
         true,
         3},
        {"1000 routes, 3 coordinates, no alternatives, overview=false, steps=false",
         {{FloatLongitude{7.437602352715465}, FloatLatitude{43.75030522209604}},
          {FloatLongitude{7.421844922513342}, FloatLatitude{43.73690777888953}},
          {FloatLongitude{7.412303912230966}, FloatLatitude{43.72851046529198}}},
         RouteParameters::OverviewType::False,
         false,
         std::nullopt},
        {"1000 routes, 2 coordinates, 3 alternatives, overview=false, steps=false",
         {{FloatLongitude{7.437602352715465}, FloatLatitude{43.75030522209604}},
          {FloatLongitude{7.412303912230966}, FloatLatitude{43.72851046529198}}},
         RouteParameters::OverviewType::False,
         false,
         3},

    };

    for (const auto &benchmark : benchmarks)
    {
        run_benchmark(benchmark);
    }

    return EXIT_SUCCESS;
}
catch (const std::exception &e)
{
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
