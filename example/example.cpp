#include "osrm/match_parameters.hpp"
#include "osrm/nearest_parameters.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/table_parameters.hpp"
#include "osrm/trip_parameters.hpp"
#include "engine/guidance/result.hpp"
#include "engine/guidance/route.hpp"
#include "engine/guidance/waypoint.hpp"
#include "engine/guidance/route_leg.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"

#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <utility>

#include <cstdlib>

int main(int argc, const char *argv[])
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
    config.use_shared_memory = false;

    // Routing machine with several services (such as Route, Table, Nearest, Trip, Match)
    const OSRM osrm{config};

    // The following shows how to use the Route service; configure this service
    RouteParameters params;

    // Route in monaco
    params.coordinates.push_back({util::FloatLongitude{7.419758}, util::FloatLatitude{43.731142}});
    params.coordinates.push_back({util::FloatLongitude{7.419505}, util::FloatLatitude{43.736825}});
    params.steps = true;

    // Response is in JSON format
    engine::guidance::Result result;

    // Execute routing request, this does the heavy lifting
    const auto status = osrm.Route(params, result);

    if (status == Status::Ok)
    {
        // Let's just use the first route
        const auto distance = result.routes[0].distance;
        const auto duration = result.routes[0].duration;

        // Warn users if extract does not contain the default Berlin coordinates from above
        if (distance == 0 or duration == 0)
        {
            std::cout << "Note: distance or duration is zero. ";
            std::cout << "You are probably doing a query outside of the OSM extract.\n\n";
        }

        std::cout << "Distance: " << distance << " meter\n";
        std::cout << "Duration: " << duration << " seconds\n";
        return EXIT_SUCCESS;
    }
    else if (status != Status::Ok)
    {
        return EXIT_FAILURE;
    }
}
