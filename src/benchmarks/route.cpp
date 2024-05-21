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

    auto run_benchmark = [&](const std::vector<util::Coordinate> &coordinates)
    {
        RouteParameters params;
        params.overview = RouteParameters::OverviewType::Full;
        params.steps = true;
        params.coordinates = coordinates;

        TIMER_START(routes);
        auto NUM = 1000;
        for (int i = 0; i < NUM; ++i)
        {
            engine::api::ResultT result = json::Object();
            const auto rc = osrm.Route(params, result);
            auto &json_result = result.get<json::Object>();
            if (rc != Status::Ok || json_result.values.find("routes") == json_result.values.end())
            {
                throw std::runtime_error{"Couldn't route"};
            }
        }
        TIMER_STOP(routes);
        std::cout << NUM << " routes with " << coordinates.size() << " coordinates: " << std::endl;
        std::cout << TIMER_MSEC(routes) << "ms" << std::endl;
        std::cout << TIMER_MSEC(routes) / NUM << "ms/req" << std::endl;
    };

    std::vector<std::vector<util::Coordinate>> routes = {
        {{FloatLongitude{7.437602352715465}, FloatLatitude{43.75030522209604}},
         {FloatLongitude{7.421844922513342}, FloatLatitude{43.73690777888953}},
         {FloatLongitude{7.412303912230966}, FloatLatitude{43.72851046529198}}},
        {{FloatLongitude{7.437602352715465}, FloatLatitude{43.75030522209604}},
         {FloatLongitude{7.412303912230966}, FloatLatitude{43.72851046529198}}}};

    for (const auto &route : routes)
    {
        run_benchmark(route);
    }

    return EXIT_SUCCESS;
}
catch (const std::exception &e)
{
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
