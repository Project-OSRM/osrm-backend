#include "python/engineconfig_nb.hpp"
#include "python/parameters/baseparameter_nb.hpp"
#include "python/parameters/matchparameter_nb.hpp"
#include "python/parameters/nearestparameter_nb.hpp"
#include "python/parameters/routeparameter_nb.hpp"
#include "python/parameters/tableparameter_nb.hpp"
#include "python/parameters/tileparameter_nb.hpp"
#include "python/parameters/tripparameter_nb.hpp"
#include "python/types/approach_nb.hpp"
#include "python/types/bearing_nb.hpp"
#include "python/types/coordinate_nb.hpp"
#include "python/types/jsoncontainer_nb.hpp"
#include "python/types/optional_nb.hpp"
#include "python/utility/osrm_utility.hpp"
#include "engine/api/match_parameters.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/api/tile_parameters.hpp"
#include "engine/api/trip_parameters.hpp"
#include "engine/engine_config.hpp"
#include "engine/status.hpp"
#include "osrm/osrm.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include <stdexcept>

namespace nb = nanobind;

NB_MODULE(osrm_ext, m)
{
    namespace api = osrm::engine::api;
    namespace json = osrm::util::json;

    using osrm::OSRM;
    using osrm::engine::EngineConfig;
    using osrm::engine::api::MatchParameters;
    using osrm::engine::api::NearestParameters;
    using osrm::engine::api::RouteParameters;
    using osrm::engine::api::TableParameters;
    using osrm::engine::api::TileParameters;
    using osrm::engine::api::TripParameters;

    init_EngineConfig(m);

    init_Approach(m);
    init_Bearing(m);
    init_Coordinate(m);
    init_JSONContainer(m);
    init_Optional(m);

    init_BaseParameters(m);
    init_NearestParameters(m);
    init_TableParameters(m);
    init_RouteParameters(m);
    init_MatchParameters(m);
    init_TripParameters(m);
    init_TileParameters(m);

    nb::class_<OSRM>(m, "OSRM", nb::is_final())
        .def(nb::init<EngineConfig &>(),
             "Instantiates an instance of OSRM.\n\n"
             "Examples:\n\
                >>> import osrm\n\
                >>> osrm_py = osrm.OSRM('.tests/test_data/ch/monaco.osrm')\n\
                >>> osrm_py = osrm.OSRM(\n\
                        algorithm = 'CH',\n\
                        storage_config = '.tests/test_data/ch/monaco.osrm',\n\
                        max_locations_trip = 3,\n\
                        max_locations_viaroute = 3,\n\
                        max_locations_distance_table = 3,\n\
                        max_locations_map_matching = 3,\n\
                        max_results_nearest = 1,\n\
                        max_alternatives = 1,\n\
                        default_radius = 'unlimited'\n\
                    )\n\n"
             "Args:\n\
                storage_config (string): File path string to storage config.\n\
                EngineConfig (osrm.osrm_ext.EngineConfig): Keyword arguments from the EngineConfig class.\n\n"
             "Returns:\n\
                __init__ (osrm.OSRM): A OSRM object.\n\n"
             "Raises:\n\
                RuntimeError: On invalid OSRM EngineConfig parameters.")
        .def("__init__",
             [](OSRM *t, const std::string &storage_path)
             {
                 EngineConfig config;
                 config.storage_config = osrm::storage::StorageConfig(storage_path);
                 config.use_shared_memory = false;

                 if (!config.IsValid())
                 {
                     throw std::runtime_error("Required files are missing");
                 }

                 new (t) OSRM(config);
             })
        .def("__init__",
             [](OSRM *t, const nb::kwargs &kwargs)
             {
                 EngineConfig config;
                 osrm_nb_util::populate_cfg_from_kwargs(kwargs, config);

                 if (!config.IsValid())
                 {
                     throw std::runtime_error("Config Parameters are Invalid");
                 }

                 new (t) OSRM(config);
             })
        .def(
            "Match",
            [](OSRM *t, const MatchParameters &params)
            {
                if (!params.IsValid())
                {
                    throw std::runtime_error("Invalid Match Parameters");
                }

                json::Object result;
                osrm::engine::Status status = t->Match(params, result);
                osrm_nb_util::check_status(status, result);

                return result;
            },
            "Matches/snaps given GPS points to the road network in the most plausible way.\n\n"
            "Examples:\n\
                >>> res = osrm_py.Match(match_params)\n\n"
            "Args:\n\
                match_params (osrm.MatchParameters): MatchParameters Object.\n\n"
            "Returns:\n\
                (json): [A Match JSON Response](https://project-osrm.org/docs/v5.24.0/api/#match-service).\n\n"
            "Raises:\n\
                RuntimeError: On invalid MatchParameters.")
        .def(
            "Nearest",
            [](OSRM *t, const NearestParameters &params)
            {
                if (!params.IsValid())
                {
                    throw std::runtime_error("Invalid Nearest Parameters");
                }

                json::Object result;
                osrm::engine::Status status = t->Nearest(params, result);
                osrm_nb_util::check_status(status, result);

                return result;
            },
            "Snaps a coordinate to the street network and returns the nearest matches.\n\n"
            "Examples:\n\
                >>> res = osrm_py.Nearest(nearest_params)\n\n"
            "Args:\n\
                nearest_params (osrm.NearestParameters): NearestParameters Object.\n\n"
            "Returns:\n\
                (json): [A Nearest JSON Response](https://project-osrm.org/docs/v5.24.0/api/#nearest-service).\n\n"
            "Raises:\n\
                RuntimeError: On invalid NearestParameters.")
        .def(
            "Route",
            [](OSRM *t, const RouteParameters &params)
            {
                if (!params.IsValid())
                {
                    throw std::runtime_error("Invalid Route Parameters");
                }

                json::Object result;
                osrm::engine::Status status = t->Route(params, result);
                osrm_nb_util::check_status(status, result);

                return result;
            },
            "Finds the fastest route between coordinates in the supplied order.\n\n"
            "Examples:\n\
                >>> res = osrm_py.Route(route_params)\n\n"
            "Args:\n\
                route_params (osrm.RouteParameters): RouteParameters Object.\n\n"
            "Returns:\n\
                (json): [A Route JSON Response](https://project-osrm.org/docs/v5.24.0/api/#route-service).\n\n"
            "Raises:\n\
                RuntimeError: On invalid RouteParameters.")
        .def(
            "Table",
            [](OSRM *t, const TableParameters &params)
            {
                if (!params.IsValid())
                {
                    throw std::runtime_error("Invalid Table Parameters");
                }

                json::Object result;
                osrm::engine::Status status = t->Table(params, result);
                osrm_nb_util::check_status(status, result);

                return result;
            },
            "Computes the duration of the fastest route between all pairs of supplied "
            "coordinates.\n\n"
            "Examples:\n\
                >>> res = osrm_py.Table(table_params)\n\n"
            "Args:\n\
                table_params (osrm.TableParameters): TableParameters Object.\n\n"
            "Returns:\n\
                (json): [A Table JSON Response](https://project-osrm.org/docs/v5.24.0/api/#table-service).\n\n"
            "Raises:\n\
                RuntimeError: On invalid TableParameters.")
        .def(
            "Tile",
            [](OSRM *t, const TileParameters &params)
            {
                if (!params.IsValid())
                {
                    throw std::runtime_error("Invalid Tile Parameters");
                }

                std::string result;
                t->Tile(params, result);
                nb::object obj = nb::bytes(result.c_str(), result.size());

                return obj;
            },
            "Computes the duration of the fastest route between all pairs of supplied "
            "coordinates.\n\n"
            "Examples:\n\
                >>> res = osrm_py.Tile(tile_params)\n\n"
            "Args:\n\
                tile_params (osrm.TileParameters): TileParameters Object.\n\n"
            "Returns:\n\
                (json): [A Tile JSON Response](https://project-osrm.org/docs/v5.24.0/api/#tile-service).\n\n"
            "Raises:\n\
                RuntimeError: On invalid TileParameters.")
        .def(
            "Trip",
            [](OSRM *t, const TripParameters &params)
            {
                if (!params.IsValid())
                {
                    throw std::runtime_error("Invalid Trip Parameters");
                }

                json::Object result;
                osrm::engine::Status status = t->Trip(params, result);
                osrm_nb_util::check_status(status, result);

                return result;
            },
            "Solves the Traveling Salesman Problem using a greedy heuristic (farthest-insertion "
            "algorithm).\n\n"
            "Examples:\n\
                >>> res = osrm_py.Trip(trip_params)\n\n"
            "Args:\n\
                trip_params (osrm.TripParameters): TripParameters Object.\n\n"
            "Returns:\n\
                (json): [A Trip JSON Response](https://project-osrm.org/docs/v5.24.0/api/#trip-service).\n\n"
            "Raises:\n\
                RuntimeError: On invalid TripParameters.");
}
