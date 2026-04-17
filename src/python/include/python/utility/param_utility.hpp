#ifndef OSRM_NB_PARAM_UTIL_H
#define OSRM_NB_PARAM_UTIL_H

#include "engine/api/base_parameters.hpp"
#include "engine/api/match_parameters.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/api/trip_parameters.hpp"
#include "engine/approach.hpp"

#include <optional>
#include <unordered_map>
#include <vector>

using osrm::engine::Approach;
using osrm::engine::api::BaseParameters;
using osrm::engine::api::MatchParameters;
using osrm::engine::api::RouteParameters;
using osrm::engine::api::TableParameters;
using osrm::engine::api::TripParameters;

namespace osrm_nb_util
{

template <typename T>
T str_to_enum(const std::string &str,
              const std::string &type_name,
              const std::unordered_map<std::string, T> &enum_map)
{
    auto itr = enum_map.find(str);

    if (itr != enum_map.end())
    {
        return itr->second;
    }

    std::string valid_strs = "(Valid Options: ";
    bool first = true;

    for (auto itr : enum_map)
    {
        if (itr.first.empty())
        {
            continue;
        }
        if (!first)
        {
            valid_strs += ", ";
        }
        valid_strs += "'" + itr.first + "'";
        first = false;
    }
    valid_strs += ")";

    throw std::invalid_argument("Invalid " + type_name + ": '" + str + "' " + valid_strs);
}

template <typename T>
std::string enum_to_str(T enum_type,
                        const std::string &type_name,
                        const std::unordered_map<std::string, T> &enum_map)
{
    for (auto itr : enum_map)
    {
        if (itr.second == enum_type)
        {
            return itr.first;
        }
    }

    throw std::invalid_argument("Undefined " + type_name + " Enum");
}

void assign_baseparameters(BaseParameters *params,
                           std::vector<osrm::util::Coordinate> coordinates,
                           std::vector<std::optional<std::string>> hints,
                           std::vector<std::optional<double>> radiuses,
                           std::vector<std::optional<osrm::engine::Bearing>> bearings,
                           const std::vector<std::optional<osrm::engine::Approach>> &approaches,
                           bool generate_hints,
                           std::vector<std::string> exclude,
                           const BaseParameters::SnappingType snapping);

void assign_routeparameters(RouteParameters *params,
                            const bool steps,
                            int number_of_alternatives,
                            const std::vector<RouteParameters::AnnotationsType> &annotations,
                            RouteParameters::GeometriesType geometries,
                            RouteParameters::OverviewType overview,
                            const std::optional<bool> continue_straight,
                            std::vector<std::size_t> waypoints);

RouteParameters::AnnotationsType
calculate_routeannotations_type(const std::vector<RouteParameters::AnnotationsType> &annotations);

TableParameters::AnnotationsType
calculate_tableannotations_type(const std::vector<TableParameters::AnnotationsType> &annotations);

} // namespace osrm_nb_util

#endif // OSRM_NB_PARAM_UTIL_H
