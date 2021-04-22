#include "engine/guidance/post_processing.hpp"
#include "guidance/constants.hpp"
#include "guidance/turn_instruction.hpp"

#include "engine/guidance/assemble_steps.hpp"
#include "engine/guidance/lane_processing.hpp"

#include "engine/guidance/collapsing_utility.hpp"
#include "util/bearing.hpp"
#include "util/group_by.hpp"
#include "util/guidance/name_announcements.hpp"
#include "util/guidance/turn_lanes.hpp"

#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/range/iterator_range.hpp>

#include "engine/guidance/collapsing_utility.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>

namespace osrm
{
namespace engine
{
namespace guidance
{
using namespace osrm::guidance;

using RouteStepIterator = std::vector<osrm::engine::guidance::RouteStep>::iterator;

namespace
{

// Ensure that after we are done with the roundabout, only the roundabout instructions themselves
// remain
void compressRange(const RouteStepIterator begin, const RouteStepIterator end)
{
    if (begin == end)
        return;

    for (auto itr = begin + 1; itr != end; ++itr)
    {
        // ensure not to invalidate the final arrive
        if (!hasWaypointType(*itr))
        {
            begin->ElongateBy(*itr);
            itr->Invalidate();
        }
    }
}

// this function handles a single roundabout between enter (which might be missing) to exit (which
// might be missing as well)
void processRoundaboutExits(const RouteStepIterator begin, const RouteStepIterator end)
{
    auto const last = end - 1;
    // If we do not exit the roundabout, there is no exit to report. All good here
    if (!leavesRoundabout(last->maneuver.instruction))
    {
        // first we do some clean-up
        if (begin->maneuver.instruction.type == TurnType::EnterRotary ||
            begin->maneuver.instruction.type == TurnType::EnterRotaryAtExit)
        {
            begin->rotary_name = begin->name;
            begin->rotary_pronunciation = begin->pronunciation;
        }
        // roundabout turns don't make sense without an exit, update the type
        else if (entersRoundabout(begin->maneuver.instruction) &&
                 (begin->maneuver.instruction.type == TurnType::EnterRoundaboutIntersection ||
                  begin->maneuver.instruction.type == TurnType::EnterRoundaboutIntersectionAtExit))
        {
            begin->maneuver.instruction.type = TurnType::EnterRoundabout;
        }

        // We are doing a roundtrip on the roundabout, Nothing to do here but to remove the
        // instructions
        compressRange(begin, end);
        return;
    }

    const auto passes_exit_or_leaves_roundabout = [](auto const &step) {
        return staysOnRoundabout(step.maneuver.instruction) ||
               leavesRoundabout(step.maneuver.instruction);
    };

    // exit count
    const auto exit = std::count_if(begin, end, passes_exit_or_leaves_roundabout);

    // removes all intermediate instructions, assigns names and exit numbers
    BOOST_ASSERT(leavesRoundabout(last->maneuver.instruction));
    BOOST_ASSERT(std::distance(begin, end) >= 1);
    last->maneuver.exit = exit;

    // when we actually have an enter instruction, we can store all the information on it that we
    // need, otherwise we only provide the exit instruciton. In case of re-routing on the
    // roundabout, this might result in strange behaviour, but this way we are more resiliant and we
    // do provide exit after all
    if (entersRoundabout(begin->maneuver.instruction))
    {
        begin->maneuver.exit = exit;
        // special handling for rotaries: remember the name (legacy feature, due to
        // adapt-step-signage)
        if (begin->maneuver.instruction.type == TurnType::EnterRotary ||
            begin->maneuver.instruction.type == TurnType::EnterRotaryAtExit)
        {
            begin->rotary_name = begin->name;
            begin->rotary_pronunciation = begin->pronunciation;
        }
        // compute the total direction modifier for roundabout turns
        else if (begin->maneuver.instruction.type == TurnType::EnterRoundaboutIntersection ||
                 begin->maneuver.instruction.type == TurnType::EnterRoundaboutIntersectionAtExit)
        {
            const auto entry_intersection = begin->intersections.front();

            const auto exit_intersection = last->intersections.front();
            const auto exit_bearing = exit_intersection.bearings[exit_intersection.out];

            BOOST_ASSERT(!begin->intersections.empty());
            const double angle = util::bearing::angleBetween(
                util::bearing::reverse(entry_intersection.bearings[entry_intersection.in]),
                exit_bearing);

            begin->maneuver.instruction.direction_modifier = getTurnDirection(angle);
        }
        begin->AdaptStepSignage(*last);
    }

    // in case of a roundabout turn, we do not emit an exit as long as the mode remains the same
    if ((begin->maneuver.instruction.type == TurnType::EnterRoundaboutIntersection ||
         begin->maneuver.instruction.type == TurnType::EnterRoundaboutIntersectionAtExit) &&
        begin->mode == last->mode)
    {
        compressRange(begin, end);
    }
    else
    {
        // do not remove last (the exit instruction)
        compressRange(begin, last);
    }
}

// roundabout groups are a sequence of roundabout instructions. This can contain enter/exit
// instructions in between
void processRoundaboutGroups(const std::pair<RouteStepIterator, RouteStepIterator> &range)
{
    const auto leaves_roundabout = [](auto const &step) {
        return leavesRoundabout(step.maneuver.instruction);
    };

    auto itr = range.first;
    while (itr != range.second)
    {
        auto exit = std::find_if(itr, range.second, leaves_roundabout);
        if (exit == range.second)
        {
            processRoundaboutExits(itr, exit);
            itr = exit;
        }
        else
        {
            processRoundaboutExits(itr, exit + 1);
            itr = exit + 1;
        }
    }
}

} // namespace

// Every Step Maneuver consists of the information until the turn.
// This list contains a set of instructions, called silent, which should
// not be part of the final output.
// They are required for maintenance purposes. We can calculate the number
// of exits to pass in a roundabout and the number of intersections
// that we come across.
std::vector<RouteStep> handleRoundabouts(std::vector<RouteStep> steps)
{
    // check if a step has roundabout type
    const auto has_roundabout_type = [](auto const &step) {
        return hasRoundaboutType(step.maneuver.instruction);
    };
    const auto first_roundabout_type =
        std::find_if(steps.begin(), steps.end(), has_roundabout_type);

    // no roundabout to process?
    if (first_roundabout_type == steps.end())
        return steps;

    // unless the first instruction enters the roundabout, we are currently on a roundabout. This is
    // a special case that happens if the route starts on a roundabout. It is a border case, but
    // could happen during re-routing. In the case of re-routing, exit counting might be confusing,
    // but it is the best we can do
    bool currently_on_roundabout = !entersRoundabout(first_roundabout_type->maneuver.instruction);

    // this group by paradigm does might contain intermediate roundabout instructions, when they are
    // directly connected. Otherwise it will be a sequence containing everything from enter to exit.
    // If we already start on the roundabout, the first valid place will be steps.begin().
    const auto is_on_roundabout = [&currently_on_roundabout](const auto &step) {
        if (currently_on_roundabout)
        {
            if (leavesRoundabout(step.maneuver.instruction))
                currently_on_roundabout = false;

            return true;
        }
        else
        {
            currently_on_roundabout = entersRoundabout(step.maneuver.instruction);
            auto result = currently_on_roundabout;
            // cases that immediately exit the roundabout
            if (currently_on_roundabout)
                currently_on_roundabout = !leavesRoundabout(step.maneuver.instruction);
            return result;
        }
    };

    // for each range of instructions between begin/end of a roundabout assign
    util::group_by(steps.begin(), steps.end(), is_on_roundabout, processRoundaboutGroups);

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
    // as turn
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

        auto const first_bearing = steps.front().maneuver.bearing_after;
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
        auto bearing = first_bearing;
        // we changed the geometry, we need to recalculate the bearing
        if (geometry.locations[first_step.geometry_begin] !=
            geometry.locations[first_step.geometry_begin + 1])
        {
            bearing = std::round(util::coordinate_calculation::bearing(
                geometry.locations[first_step.geometry_begin],
                geometry.locations[first_step.geometry_begin + 1]));
        }
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
        geometry.annotations.resize(geometry.segment_offsets.back());
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

    BOOST_ASSERT(geometry.segment_offsets.back() + 1 == geometry.locations.size());
    BOOST_ASSERT(geometry.segment_offsets.back() + 1 == geometry.osm_node_ids.size());
    BOOST_ASSERT(geometry.segment_offsets.back() == geometry.annotations.size());

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
            : DirectionModifier::UTurn;

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
            : DirectionModifier::UTurn;

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
                {
                    bool same_name =
                        !(step.name.empty() && step.ref.empty()) &&
                        !util::guidance::requiresNameAnnounced(previous_step.name,
                                                               previous_step.ref,
                                                               previous_step.pronunciation,
                                                               previous_step.exits,
                                                               step.name,
                                                               step.ref,
                                                               step.pronunciation,
                                                               step.exits);

                    step.maneuver.instruction.type =
                        same_name ? TurnType::Continue : TurnType::Turn;
                }
            }

