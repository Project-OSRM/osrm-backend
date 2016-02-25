#include "engine/api/json_factory.hpp"

#include "engine/guidance/route_step.hpp"
#include "engine/guidance/step_maneuver.hpp"
#include "engine/guidance/route_leg.hpp"
#include "engine/guidance/route.hpp"
#include "engine/guidance/leg_geometry.hpp"
#include "engine/guidance/turn_instruction.hpp"
#include "engine/polyline_compressor.hpp"
#include "engine/hint.hpp"

#include <boost/assert.hpp>
#include <boost/range/irange.hpp>

#include <string>
#include <utility>
#include <algorithm>
#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{
namespace json
{
namespace detail
{

std::string instructionTypeToString(guidance::TurnType type)
{
    return guidance::turn_type_names[static_cast<std::size_t>(type)];
}

std::string instructionModifierToString(guidance::DirectionModifier modifier)
{
    return guidance::modifier_names[static_cast<std::size_t>(modifier)];
}

util::json::Array coordinateToLonLat(const util::Coordinate &coordinate)
{
    util::json::Array array;
    array.values.push_back(static_cast<double>(toFloating(coordinate.lon)));
    array.values.push_back(static_cast<double>(toFloating(coordinate.lat)));
    return array;
}

// FIXME this actually needs to be configurable from the profiles
std::string modeToString(const extractor::TravelMode mode)
{
    std::string token;
    switch (mode)
    {
    case TRAVEL_MODE_DEFAULT:
        token = "default";
        break;
    case TRAVEL_MODE_INACCESSIBLE:
        token = "inaccessible";
        break;
    default:
        token = "other";
        break;
    }
    return token;
}

} // namespace detail

util::json::Object makeStepManeuver(const guidance::StepManeuver &maneuver)
{
    util::json::Object step_maneuver;
    step_maneuver.values["type"] = detail::instructionTypeToString(maneuver.instruction.type);
    if( isValidModifier( maneuver.instruction.type, maneuver.instruction.direction_modifier ) )
      step_maneuver.values["modifier"] =
          detail::instructionModifierToString(maneuver.instruction.direction_modifier);
    step_maneuver.values["location"] = detail::coordinateToLonLat(maneuver.location);
    step_maneuver.values["bearing_before"] = maneuver.bearing_before;
    step_maneuver.values["bearing_after"] = maneuver.bearing_after;
    if( maneuver.exit != 0 )
      step_maneuver.values["exit"] = maneuver.exit;
    return step_maneuver;
}

util::json::Object makeRouteStep(guidance::RouteStep &&step,
                                 boost::optional<util::json::Value> geometry)
{
    util::json::Object route_step;
    route_step.values["distance"] = step.distance;
    route_step.values["duration"] = step.duration;
    route_step.values["name"] = std::move(step.name);
    route_step.values["mode"] = detail::modeToString(step.mode);
    route_step.values["maneuver"] = makeStepManeuver(step.maneuver);
    if (geometry)
    {
        route_step.values["geometry"] = std::move(*geometry);
    }
    return route_step;
}

util::json::Object makeRoute(const guidance::Route &route,
                             util::json::Array &&legs,
                             boost::optional<util::json::Value> geometry)
{
    util::json::Object json_route;
    json_route.values["distance"] = route.distance;
    json_route.values["duration"] = route.duration;
    json_route.values["legs"] = std::move(legs);
    if (geometry)
    {
        json_route.values["geometry"] = std::move(*geometry);
    }
    return json_route;
}

util::json::Object
makeWaypoint(const util::Coordinate location, std::string &&name, const Hint &hint)
{
    util::json::Object waypoint;
    waypoint.values["location"] = detail::coordinateToLonLat(location);
    waypoint.values["name"] = std::move(name);
    waypoint.values["hint"] = hint.ToBase64();
    return waypoint;
}

util::json::Object makeRouteLeg(guidance::RouteLeg &&leg, util::json::Array &&steps)
{
    util::json::Object route_leg;
    route_leg.values["distance"] = leg.distance;
    route_leg.values["duration"] = leg.duration;
    route_leg.values["summary"] = std::move(leg.summary);
    route_leg.values["steps"] = std::move(steps);
    return route_leg;
}

util::json::Array makeRouteLegs(std::vector<guidance::RouteLeg> &&legs,
                                const std::vector<guidance::LegGeometry> &leg_geometries)
{
    util::json::Array json_legs;
    for (const auto idx : boost::irange(0UL, legs.size()))
    {
        auto &&leg = std::move(legs[idx]);
        const auto &leg_geometry = leg_geometries[idx];
        util::json::Array json_steps;
        json_steps.values.reserve(leg.steps.size());
        const auto begin_iter = leg_geometry.locations.begin();
        std::transform(
            std::make_move_iterator(leg.steps.begin()), std::make_move_iterator(leg.steps.end()),
            std::back_inserter(json_steps.values), [&begin_iter](guidance::RouteStep &&step)
            {
                // FIXME we only support polyline here
                auto geometry = boost::make_optional<util::json::Value>(
                    makePolyline(begin_iter + step.geometry_begin, begin_iter + step.geometry_end));
                return makeRouteStep(std::move(step), std::move(geometry));
            });
        json_legs.values.push_back(makeRouteLeg(std::move(leg), std::move(json_steps)));
    }

    return json_legs;
}
}
}
} // namespace engine
} // namespace osrm
