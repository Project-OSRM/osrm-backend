#include "python/utility/osrm_utility.hpp"
#include "python/engineconfig_nb.hpp"
#include "python/utility/param_utility.hpp"
#include "engine/engine_config.hpp"
#include "engine/status.hpp"
#include "osrm/osrm.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include <stdexcept>
#include <unordered_map>

#define UNLIMITED -1

namespace nb = nanobind;

using osrm::engine::EngineConfig;

template <typename T> void assign_val(T &to_assign, const std::pair<nb::handle, nb::handle> &val)
{
    try
    {
        to_assign = nb::cast<T>(val.second);
    }
    catch (const nb::cast_error &ex)
    {
        throw std::runtime_error("Invalid type passed for argument: " +
                                 nb::cast<std::string>(val.first));
    }
}

namespace osrm_nb_util
{

void check_status(osrm::engine::Status status, osrm::util::json::Object &res)
{
    if (status == osrm::engine::Status::Ok)
    {
        return;
    }

    const std::string code = std::get<osrm::util::json::String>(res.values.at("code")).value;
    const std::string msg = std::get<osrm::util::json::String>(res.values.at("message")).value;

    throw std::runtime_error(code + " - " + msg);
}

void populate_cfg_from_kwargs(const nb::kwargs &kwargs, EngineConfig &config)
{
    std::unordered_map<std::string, std::function<void(const std::pair<nb::handle, nb::handle> &)>>
        assign_map{{"storage_config",
                    [&config](const std::pair<nb::handle, nb::handle> &val)
                    {
                        std::string str;
                        assign_val(str, val);
                        config.storage_config = osrm::storage::StorageConfig(str);
                    }},
                   {"max_locations_trip",
                    [&config](const std::pair<nb::handle, nb::handle> &val)
                    { assign_val(config.max_locations_trip, val); }},
                   {"max_locations_viaroute",
                    [&config](const std::pair<nb::handle, nb::handle> &val)
                    { assign_val(config.max_locations_viaroute, val); }},
                   {"max_locations_distance_table",
                    [&config](const std::pair<nb::handle, nb::handle> &val)
                    { assign_val(config.max_locations_distance_table, val); }},
                   {"max_locations_map_matching",
                    [&config](const std::pair<nb::handle, nb::handle> &val)
                    { assign_val(config.max_locations_map_matching, val); }},
                   {"max_radius_map_matching",
                    [&config](const std::pair<nb::handle, nb::handle> &val)
                    { assign_val(config.max_radius_map_matching, val); }},
                   {"max_results_nearest",
                    [&config](const std::pair<nb::handle, nb::handle> &val)
                    { assign_val(config.max_results_nearest, val); }},
                   {"default_radius",
                    [&config](const std::pair<nb::handle, nb::handle> &val)
                    {
                        try
                        {
                            const std::string rad_val = nb::cast<std::string>(val.second);

                            if (!(rad_val == "unlimited" || rad_val == "UNLIMITED"))
                            {
                                throw std::runtime_error(
                                    "default_radius must be a float value or 'unlimited'");
                            }

                            config.default_radius = UNLIMITED;
                        }
                        catch (const nb::cast_error &)
                        {
                            assign_val(config.default_radius, val);
                        }
                    }},
                   {"max_alternatives",
                    [&config](const std::pair<nb::handle, nb::handle> &val)
                    { assign_val(config.max_alternatives, val); }},
                   {"use_shared_memory",
                    [&config](const std::pair<nb::handle, nb::handle> &val)
                    { assign_val(config.use_shared_memory, val); }},
                   {"memory_file",
                    [&config](const std::pair<nb::handle, nb::handle> &val)
                    {
                        std::string str;
                        assign_val(str, val);
                        config.memory_file = std::filesystem::path(str);
                    }},
                   {"use_mmap",
                    [&config](const std::pair<nb::handle, nb::handle> &val)
                    { assign_val(config.use_mmap, val); }},
                   {"algorithm",
                    [&config](const std::pair<nb::handle, nb::handle> &val)
                    {
                        std::string str;
                        assign_val(str, val);
                        config.algorithm =
                            osrm_nb_util::str_to_enum(str, "Algorithm", algorithm_map);
                    }},
                   {"verbosity",
                    [&config](const std::pair<nb::handle, nb::handle> &val)
                    { assign_val(config.verbosity, val); }},
                   {"dataset_name", [&config](const std::pair<nb::handle, nb::handle> &val) {
                        assign_val(config.dataset_name, val);
                    }}};

    for (auto kwarg : kwargs)
    {
        const std::string arg_str = nb::cast<std::string>(kwarg.first);
        auto itr = assign_map.find(arg_str);

        if (itr == assign_map.end())
        {
            throw std::invalid_argument(arg_str);
        }

        itr->second(kwarg);
    }
}

} // namespace osrm_nb_util
