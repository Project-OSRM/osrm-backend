#include "engine/guidance/post_processing.hpp"
#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/turn_instruction.hpp"

#include "engine/guidance/assemble_steps.hpp"
#include "engine/guidance/lane_processing.hpp"

#include "engine/guidance/collapsing_utility.hpp"
#include "util/bearing.hpp"
#include "util/guidance/name_announcements.hpp"
#include "util/guidance/turn_lanes.hpp"

#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/range/iterator_range.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>

using osrm::util::angularDeviation;
using osrm::extractor::guidance::getTurnDirection;
using osrm::extractor::guidance::hasRampType;
using osrm::extractor::guidance::mirrorDirectionModifier;
using osrm::extractor::guidance::bearingToDirectionModifier;

namespace osrm
{
namespace engine
{
namespace guidance
{

namespace
{

void fixFinalRoundabout(std::vector<RouteStep> &steps)
{
    for (std::size_t propagation_index = steps.size() - 1; propagation_index > 0;
         --propagation_index)
    {
        auto &propagation_step = steps[propagation_index];
        propagation_step.maneuver.exit = 0;
        if (entersRoundabout(propagation_step.maneuver.instruction))
        {
            // remember the current name as rotary name in tha case we end in a rotary
            if (propagation_step.maneuver.instruction.type == TurnType::EnterRotary ||
                propagation_step.maneuver.instruction.type == TurnType::EnterRotaryAtExit)
            {
                propagation_step.rotary_name = propagation_step.name;
                propagation_step.rotary_pronunciation = propagation_step.pronunciation;
            }
            else if (propagation_step.maneuver.instruction.type ==
                         TurnType::EnterRoundaboutIntersection ||
                     propagation_step.maneuver.instruction.type ==
                         TurnType::EnterRoundaboutIntersectionAtExit)
            {
                propagation_step.maneuver.instruction.type = TurnType::EnterRoundabout;
            }

            return;
        }
        // accumulate turn data into the enter instructions
        else if (propagation_step.maneuver.instruction.type == TurnType::StayOnRoundabout)
        {
            // TODO this operates on the data that is in the instructions.
            // We are missing out on the final segment after the last stay-on-roundabout
            // instruction though. it is not contained somewhere until now
            steps[propagation_index - 1].ElongateBy(propagation_step);
            steps[propagation_index - 1].maneuver.exit = propagation_step.maneuver.exit;
            propagation_step.Invalidate();
        }
    }
}

bool setUpRoundabout(RouteStep &step)
{
    // basic entry into a roundabout
    // Special case handling, if an entry is directly tied to an exit
    const auto instruction = step.maneuver.instruction;
    if (instruction.type == TurnType::EnterRotaryAtExit ||
        instruction.type == TurnType::EnterRoundaboutAtExit ||
        instruction.type == TurnType::EnterRoundaboutIntersectionAtExit)
    {
        // Here we consider an actual entry, not an exit. We simply have to count the additional
        // exit
        step.maneuver.exit = 1;
        // prevent futher special case handling of these two.
        if (instruction.type == TurnType::EnterRotaryAtExit)
            step.maneuver.instruction.type = TurnType::EnterRotary;
        else if (instruction.type == TurnType::EnterRoundaboutAtExit)
            step.maneuver.instruction.type = TurnType::EnterRoundabout;
        else
            step.maneuver.instruction.type = TurnType::EnterRoundaboutIntersection;
    }

    if (leavesRoundabout(instruction))
    {
        // This set-up, even though it looks the same, is actually looking at entering AND exiting
        step.maneuver.exit = 1; // count the otherwise missing exit

        // prevent futher special case handling of these two.
        if (instruction.type == TurnType::EnterAndExitRotary)
            step.maneuver.instruction.type = TurnType::EnterRotary;
        else if (instruction.type == TurnType::EnterAndExitRoundabout)
            step.maneuver.instruction.type = TurnType::EnterRoundabout;
        else
            step.maneuver.instruction.type = TurnType::EnterRoundaboutIntersection;
        return false;
    }
    else
    {
        return true;
    }
}

void closeOffRoundabout(const bool on_roundabout,
                        std::vector<RouteStep> &steps,
                        std::size_t step_index)
{
    auto &step = steps[step_index];
    step.maneuver.exit += 1;
    if (!on_roundabout)
    {
        BOOST_ASSERT(steps.size() >= 2);

        // We reached a special case that requires the addition of a special route step in the
        // beginning. We started in a roundabout, so to announce the exit, we move use the exit
        // instruction and move it right to the beginning to make sure to immediately announce the
        // exit.
        BOOST_ASSERT(leavesRoundabout(steps[1].maneuver.instruction) ||
                     steps[1].maneuver.instruction.type == TurnType::StayOnRoundabout ||
                     steps[1].maneuver.instruction.type == TurnType::Suppressed ||
                     steps[1].maneuver.instruction.type == TurnType::NoTurn ||
                     steps[1].maneuver.instruction.type == TurnType::UseLane);
        steps[0].geometry_end = 1;
        steps[1].geometry_begin = 0;
        steps[1].AddInFront(steps[0]);
        steps[1].intersections.erase(steps[1].intersections.begin()); // otherwise we copy the
                                                                      // source
        if (leavesRoundabout(steps[1].maneuver.instruction))
            steps[1].maneuver.exit = 1;
        steps[0].duration = 0;
        steps[0].distance = 0;
        const auto exitToEnter = [](const TurnType::Enum type) {
            if (TurnType::ExitRotary == type)
                return TurnType::EnterRotary;
            // if we do not enter the roundabout Intersection, we cannot treat the full traversal as
            // a turn. So we switch it up to the roundabout type
            else if (type == TurnType::ExitRoundaboutIntersection)
                return TurnType::EnterRoundabout;
            else
                return TurnType::EnterRoundabout;
        };
        steps[1].maneuver.instruction.type = exitToEnter(step.maneuver.instruction.type);
        if (steps[1].maneuver.instruction.type == TurnType::EnterRotary)
        {
            steps[1].rotary_name = steps[0].name;
            steps[1].rotary_pronunciation = steps[0].pronunciation;
        }
    }

    if (step_index > 1)
    {
        auto &exit_step = steps[step_index];
        auto &prev_step = steps[step_index - 1];
        // In case the step with the roundabout exit instruction cannot be merged with the
        // previous step we change the instruction to a normal turn
        if (!guidance::haveSameMode(exit_step, prev_step))
        {
            BOOST_ASSERT(leavesRoundabout(exit_step.maneuver.instruction));
            prev_step.maneuver.instruction = exit_step.maneuver.instruction;
            if (!entersRoundabout(prev_step.maneuver.instruction))
                prev_step.maneuver.exit = exit_step.maneuver.exit;
            exit_step.maneuver.instruction.type = TurnType::Notification;
            step_index--;
        }
    }

    // Normal exit from the roundabout, or exit from a previously fixed roundabout. Propagate the
    // index back to the entering location and prepare the current silent set of instructions for
    // removal.
    std::vector<std::size_t> intermediate_steps;
    BOOST_ASSERT(!steps[step_index].intersections.empty());
    // the very first intersection in the steps represents the location of the turn. Following
    // intersections are locations passed along the way
    const auto exit_intersection = steps[step_index].intersections.front();
    const auto exit_bearing = exit_intersection.bearings[exit_intersection.out];
    const auto destination_copy = step;

    if (step_index > 1)
    {
        // The very first route-step is head, so we cannot iterate past that one
        for (std::size_t propagation_index = step_index - 1; propagation_index > 0;
             --propagation_index)
        {
            auto &propagation_step = steps[propagation_index];
            auto &next_step = steps[propagation_index + 1];
            propagation_step.ElongateBy(next_step);
            propagation_step.maneuver.exit = next_step.maneuver.exit;
            next_step.Invalidate();

            if (entersRoundabout(propagation_step.maneuver.instruction))
            {
                const auto entry_intersection = propagation_step.intersections.front();

                // remember rotary name
                if (propagation_step.maneuver.instruction.type == TurnType::EnterRotary ||
                    propagation_step.maneuver.instruction.type == TurnType::EnterRotaryAtExit)
                {
                    propagation_step.rotary_name = propagation_step.name;
                    propagation_step.rotary_pronunciation = propagation_step.pronunciation;
                }
                else if (propagation_step.maneuver.instruction.type ==
                             TurnType::EnterRoundaboutIntersection ||
                         propagation_step.maneuver.instruction.type ==
                             TurnType::EnterRoundaboutIntersectionAtExit)
                {
                    BOOST_ASSERT(!propagation_step.intersections.empty());
                    const double angle = util::bearing::angleBetween(
                        util::bearing::reverse(entry_intersection.bearings[entry_intersection.in]),
                        exit_bearing);

                    auto bearings = propagation_step.intersections.front().bearings;
                    propagation_step.maneuver.instruction.direction_modifier =
                        getTurnDirection(angle);
                }

                propagation_step.AdaptStepSignage(destination_copy);
                break;
            }
        }
        // remove exit
    }
}

} // namespace

// Every Step Maneuver consists of the information until the turn.
// This list contains a set of instructions, called silent, which should
// not be part of the final output.
// They are required for maintenance purposes. We can calculate the number
// of exits to pass in a roundabout and the number of intersections
// that we come across.
std::vector<RouteStep> postProcess(std::vector<RouteStep> steps)
{
    // the steps should always include the first/last step in form of a location
    BOOST_ASSERT(steps.size() >= 2);
    if (steps.size() == 2)
        return steps;

    // Count Street Exits forward
    bool on_roundabout = false;
    bool has_entered_roundabout = false;

    // count the exits forward. if enter/exit roundabout happen both, no further treatment is
    // required. We might end up with only one of them (e.g. starting within a roundabout)
    // or having a via-point in the roundabout.
    // In this case, exits are numbered from the start of the leg.
    for (std::size_t step_index = 0; step_index < steps.size(); ++step_index)
    {
        const auto next_step_index = step_index + 1;
        auto &step = steps[step_index];
        const auto instruction = step.maneuver.instruction;
        if (entersRoundabout(instruction))
        {
            has_entered_roundabout = setUpRoundabout(step);

            if (has_entered_roundabout && next_step_index < steps.size())
                steps[next_step_index].maneuver.exit = step.maneuver.exit;
        }
        else if (instruction.type == TurnType::StayOnRoundabout)
        {
            on_roundabout = true;
            // increase the exit number we require passing the exit
            step.maneuver.exit += 1;
            if (next_step_index < steps.size())
                steps[next_step_index].maneuver.exit = step.maneuver.exit;
        }
        else if (leavesRoundabout(instruction))
        {
            // if (!has_entered_roundabout)
            // in case the we are not on a roundabout, the very first instruction
            // after the depart will be transformed into a roundabout and become
            // the first valid instruction
            closeOffRoundabout(has_entered_roundabout, steps, step_index);
            has_entered_roundabout = false;
            on_roundabout = false;
        }
        else if (on_roundabout && next_step_index < steps.size())
        {
            steps[next_step_index].maneuver.exit = step.maneuver.exit;
        }
    }

    // unterminated roundabout
    // Move backwards through the instructions until the start and remove the exit number
    // A roundabout without exit translates to enter-roundabout
    if (has_entered_roundabout || on_roundabout)
    {
        fixFinalRoundabout(steps);
    }

    BOOST_ASSERT(steps.front().intersections.size() >= 1);
    BOOST_ASSERT(steps.front().intersections.front().bearings.size() == 1);
    BOOST_ASSERT(steps.front().intersections.front().entry.size() == 1);
    BOOST_ASSERT(steps.front().maneuver.waypoint_type == WaypointType::Depart);

    BOOST_ASSERT(steps.back().intersections.size() == 1);
    BOOST_ASSERT(steps.back().intersections.front().bearings.size() == 1);
    BOOST_ASSERT(steps.back().intersections.front().entry.size() == 1);
    BOOST_ASSERT(steps.back().maneuver.waypoint_type == WaypointType::Arrive);

    return removeNoTurnInstructions(std::move(steps));
}

// Doing this step in post-processing provides a few challenges we cannot overcome.
// The removal of an initial step imposes some copy overhead in the steps, moving all later
// steps to the front. In addition, we cannot reduce the travel time that is accumulated at a
// different location.
// As a direct implication, we have to keep the time of the initial/final turns (which adds a
// few seconds of inaccuracy at both ends. This is acceptable, however, since the turn should
// usually not be as relevant.
void trimShortSegments(std::vector<RouteStep> &steps, LegGeometry &geometry)
{
    if (steps.size() < 2 || geometry.locations.size() <= 2)
        return;

    // if phantom node is located at the connection of two segments, either one can be selected
    // as
    // turn
    //
    // a --- b
    //       |
    //       c
    //
    // If a route from b to c is requested, both a--b and b--c could be selected as start
    // segment.
    // In case of a--b, we end up with an unwanted turn saying turn-right onto b-c.
    // These cases start off with an initial segment which is of zero length.
    // We have to be careful though, since routing that starts in a roundabout has a valid.
    // To catch these cases correctly, we have to perform trimming prior to the post-processing

    BOOST_ASSERT(geometry.locations.size() >= steps.size());
    // Look for distances under 1m
    const bool zero_length_step = steps.front().distance <= 1 && steps.size() > 2;
    const bool duplicated_coordinate = util::coordinate_calculation::haversineDistance(
                                           geometry.locations[0], geometry.locations[1]) <= 1;
    if (zero_length_step || duplicated_coordinate)
    {

        // remove the initial distance value
        geometry.segment_distances.erase(geometry.segment_distances.begin());

        const auto offset = zero_length_step ? geometry.segment_offsets[1] : 1;
        if (offset > 0)
        {
            // fixup the coordinates/annotations/ids
            geometry.locations.erase(geometry.locations.begin(),
                                     geometry.locations.begin() + offset);
            geometry.annotations.erase(geometry.annotations.begin(),
                                       geometry.annotations.begin() + offset);
            geometry.osm_node_ids.erase(geometry.osm_node_ids.begin(),
                                        geometry.osm_node_ids.begin() + offset);
        }

        // We have to adjust the first step both for its name and the bearings
        if (zero_length_step)
        {
            // since we are not only checking for epsilon but for a full meter, we can have multiple
            // coordinates here. Move all offsets to the front and reduce by one. (This is an
            // inplace forward one and reduce by one)
            std::transform(geometry.segment_offsets.begin() + 1,
                           geometry.segment_offsets.end(),
                           geometry.segment_offsets.begin(),
                           [offset](const std::size_t val) { return val - offset; });

            geometry.segment_offsets.pop_back();
            const auto &current_depart = steps.front();
            auto &designated_depart = *(steps.begin() + 1);

            // FIXME this is required to be consistent with the route durations. The initial
            // turn is not actually part of the route, though
            designated_depart.duration += current_depart.duration;

            // update initial turn direction/bearings. Due to the duplicated first coordinate,
            // the initial bearing is invalid
            designated_depart.maneuver.waypoint_type = WaypointType::Depart;
            designated_depart.maneuver.bearing_before = 0;
            designated_depart.maneuver.instruction = TurnInstruction::NO_TURN();
            // we need to make this conform with the intersection format for the first intersection
            auto &first_intersection = designated_depart.intersections.front();
            designated_depart.intersections.front().lanes = util::guidance::LaneTuple();
            designated_depart.intersections.front().lane_description.clear();
            first_intersection.bearings = {first_intersection.bearings[first_intersection.out]};
            first_intersection.entry = {true};
            first_intersection.in = IntermediateIntersection::NO_INDEX;
            first_intersection.out = 0;

            // finally remove the initial (now duplicated move)
            steps.erase(steps.begin());
        }
        else
        {
            // we need to make this at least 1 because we will substract 1
            // from all offsets at the end of the loop.
            steps.front().geometry_begin = 1;

            // reduce all offsets by one (inplace)
            std::transform(geometry.segment_offsets.begin(),
                           geometry.segment_offsets.end(),
                           geometry.segment_offsets.begin(),
                           [](const std::size_t val) { return val - 1; });
        }

        // and update the leg geometry indices for the removed entry
        std::for_each(steps.begin(), steps.end(), [offset](RouteStep &step) {
            step.geometry_begin -= offset;
            step.geometry_end -= offset;
        });

        auto &first_step = steps.front();
        // we changed the geometry, we need to recalculate the bearing
        auto bearing = std::round(util::coordinate_calculation::bearing(
            geometry.locations[first_step.geometry_begin],
            geometry.locations[first_step.geometry_begin + 1]));
        first_step.maneuver.bearing_after = bearing;
        first_step.intersections.front().bearings.front() = bearing;
    }

    BOOST_ASSERT(steps.front().intersections.size() >= 1);
    BOOST_ASSERT(steps.front().intersections.front().bearings.size() == 1);
    BOOST_ASSERT(steps.front().intersections.front().entry.size() == 1);
    BOOST_ASSERT(steps.front().maneuver.waypoint_type == WaypointType::Depart);

    BOOST_ASSERT(steps.back().intersections.size() == 1);
    BOOST_ASSERT(steps.back().intersections.front().bearings.size() == 1);
    BOOST_ASSERT(steps.back().intersections.front().entry.size() == 1);
    BOOST_ASSERT(steps.back().maneuver.waypoint_type == WaypointType::Arrive);

    // make sure we still have enough segments
    if (steps.size() < 2 || geometry.locations.size() == 2)
        return;

    BOOST_ASSERT(geometry.locations.size() >= steps.size());
    auto &next_to_last_step = *(steps.end() - 2);
    // in the end, the situation with the roundabout cannot occur. As a result, we can remove
    // all zero-length instructions
    if (next_to_last_step.distance <= 1 && steps.size() > 2)
    {
        geometry.segment_offsets.pop_back();
        // remove all the last coordinates from the geometry
        geometry.locations.resize(geometry.segment_offsets.back() + 1);
        geometry.annotations.resize(geometry.segment_offsets.back() + 1);
        geometry.osm_node_ids.resize(geometry.segment_offsets.back() + 1);

        BOOST_ASSERT(geometry.segment_distances.back() <= 1);
        geometry.segment_distances.pop_back();

        next_to_last_step.maneuver.waypoint_type = WaypointType::Arrive;
        next_to_last_step.maneuver.instruction = TurnInstruction::NO_TURN();
        next_to_last_step.maneuver.bearing_after = 0;
        next_to_last_step.intersections.front().lanes = util::guidance::LaneTuple();
        next_to_last_step.intersections.front().lane_description.clear();
        next_to_last_step.geometry_end = next_to_last_step.geometry_begin + 1;
        BOOST_ASSERT(next_to_last_step.intersections.size() == 1);
        auto &last_intersection = next_to_last_step.intersections.back();
        last_intersection.bearings = {last_intersection.bearings[last_intersection.in]};
        last_intersection.entry = {true};
        last_intersection.out = IntermediateIntersection::NO_INDEX;
        last_intersection.in = 0;
        steps.pop_back();

        // Because we eliminated a really short segment, it was probably
        // near an intersection.  The convention is *not* to make the
        // turn, so the `arrive` instruction should be on the same road
        // as the segment before it.  Thus, we have to copy the names
        // and travel modes from the new next_to_last step.
        auto &new_next_to_last = *(steps.end() - 2);
        next_to_last_step.AdaptStepSignage(new_next_to_last);
        next_to_last_step.mode = new_next_to_last.mode;
        // the geometry indices of the last step are already correct;
    }
    else if (util::coordinate_calculation::haversineDistance(
                 geometry.locations[geometry.locations.size() - 2],
                 geometry.locations[geometry.locations.size() - 1]) <= 1)
    {
        // correct steps but duplicated coordinate in the end.
        // This can happen if the last coordinate snaps to a node in the unpacked geometry
        geometry.locations.pop_back();
        geometry.annotations.pop_back();
        geometry.osm_node_ids.pop_back();
        geometry.segment_offsets.back()--;
        // since the last geometry includes the location of arrival, the arrival instruction
        // geometry overlaps with the previous segment
        BOOST_ASSERT(next_to_last_step.geometry_end == steps.back().geometry_begin + 1);
        BOOST_ASSERT(next_to_last_step.geometry_begin < next_to_last_step.geometry_end);
        next_to_last_step.geometry_end--;
        auto &last_step = steps.back();
        last_step.geometry_begin--;
        last_step.geometry_end--;
        BOOST_ASSERT(next_to_last_step.geometry_end == last_step.geometry_begin + 1);
        BOOST_ASSERT(last_step.geometry_begin == last_step.geometry_end - 1);
        BOOST_ASSERT(next_to_last_step.geometry_end >= 2);
        // we changed the geometry, we need to recalculate the bearing
        auto bearing = std::round(util::coordinate_calculation::bearing(
            geometry.locations[next_to_last_step.geometry_end - 2],
            geometry.locations[last_step.geometry_begin]));
        last_step.maneuver.bearing_before = bearing;
        last_step.intersections.front().bearings.front() = util::bearing::reverse(bearing);
    }

    BOOST_ASSERT(steps.back().geometry_end == geometry.locations.size());

    BOOST_ASSERT(steps.front().intersections.size() >= 1);
    BOOST_ASSERT(steps.front().intersections.front().bearings.size() == 1);
    BOOST_ASSERT(steps.front().intersections.front().entry.size() == 1);
    BOOST_ASSERT(steps.front().maneuver.waypoint_type == WaypointType::Depart);

    BOOST_ASSERT(steps.back().intersections.size() == 1);
    BOOST_ASSERT(steps.back().intersections.front().bearings.size() == 1);
    BOOST_ASSERT(steps.back().intersections.front().entry.size() == 1);
    BOOST_ASSERT(steps.back().maneuver.waypoint_type == WaypointType::Arrive);
}

// assign relative locations to depart/arrive instructions
std::vector<RouteStep> assignRelativeLocations(std::vector<RouteStep> steps,
                                               const LegGeometry &leg_geometry,
                                               const PhantomNode &source_node,
                                               const PhantomNode &target_node)
{
    // We report the relative position of source/target to the road only within a range that is
    // sufficiently different but not full of the path
    BOOST_ASSERT(steps.size() >= 2);
    BOOST_ASSERT(leg_geometry.locations.size() >= 2);
    const constexpr double MINIMAL_RELATIVE_DISTANCE = 5., MAXIMAL_RELATIVE_DISTANCE = 300.;
    const auto distance_to_start = util::coordinate_calculation::haversineDistance(
        source_node.input_location, leg_geometry.locations[0]);
    const auto initial_modifier =
        distance_to_start >= MINIMAL_RELATIVE_DISTANCE &&
                distance_to_start <= MAXIMAL_RELATIVE_DISTANCE
            ? bearingToDirectionModifier(util::coordinate_calculation::computeAngle(
                  source_node.input_location, leg_geometry.locations[0], leg_geometry.locations[1]))
            : extractor::guidance::DirectionModifier::UTurn;

    steps.front().maneuver.instruction.direction_modifier = initial_modifier;

    const auto distance_from_end = util::coordinate_calculation::haversineDistance(
        target_node.input_location, leg_geometry.locations.back());
    const auto final_modifier =
        distance_from_end >= MINIMAL_RELATIVE_DISTANCE &&
                distance_from_end <= MAXIMAL_RELATIVE_DISTANCE
            ? bearingToDirectionModifier(util::coordinate_calculation::computeAngle(
                  leg_geometry.locations[leg_geometry.locations.size() - 2],
                  leg_geometry.locations[leg_geometry.locations.size() - 1],
                  target_node.input_location))
            : extractor::guidance::DirectionModifier::UTurn;

    steps.back().maneuver.instruction.direction_modifier = final_modifier;

    BOOST_ASSERT(steps.front().intersections.size() >= 1);
    BOOST_ASSERT(steps.front().intersections.front().bearings.size() == 1);
    BOOST_ASSERT(steps.front().intersections.front().entry.size() == 1);
    BOOST_ASSERT(steps.front().maneuver.waypoint_type == WaypointType::Depart);

    BOOST_ASSERT(steps.back().intersections.size() == 1);
    BOOST_ASSERT(steps.back().intersections.front().bearings.size() == 1);
    BOOST_ASSERT(steps.back().intersections.front().entry.size() == 1);
    BOOST_ASSERT(steps.back().maneuver.waypoint_type == WaypointType::Arrive);
    return steps;
}

LegGeometry resyncGeometry(LegGeometry leg_geometry, const std::vector<RouteStep> &steps)
{
    // The geometry uses an adjacency array-like structure for representation.
    // To sync it back up with the steps, we cann add a segment for every step.
    leg_geometry.segment_offsets.clear();
    leg_geometry.segment_distances.clear();
    leg_geometry.segment_offsets.push_back(0);

    for (const auto &step : steps)
    {
        leg_geometry.segment_distances.push_back(step.distance);
        // the leg geometry does not follow the begin/end-convetion. So we have to subtract one
        // to get the back-index.
        leg_geometry.segment_offsets.push_back(step.geometry_end - 1);
    }

    // remove the data from the reached-target step again
    leg_geometry.segment_offsets.pop_back();
    leg_geometry.segment_distances.pop_back();

    return leg_geometry;
}

std::vector<RouteStep> buildIntersections(std::vector<RouteStep> steps)
{
    std::size_t last_valid_instruction = 0;
    for (std::size_t step_index = 0; step_index < steps.size(); ++step_index)
    {
        auto &step = steps[step_index];
        const auto instruction = step.maneuver.instruction;
        if (instruction.type == TurnType::Suppressed)
        {
            BOOST_ASSERT(steps[last_valid_instruction].mode == step.mode);
            // count intersections. We cannot use exit, since intersections can follow directly
            // after a roundabout
            steps[last_valid_instruction].ElongateBy(step);
            steps[step_index].Invalidate();
        }
        else if (!isSilent(instruction))
        {

            // End of road is a turn that helps to identify the location of a turn. If the turn does
            // not pass by any oter intersections, the end-of-road characteristic does not improve
            // the instructions.
            // Here we reduce the verbosity of our output by reducing end-of-road emissions in cases
            // where no intersections have been passed in between.
            // Since the instruction is located at the beginning of a step, we need to check the
            // previous instruction.
            if (instruction.type == TurnType::EndOfRoad)
            {
                BOOST_ASSERT(step_index > 0);
                const auto &previous_step = steps[last_valid_instruction];
                if (previous_step.intersections.size() < MIN_END_OF_ROAD_INTERSECTIONS)
                    step.maneuver.instruction.type = TurnType::Turn;
            }

            // Remember the last non silent instruction
            last_valid_instruction = step_index;
        }
    }
    return removeNoTurnInstructions(std::move(steps));
}

} // namespace guidance
} // namespace engine
} // namespace osrm
