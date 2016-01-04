#include "util/version.hpp"
#include "util/json_renderer.hpp"
#include "util/routed_options.hpp"
#include "util/simple_logger.hpp"

#include "osrm/json_container.hpp"
#include "osrm/libosrm_config.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/osrm.hpp"

#include <string>

int main(int argc, const char *argv[])
{
    LogPolicy::GetInstance().Unmute();
    try
    {
        std::string ip_address;
        int ip_port, requested_thread_num;
        bool trial_run = false;
        LibOSRMConfig lib_config;
        const unsigned init_result = GenerateServerProgramOptions(
            argc, argv, lib_config.server_paths, ip_address, ip_port, requested_thread_num,
            lib_config.use_shared_memory, trial_run, lib_config.max_locations_trip,
            lib_config.max_locations_viaroute, lib_config.max_locations_distance_table,
            lib_config.max_locations_map_matching);

        if (init_result == INIT_OK_DO_NOT_START_ENGINE)
        {
            return 0;
        }
        if (init_result == INIT_FAILED)
        {
            return 1;
        }
        SimpleLogger().Write() << "starting up engines, " << OSRM_VERSION;

        OSRM routing_machine(lib_config);

        RouteParameters route_parameters;
        route_parameters.zoom_level = 18;           // no generalization
        route_parameters.print_instructions = true; // turn by turn instructions
        route_parameters.alternate_route = true;    // get an alternate route, too
        route_parameters.geometry = true;           // retrieve geometry of route
        route_parameters.compression = true;        // polyline encoding
        route_parameters.check_sum = -1;            // see wiki
        route_parameters.service = "viaroute";      // that's routing
        route_parameters.output_format = "json";
        route_parameters.jsonp_parameter = ""; // set for jsonp wrapping
        route_parameters.language = "";        // unused atm
        // route_parameters.hints.push_back(); // see wiki, saves I/O if done properly

        // start_coordinate
        route_parameters.coordinates.emplace_back(52.519930 * COORDINATE_PRECISION,
                                                  13.438640 * COORDINATE_PRECISION);
        // target_coordinate
        route_parameters.coordinates.emplace_back(52.513191 * COORDINATE_PRECISION,
                                                  13.415852 * COORDINATE_PRECISION);
        osrm::json::Object json_result;
        const int result_code = routing_machine.RunQuery(route_parameters, json_result);
        SimpleLogger().Write() << "http code: " << result_code;
        osrm::json::render(SimpleLogger().Write(), json_result);
    }
    catch (std::exception &current_exception)
    {
        SimpleLogger().Write(logWARNING) << "caught exception: " << current_exception.what();
        return -1;
    }
    return 0;
}