            // Remember the last non silent instruction
            last_valid_instruction = step_index;
        }
    }
    return removeNoTurnInstructions(std::move(steps));
}

void applyOverrides(const datafacade::BaseDataFacade &facade,
                    std::vector<RouteStep> &steps,
                    const LegGeometry &leg_geometry)
{
    // Find overrides that match, and apply them
    // The +/-1 here are to remove the depart and arrive steps, which
    // we don't allow updates to
    for (auto current_step_it = steps.begin(); current_step_it != steps.end(); ++current_step_it)
    {
        util::Log(logDEBUG) << "Searching for " << current_step_it->from_id << std::endl;
        const auto overrides = facade.GetOverridesThatStartAt(current_step_it->from_id);
        if (overrides.empty())
            continue;
        util::Log(logDEBUG) << "~~~~ GOT A HIT, checking the rest ~~~" << std::endl;
        for (const extractor::ManeuverOverride &maneuver_relation : overrides)
        {
            util::Log(logDEBUG) << "Override sequence is ";
            for (auto &n : maneuver_relation.node_sequence)
            {
                util::Log(logDEBUG) << n << " ";
            }
            util::Log(logDEBUG) << std::endl;
            util::Log(logDEBUG) << "Override type is "
                                << osrm::guidance::internalInstructionTypeToString(
                                       maneuver_relation.override_type)
                                << std::endl;
            util::Log(logDEBUG) << "Override direction is "
                                << osrm::guidance::instructionModifierToString(
                                       maneuver_relation.direction)
                                << std::endl;

            util::Log(logDEBUG) << "Route sequence is ";
            for (auto it = current_step_it; it != steps.end(); ++it)
            {
                util::Log(logDEBUG) << it->from_id << " ";
            }
            util::Log(logDEBUG) << std::endl;

            auto search_iter = maneuver_relation.node_sequence.begin();
            auto route_iter = current_step_it;
            while (search_iter != maneuver_relation.node_sequence.end())
            {
                if (route_iter == steps.end())
                    break;

                if (*search_iter == route_iter->from_id)
                {
                    ++search_iter;
                    ++route_iter;
                    continue;
                }
                // Skip over duplicated EBNs in the step array
                // EBNs are sometime duplicated because guidance code inserts
                // "fake" steps that it later removes.  This hasn't happened yet
                // at this point, but we can safely just skip past the dupes.
                if ((route_iter - 1)->from_id == route_iter->from_id)
                {
                    ++route_iter;
                    continue;
                }
                // If we get here, the values got out of sync so it's not
                // a match.
                break;
            }

            // We got a match, update using the instruction_node
            if (search_iter == maneuver_relation.node_sequence.end())
            {
                util::Log(logDEBUG) << "Node sequence matched, looking for the step "
                                    << "that has the via node" << std::endl;
                const auto via_node_coords =
                    facade.GetCoordinateOfNode(maneuver_relation.instruction_node);
                // Find the step that has the instruction_node at the intersection point
                auto step_to_update = std::find_if(
                    current_step_it,
                    route_iter,
                    [&leg_geometry, &via_node_coords](const auto &step) {
                        util::Log(logDEBUG) << "Leg geom from " << step.geometry_begin << " to  "
                                            << step.geometry_end << std::endl;

                        // iterators over geometry of current step
                        auto begin = leg_geometry.locations.begin() + step.geometry_begin;
                        auto end = leg_geometry.locations.begin() + step.geometry_end;
                        auto via_match = std::find_if(begin, end, [&](const auto &location) {
                            return location == via_node_coords;
                        });
                        if (via_match != end)
                        {
                            util::Log(logDEBUG)
                                << "Found geometry match at "
                                << (std::distance(begin, end) - std::distance(via_match, end))
                                << std::endl;
                        }
                        util::Log(logDEBUG)
                            << ((*(leg_geometry.locations.begin() + step.geometry_begin) ==
                                 via_node_coords)
                                    ? "true"
                                    : "false")
                            << std::endl;
                        return *(leg_geometry.locations.begin() + step.geometry_begin) ==
                               via_node_coords;
                        // return via_match != end;
                    });
                // We found a step that had the intersection_node coordinate
                // in its geometry
                if (step_to_update != route_iter)
                {
                    // Don't update the last step (it's an arrive instruction)
                    util::Log(logDEBUG) << "Updating step "
                                        << std::distance(steps.begin(), steps.end()) -
                                               std::distance(step_to_update, steps.end())
                                        << std::endl;
                    if (maneuver_relation.override_type != osrm::guidance::TurnType::MaxTurnType)
                    {
                        util::Log(logDEBUG) << "    instruction was "
                                            << osrm::guidance::internalInstructionTypeToString(
                                                   step_to_update->maneuver.instruction.type)
                                            << " now "
                                            << osrm::guidance::internalInstructionTypeToString(
                                                   maneuver_relation.override_type)
                                            << std::endl;
                        step_to_update->maneuver.instruction.type = maneuver_relation.override_type;
                    }
                    if (maneuver_relation.direction !=
                        osrm::guidance::DirectionModifier::MaxDirectionModifier)
                    {
                        util::Log(logDEBUG)
                            << "    direction was "
                            << osrm::guidance::instructionModifierToString(
                                   step_to_update->maneuver.instruction.direction_modifier)
                            << " now "
                            << osrm::guidance::instructionModifierToString(
                                   maneuver_relation.direction)
                            << std::endl;
                        step_to_update->maneuver.instruction.direction_modifier =
                            maneuver_relation.direction;
                    }
                    // step_to_update->is_overridden = true;
                }
            }
        }
        util::Log(logDEBUG) << "Done tweaking steps" << std::endl;
    }
}

} // namespace guidance
} // namespace engine
} // namespace osrm
