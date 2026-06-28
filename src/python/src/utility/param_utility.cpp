#include "python/utility/param_utility.hpp"
#include "engine/api/base_parameters.hpp"
#include "engine/api/match_parameters.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/api/trip_parameters.hpp"
#include "engine/approach.hpp"
#include "engine/hint.hpp"

#include <optional>
#include <string>
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

void assign_baseparameters(BaseParameters *params,
                           std::vector<osrm::util::Coordinate> coordinates,
                           std::vector<std::optional<std::string>> hints,
                           std::vector<std::optional<double>> radiuses,
                           std::vector<std::optional<osrm::engine::Bearing>> bearings,
                           const std::vector<std::optional<osrm::engine::Approach>> &approaches,
                           bool generate_hints,
                           std::vector<std::string> exclude,
                           const BaseParameters::SnappingType snapping)
{
    params->coordinates = std::move(coordinates);
    params->hints.clear();
    for (const auto &h : hints)
    {
        if (h)
        {
            params->hints.push_back(osrm::engine::Hint::FromBase64(*h));
        }
        else
        {
            params->hints.push_back(std::nullopt);
        }
    }
    params->radiuses = std::move(radiuses);
    params->bearings = std::move(bearings);
    params->approaches = approaches;
    params->generate_hints = generate_hints;
    params->exclude = std::move(exclude);
    params->snapping = snapping;
}

void assign_routeparameters(RouteParameters *params,
                            const bool steps,
                            int number_of_alternatives,
                            const std::vector<RouteParameters::AnnotationsType> &annotations,
                            RouteParameters::GeometriesType geometries,
                            RouteParameters::OverviewType overview,
                            const std::optional<bool> continue_straight,
                            std::vector<std::size_t> waypoints)
{
    params->steps = steps;
    params->alternatives = (bool)number_of_alternatives;
    params->number_of_alternatives = number_of_alternatives;
    params->annotations = !annotations.empty();
    params->annotations_type = calculate_routeannotations_type(annotations);
    params->geometries = geometries;
    params->overview = overview;
    params->continue_straight = continue_straight;
    params->waypoints = std::move(waypoints);
}

RouteParameters::AnnotationsType
calculate_routeannotations_type(const std::vector<RouteParameters::AnnotationsType> &annotations)
{
    RouteParameters::AnnotationsType res = RouteParameters::AnnotationsType::None;

    for (size_t i = 0; i < annotations.size(); ++i)
    {
        res = res | annotations[i];
    }

    return res;
}

TableParameters::AnnotationsType
calculate_tableannotations_type(const std::vector<TableParameters::AnnotationsType> &annotations)
{
    if (annotations.empty())
    {
        return TableParameters::AnnotationsType::Duration;
    }

    TableParameters::AnnotationsType res = TableParameters::AnnotationsType::None;

    for (size_t i = 0; i < annotations.size(); ++i)
    {
        res = res | annotations[i];
    }

    return res;
}

} // namespace osrm_nb_util
