#include "extractor/travel_mode.hpp"
#include "guidance/turn_instruction.hpp"

#include "engine/api/json_factory.hpp"
#include "engine/hint.hpp"
#include "engine/polyline_compressor.hpp"
#include "util/integer_range.hpp"

#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>
#include <boost/optional.hpp>

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace TurnType = osrm::guidance::TurnType;
namespace DirectionModifier = osrm::guidance::DirectionModifier;
using TurnInstruction = osrm::guidance::TurnInstruction;

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

// Check whether to include a modifier in the result of the API
inline bool isValidModifier(const guidance::StepManeuver maneuver)
{
    return (maneuver.waypoint_type == guidance::WaypointType::None ||
            maneuver.instruction.direction_modifier != DirectionModifier::UTurn);
}

inline bool hasValidLanes(const guidance::IntermediateIntersection &intersection)
{
    return intersection.lanes.lanes_in_turn > 0;
}

inline util::json::Array toJSON(const extractor::TurnLaneType::Mask lane_type)
{
    util::json::Array result;
    std::bitset<8 * sizeof(extractor::TurnLaneType::Mask)> mask(lane_type);
    for (auto index : util::irange<std::size_t>(0, extractor::TurnLaneType::NUM_TYPES))
    {
        if (mask[index])
        {
            result.values.push_back(extractor::TurnLaneType::laneTypeToName(index));
        }
    }
    return result;
}

util::json::Array lanesFromIntersection(const guidance::IntermediateIntersection &intersection)
{
    BOOST_ASSERT(intersection.lanes.lanes_in_turn >= 1);
    util::json::Array result;
    LaneID lane_id = intersection.lane_description.size();

    for (const auto &lane_desc : intersection.lane_description)
    {
        --lane_id;
        util::json::Object lane;
        lane.values["indications"] = toJSON(lane_desc);
        if (lane_id >= intersection.lanes.first_lane_from_the_right &&
            lane_id <
                intersection.lanes.first_lane_from_the_right + intersection.lanes.lanes_in_turn)
            lane.values["valid"] = util::json::True();
        else
            lane.values["valid"] = util::json::False();

        result.values.push_back(lane);
    }

    return result;
}

const constexpr char *waypoint_type_names[] = {"invalid", "arrive", "depart"};

std::string waypointTypeToString(const guidance::WaypointType waypoint_type)
{
    static_assert(sizeof(waypoint_type_names) / sizeof(waypoint_type_names[0]) >=
                      static_cast<size_t>(guidance::WaypointType::MaxWaypointType),
                  "Some waypoint types has not string representation.");
    return waypoint_type_names[static_cast<std::size_t>(waypoint_type)];
}

util::json::Array coordinateToLonLat(const util::Coordinate &coordinate)
{
    util::json::Array array;
    array.values.push_back(static_cast<double>(util::toFloating(coordinate.lon)));
    array.values.push_back(static_cast<double>(util::toFloating(coordinate.lat)));
    return array;
}

} // namespace detail

util::json::Object makeStepManeuver(const guidance::StepManeuver &maneuver)
{
    util::json::Object step_maneuver;

    std::string maneuver_type;

    if (maneuver.waypoint_type == guidance::WaypointType::None)
        maneuver_type = osrm::guidance::instructionTypeToString(maneuver.instruction.type);
    else
        maneuver_type = detail::waypointTypeToString(maneuver.waypoint_type);

    // These invalid responses should never happen: log if they do happen
    BOOST_ASSERT_MSG(maneuver_type != "invalid", "unexpected invalid maneuver type");

    step_maneuver.values["type"] = std::move(maneuver_type);

    if (detail::isValidModifier(maneuver))
        step_maneuver.values["modifier"] =
            osrm::guidance::instructionModifierToString(maneuver.instruction.direction_modifier);

    step_maneuver.values["location"] = detail::coordinateToLonLat(maneuver.location);
    step_maneuver.values["bearing_before"] = detail::roundAndClampBearing(maneuver.bearing_before);
    step_maneuver.values["bearing_after"] = detail::roundAndClampBearing(maneuver.bearing_after);
    if (maneuver.exit != 0)
        step_maneuver.values["exit"] = maneuver.exit;

    return step_maneuver;
}

util::json::Object makeIntersection(const guidance::IntermediateIntersection &intersection)
{
    util::json::Object result;
    util::json::Array bearings;
    util::json::Array entry;

    bearings.values.reserve(intersection.bearings.size());
    std::transform(intersection.bearings.begin(),
                   intersection.bearings.end(),
                   std::back_inserter(bearings.values),
                   detail::roundAndClampBearing);

    entry.values.reserve(intersection.entry.size());
    std::transform(intersection.entry.begin(),
                   intersection.entry.end(),
                   std::back_inserter(entry.values),
                   [](const bool has_entry) -> util::json::Value {
                       if (has_entry)
                           return util::json::True();
                       else
                           return util::json::False();
                   });

    result.values["location"] = detail::coordinateToLonLat(intersection.location);
    result.values["bearings"] = bearings;
    result.values["entry"] = entry;
    if (intersection.in != guidance::IntermediateIntersection::NO_INDEX)
        result.values["in"] = intersection.in;
    if (intersection.out != guidance::IntermediateIntersection::NO_INDEX)
        result.values["out"] = intersection.out;

    if (detail::hasValidLanes(intersection))
        result.values["lanes"] = detail::lanesFromIntersection(intersection);

    if (!intersection.classes.empty())
    {
        util::json::Array classes;
        classes.values.reserve(intersection.classes.size());
        std::transform(
            intersection.classes.begin(),
            intersection.classes.end(),
            std::back_inserter(classes.values),
            [](const std::string &class_name) { return util::json::String{class_name}; });
        result.values["classes"] = std::move(classes);
    }

    return result;
}

