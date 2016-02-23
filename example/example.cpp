#include "osrm/route_parameters.hpp"
#include "osrm/table_parameters.hpp"
#include "osrm/nearest_parameters.hpp"
#include "osrm/trip_parameters.hpp"
#include "osrm/match_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"

#include "osrm/status.hpp"
#include "osrm/osrm.hpp"

#include <string>
#include <utility>
#include <iostream>
#include <exception>

#include <cstdlib>

int main(int argc, const char *argv[]) try
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " data.osrm\n";
        return EXIT_FAILURE;
    }

    // Path to .osrm base file
    std::string base{argv[1]};

    // Configure routing machine
    osrm::EngineConfig config;

    // TODO(daniel-j-h): this is ugly, provide easier way for users
    config.server_paths["ramindex"] = base + ".ramIndex";
    config.server_paths["fileindex"] = base + ".fileIndex";
    config.server_paths["hsgrdata"] = base + ".hsgr";
    config.server_paths["nodesdata"] = base + ".nodes";
    config.server_paths["edgesdata"] = base + ".edges";
    config.server_paths["coredata"] = base + ".core";
    config.server_paths["geometries"] = base + ".geometry";
    config.server_paths["timestamp"] = base + ".timestamp";
    config.server_paths["namesdata"] = base + ".names";
    config.use_shared_memory = false;

    // Routing machine with several services (such as Route, Table, Nearest, Trip, Match)
    osrm::OSRM osrm{config};

    // The following shows how to use the Route service; configure this service
    osrm::RouteParameters params;

    // Route is in Berlin. Latitude, Longitude
    // TODO(daniel-j-h): use either coordinate_precision or better provide double,double-taking ctor
    params.coordinates.push_back({osrm::util::FloatLongitude(13.438640), osrm::util::FloatLatitude(52.519930)});
    params.coordinates.push_back({osrm::util::FloatLongitude(13.415852), osrm::util::FloatLatitude(52.513191)});

    // Response is in JSON format
    osrm::json::Object result;

    // Execute routing request, this does the heavy lifting
    const auto status = osrm.Route(params, result);

    if (status == osrm::Status::Ok)
    {
        auto &routes = result.values["routes"].get<osrm::json::Array>();

        // Let's just use the first route
        auto &route = routes.values.at(0).get<osrm::json::Object>();
        const auto distance = route.values["distance"].get<osrm::json::Number>().value;
        const auto duration = route.values["duration"].get<osrm::json::Number>().value;

        // Warn users if extract does not contain the default Berlin coordinates from above
        if (distance == 0 or duration == 0)
        {
            std::cout << "Note: distance or duration is zero. ";
            std::cout << "You are probably doing a query outside of the OSM extract.\n\n";
        }

        std::cout << "Distance: " << distance << " meter\n";
        std::cout << "Duration: " << duration << " seconds\n";
    }
    else if (status == osrm::Status::Error)
    {
        const auto code = result.values["code"].get<osrm::json::String>().value;
        const auto message = result.values["message"].get<osrm::json::String>().value;

        std::cout << "Code: " << code << "\n";
        std::cout << "Message: " << code << "\n";
        return EXIT_FAILURE;
    }
}
catch (const std::exception &e)
{
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
