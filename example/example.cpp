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
        std::cerr << "Error: Not enough arguments." << std::endl
                  << "Run " << argv[0] << " data.osrm" << std::endl;
        return EXIT_FAILURE;
    }

    osrm::EngineConfig engine_config;
    std::string base_path(argv[1]);
    engine_config.server_paths["ramindex"] = base_path + ".ramIndex";
    engine_config.server_paths["fileindex"] = base_path + ".fileIndex";
    engine_config.server_paths["hsgrdata"] = base_path + ".hsgr";
    engine_config.server_paths["nodesdata"] = base_path + ".nodes";
    engine_config.server_paths["edgesdata"] = base_path + ".edges";
    engine_config.server_paths["coredata"] = base_path + ".core";
    engine_config.server_paths["geometries"] = base_path + ".geometry";
    engine_config.server_paths["timestamp"] = base_path + ".timestamp";
    engine_config.server_paths["namesdata"] = base_path + ".names";
    engine_config.use_shared_memory = false;

    osrm::OSRM routing_machine(engine_config);

    osrm::RouteParameters route_parameters;
    // route is in Berlin
    route_parameters.service = "viaroute";
    route_parameters.AddCoordinate(52.519930, 13.438640);
    route_parameters.AddCoordinate(52.513191, 13.415852);

    osrm::json::Object json_result;
    const int result_code = routing_machine.RunQuery(route_parameters, json_result);
    std::cout << "result code: " << result_code << std::endl;
    // 2xx code
    if (result_code / 100 == 2)
    {
        // Extract data out of JSON structure
        auto &summary = json_result.values["route_summary"].get<osrm::json::Object>();
        auto duration = summary.values["total_time"].get<osrm::json::Number>().value;
        auto distance = summary.values["total_distance"].get<osrm::json::Number>().value;
        std::cout << "duration: " << duration << std::endl;
        std::cout << "distance: " << distance << std::endl;
    }
    return EXIT_SUCCESS;
}
catch (const std::exception &current_exception)
{
    std::cout << "exception: " << current_exception.what();
    return EXIT_FAILURE;
}