util::json::Object makeRouteStep(guidance::RouteStep step, util::json::Value geometry)
{
    util::json::Object route_step;
    route_step.values["distance"] = std::round(step.distance * 10) / 10.;
    route_step.values["duration"] = step.duration;
    route_step.values["weight"] = step.weight;
    route_step.values["name"] = std::move(step.name);
    if (!step.ref.empty())
        route_step.values["ref"] = std::move(step.ref);
    if (!step.pronunciation.empty())
        route_step.values["pronunciation"] = std::move(step.pronunciation);
    if (!step.destinations.empty())
        route_step.values["destinations"] = std::move(step.destinations);
    if (!step.exits.empty())
        route_step.values["exits"] = std::move(step.exits);
    if (!step.rotary_name.empty())
    {
        route_step.values["rotary_name"] = std::move(step.rotary_name);
        if (!step.rotary_pronunciation.empty())
        {
            route_step.values["rotary_pronunciation"] = std::move(step.rotary_pronunciation);
        }
    }

    route_step.values["mode"] = extractor::travelModeToString(std::move(step.mode));
    route_step.values["maneuver"] = makeStepManeuver(std::move(step.maneuver));
    route_step.values["geometry"] = std::move(geometry);
    route_step.values["driving_side"] = step.is_left_hand_driving ? "left" : "right";

    util::json::Array intersections;
    intersections.values.reserve(step.intersections.size());
    std::transform(step.intersections.begin(),
                   step.intersections.end(),
                   std::back_inserter(intersections.values),
                   makeIntersection);
    route_step.values["intersections"] = std::move(intersections);

    return route_step;
}

util::json::Object makeRoute(const guidance::Route &route,
                             util::json::Array legs,
                             boost::optional<util::json::Value> geometry,
                             const char *weight_name)
{
    util::json::Object json_route;
    json_route.values["distance"] = route.distance;
    json_route.values["duration"] = route.duration;
    json_route.values["weight"] = route.weight;
    json_route.values["weight_name"] = weight_name;
    json_route.values["legs"] = std::move(legs);
    if (geometry)
    {
        json_route.values["geometry"] = *std::move(geometry);
    }
    return json_route;
}

util::json::Object
makeWaypoint(const util::Coordinate &location, const double &distance, std::string name)
{
    util::json::Object waypoint;
    waypoint.values["location"] = detail::coordinateToLonLat(location);
    waypoint.values["name"] = std::move(name);
    waypoint.values["distance"] = distance;
    return waypoint;
}

util::json::Object makeWaypoint(const util::Coordinate &location,
                                const double &distance,
                                std::string name,
                                const Hint &hint)
{
    auto waypoint = makeWaypoint(location, distance, name);
    waypoint.values["hint"] = hint.ToBase64();
    return waypoint;
}

util::json::Object makeRouteLeg(guidance::RouteLeg leg, util::json::Array steps)
{
    util::json::Object route_leg;
    route_leg.values["distance"] = leg.distance;
    route_leg.values["duration"] = leg.duration;
    route_leg.values["weight"] = leg.weight;
    route_leg.values["summary"] = std::move(leg.summary);
    route_leg.values["steps"] = std::move(steps);
    return route_leg;
}

util::json::Object
makeRouteLeg(guidance::RouteLeg leg, util::json::Array steps, util::json::Object annotation)
{
    util::json::Object route_leg = makeRouteLeg(std::move(leg), std::move(steps));
    route_leg.values["annotation"] = std::move(annotation);
    return route_leg;
}

util::json::Array makeRouteLegs(std::vector<guidance::RouteLeg> legs,
                                std::vector<util::json::Value> step_geometries,
                                std::vector<util::json::Object> annotations)
{
    util::json::Array json_legs;
    auto step_geometry_iter = step_geometries.begin();
    for (const auto idx : util::irange<std::size_t>(0UL, legs.size()))
    {
        auto leg = std::move(legs[idx]);
        util::json::Array json_steps;
        json_steps.values.reserve(leg.steps.size());
        std::transform(std::make_move_iterator(leg.steps.begin()),
                       std::make_move_iterator(leg.steps.end()),
                       std::back_inserter(json_steps.values),
                       [&step_geometry_iter](guidance::RouteStep step) {
                           return makeRouteStep(std::move(step), std::move(*step_geometry_iter++));
                       });
        if (annotations.size() > 0)
        {
            json_legs.values.push_back(
                makeRouteLeg(std::move(leg), std::move(json_steps), annotations[idx]));
        }
        else
        {
            json_legs.values.push_back(makeRouteLeg(std::move(leg), std::move(json_steps)));
        }
    }
    return json_legs;
}
} // namespace json
} // namespace api
} // namespace engine
} // namespace osrm
