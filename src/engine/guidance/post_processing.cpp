#include "engine/guidance/post_processing.hpp"
#include "extractor/guidance/turn_instruction.hpp"

#include "engine/guidance/assemble_steps.hpp"
#include "engine/guidance/toolkit.hpp"

#include "util/guidance/toolkit.hpp"

#include <boost/assert.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <utility>

using TurnInstruction = osrm::extractor::guidance::TurnInstruction;
using TurnType = osrm::extractor::guidance::TurnType;
using DirectionModifier = osrm::extractor::guidance::DirectionModifier;
using osrm::util::guidance::angularDeviation;
using osrm::util::guidance::getTurnDirection;

namespace osrm
{
namespace engine
{
namespace guidance
{

namespace
{

// invalidate a step and set its content to nothing
void invalidateStep(RouteStep &step) { step = getInvalidRouteStep(); }

void print(const std::vector<RouteStep> &steps)
{
    std::cout << "Path\n";
    int segment = 0;
    for (const auto &step : steps)
    {
        const auto type =
            static_cast<std::underlying_type<TurnType>::type>(step.maneuver.instruction.type);
        const auto modifier = static_cast<std::underlying_type<DirectionModifier>::type>(
            step.maneuver.instruction.direction_modifier);

        std::cout << "\t[" << ++segment << "]: " << type << " " << modifier
                  << " Duration: " << step.duration << " Distance: " << step.distance
                  << " Geometry: " << step.geometry_begin << " " << step.geometry_end
                  << " exit: " << step.maneuver.exit
                  << " Intersections: " << step.intersections.size() << " [";

        for (const auto &intersection : step.intersections)
        {
            std::cout << "(bearings:";
            for( auto bearing : intersection.bearings)
                std:: cout << " " << bearing;
            std::cout << ", entry: ";
            for( auto entry : intersection.entry)
                std:: cout << " " << entry;
            std::cout << ")";
        }

        std::cout << "] name[" << step.name_id << "]: " << step.name << std::endl;
    }
}

RouteStep forwardInto(RouteStep destination, const RouteStep &source)
{
    // Merge a turn into a silent turn
    // Overwrites turn instruction and increases exit NR
    destination.duration += source.duration;
    destination.distance += source.distance;

    if (destination.geometry_begin < source.geometry_begin)
    {
        destination.intersections.insert(destination.intersections.end(),
                                         source.intersections.begin(), source.intersections.end());
    }
    else
    {
        destination.intersections.insert(destination.intersections.begin(),
                                         source.intersections.begin(), source.intersections.end());
    }

    destination.geometry_begin = std::min(destination.geometry_begin, source.geometry_begin);
    destination.geometry_end = std::max(destination.geometry_end, source.geometry_end);
    destination.maneuver.exit = destination.intersections.size() - 1;

    return destination;
}

void fixFinalRoundabout(std::vector<RouteStep> &steps)
{
    for (std::size_t propagation_index = steps.size() - 1; propagation_index > 0;
         --propagation_index)
    {
        auto &propagation_step = steps[propagation_index];
        if (entersRoundabout(propagation_step.maneuver.instruction))
        {
            propagation_step.maneuver.exit = 0;
            propagation_step.geometry_end = steps.back().geometry_begin;

            // remember the current name as rotary name in tha case we end in a rotary
            if (propagation_step.maneuver.instruction.type == TurnType::EnterRotary ||
                propagation_step.maneuver.instruction.type == TurnType::EnterRotaryAtExit)
                propagation_step.rotary_name = propagation_step.name;

            else if (propagation_step.maneuver.instruction.type ==
                         TurnType::EnterRoundaboutIntersection ||
                     propagation_step.maneuver.instruction.type ==
                         TurnType::EnterRoundaboutIntersectionAtExit)
                propagation_step.maneuver.instruction.type = TurnType::EnterRoundabout;

            return;
        }
        // accumulate turn data into the enter instructions
        else if (propagation_step.maneuver.instruction.type == TurnType::StayOnRoundabout)
        {
            // TODO this operates on the data that is in the instructions.
            // We are missing out on the final segment after the last stay-on-roundabout
            // instruction though. it is not contained somewhere until now
            steps[propagation_index - 1] =
                forwardInto(std::move(steps[propagation_index - 1]), propagation_step);
            propagation_step.maneuver.instruction =
                TurnInstruction::NO_TURN(); // mark intermediate instructions invalid
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
                        const std::size_t step_index)
{
    auto &step = steps[step_index];
    step.maneuver.exit += 1;
    if (!on_roundabout)
    {

        // We reached a special case that requires the addition of a special route step in the
        // beginning. We started in a roundabout, so to announce the exit, we move use the exit
        // instruction and move it right to the beginning to make sure to immediately announce the
        // exit.
        BOOST_ASSERT(leavesRoundabout(steps[1].maneuver.instruction) ||
                     steps[1].maneuver.instruction.type == TurnType::StayOnRoundabout);
        steps[0].geometry_end = 1;
        steps[1] = forwardInto(steps[1], steps[0]);
        steps[0].duration = 0;
        steps[0].distance = 0;
        const auto exitToEnter = [](const TurnType type) {
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
            steps[1].rotary_name = steps[0].name;
    }

    // Normal exit from the roundabout, or exit from a previously fixed roundabout. Propagate the
    // index back to the entering location and prepare the current silent set of instructions for
    // removal.
    std::vector<std::size_t> intermediate_steps;
    BOOST_ASSERT(!steps[step_index].intersections.empty());
    const auto exit_intersection = steps[step_index].intersections.back();
    const auto exit_bearing = exit_intersection.bearings[exit_intersection.out];
    if (step_index > 1)
    {
        // The very first route-step is head, so we cannot iterate past that one
        for (std::size_t propagation_index = step_index - 1; propagation_index > 0;
             --propagation_index)
        {
            auto &propagation_step = steps[propagation_index];
            propagation_step = forwardInto(propagation_step, steps[propagation_index + 1]);
            if (entersRoundabout(propagation_step.maneuver.instruction))
            {
                propagation_step.maneuver.exit = step.maneuver.exit;
                propagation_step.geometry_end = step.geometry_end;
                const auto entry_intersection = propagation_step.intersections.front();

                // remember rotary name
                if (propagation_step.maneuver.instruction.type == TurnType::EnterRotary ||
                    propagation_step.maneuver.instruction.type == TurnType::EnterRotaryAtExit)
                {
                    propagation_step.rotary_name = propagation_step.name;
                }
                else if (propagation_step.maneuver.instruction.type ==
                             TurnType::EnterRoundaboutIntersection ||
                         propagation_step.maneuver.instruction.type ==
                             TurnType::EnterRoundaboutIntersectionAtExit)
                {
                    // Compute the angle between two bearings on a normal turn circle
                    //
                    //      Bearings                      Angles
                    //
                    //         0                           180
                    //   315         45               225       135
                    //
                    // 270     x       90           270     x      90
                    //
                    //   225        135               315        45
                    //        180                           0
                    //
                    // A turn from north to north-east offerst bearing 0 and 45 has to be translated
                    // into a turn of 135 degrees. The same holdes for 90 - 135 (east to south
                    // east).
                    // For north, the transformation works by angle = 540 (360 + 180) - exit_bearing
                    // % 360;
                    // All other cases are handled by first rotating both bearings to an
                    // entry_bearing of 0.
                    BOOST_ASSERT(!propagation_step.intersections.empty());
                    const double angle = [](const double entry_bearing, const double exit_bearing) {
                        const double offset = 360 - entry_bearing;
                        const double rotated_exit = [](double bearing, const double offset) {
                            bearing += offset;
                            return bearing > 360 ? bearing - 360 : bearing;
                        }(exit_bearing, offset);

                        const auto angle = 540 - rotated_exit;
                        return angle > 360 ? angle - 360 : angle;
                    }(util::bearing::reverseBearing(entry_intersection.bearings[entry_intersection.in]), exit_bearing);

                    propagation_step.maneuver.instruction.direction_modifier =
                        ::osrm::util::guidance::getTurnDirection(angle);
                }

                propagation_step.name = step.name;
                propagation_step.name_id = step.name_id;
                break;
            }
            else
            {
                BOOST_ASSERT(propagation_step.maneuver.instruction.type =
                                 TurnType::StayOnRoundabout);
                propagation_step.maneuver.instruction =
                    TurnInstruction::NO_TURN(); // mark intermediate instructions invalid
            }
        }
        // remove exit
        step.maneuver.instruction = TurnInstruction::NO_TURN();
    }
}

// elongate a step by another. the data is added either at the front, or the back
RouteStep elongate(RouteStep step, const RouteStep &by_step)
{
    BOOST_ASSERT(step.mode == by_step.mode);

    step.duration += by_step.duration;
    step.distance += by_step.distance;

    if (step.geometry_end == by_step.geometry_begin + 1)
    {
        step.geometry_end = by_step.geometry_end;

        // if we elongate in the back, we only need to copy the intersections to the beginning.
        // the bearings remain the same, as the location of the turn doesn't change
        step.intersections.insert(step.intersections.end(), by_step.intersections.begin(),
                                  by_step.intersections.end());
    }
    else
    {
        BOOST_ASSERT(step.maneuver.waypoint_type == WaypointType::None &&
                     by_step.maneuver.waypoint_type == WaypointType::None);
        BOOST_ASSERT(by_step.geometry_end == step.geometry_begin + 1);
        step.geometry_begin = by_step.geometry_begin;

        // elongating in the front changes the location of the maneuver
        step.maneuver = by_step.maneuver;

        step.intersections.insert(step.intersections.begin(), by_step.intersections.begin(),
                                  by_step.intersections.end());
    }
    return step;
}

// A check whether two instructions can be treated as one. This is only the case for very short
// maneuvers that can, in some form, be seen as one. The additional in_step is to find out about
// a possible u-turn.
bool collapsable(const RouteStep &step)
{
    const constexpr double MAX_COLLAPSE_DISTANCE = 25;
    return step.distance < MAX_COLLAPSE_DISTANCE;
}

void collapseTurnAt(std::vector<RouteStep> &steps,
                    const std::size_t two_back_index,
                    const std::size_t one_back_index,
                    const std::size_t step_index)
{
    BOOST_ASSERT(step_index < steps.size());
    BOOST_ASSERT(one_back_index < steps.size());
    const auto &current_step = steps[step_index];

    const auto &one_back_step = steps[one_back_index];

    const auto bearingsAreReversed = [](const double bearing_in, const double bearing_out) {
        // Nearly perfectly reversed angles have a difference close to 180 degrees (straight)
        return angularDeviation(bearing_in, bearing_out) > 170;
    };

    BOOST_ASSERT(!one_back_step.intersections.empty() && !current_step.intersections.empty());
    const auto isCollapsableInstruction = [](const TurnInstruction instruction) {
        return instruction.type == TurnType::NewName ||
               (instruction.type == TurnType::Turn &&
                instruction.direction_modifier == DirectionModifier::Straight);
    };
    // Very Short New Name
    if (isCollapsableInstruction(one_back_step.maneuver.instruction))
    {
        BOOST_ASSERT(two_back_index < steps.size());
        if (one_back_step.mode == steps[two_back_index].mode)
        {
            steps[two_back_index] = elongate(std::move(steps[two_back_index]), one_back_step);
            // If the previous instruction asked to continue, the name change will have to
            // be changed into a turn
            invalidateStep(steps[one_back_index]);

            if (TurnType::Continue == current_step.maneuver.instruction.type)
                steps[step_index].maneuver.instruction.type = TurnType::Turn;
        }
    }
    // very short segment after turn
    else if (isCollapsableInstruction(current_step.maneuver.instruction))
    {
        if (one_back_step.mode == current_step.mode)
        {
            steps[step_index] = elongate(std::move(steps[step_index]), steps[one_back_index]);
            invalidateStep(steps[one_back_index]);

            if (TurnType::Continue == current_step.maneuver.instruction.type)
            {
                steps[step_index].maneuver.instruction.type = TurnType::Turn;
            }
        }
    }
    // Potential U-Turn
    else if (bearingsAreReversed(util::bearing::reverseBearing(one_back_step.intersections.front().bearings[one_back_step.intersections.front().in]),
                                 current_step.intersections.front().bearings[current_step.intersections.front().out]))

    {
        BOOST_ASSERT(two_back_index < steps.size());
        // the simple case is a u-turn that changes directly into the in-name again
        const bool direct_u_turn = steps[two_back_index].name == current_step.name;

        // however, we might also deal with a dual-collapse scenario in which we have to
        // additionall collapse a name-change as well
        const bool continues_with_name_change =
            (step_index + 1 < steps.size()) &&
            isCollapsableInstruction(steps[step_index + 1].maneuver.instruction);
        const bool u_turn_with_name_change =
            collapsable(current_step) && continues_with_name_change &&
            steps[step_index + 1].name == steps[two_back_index].name;

        if (direct_u_turn || u_turn_with_name_change)
        {
            steps[one_back_index] = elongate(std::move(steps[one_back_index]), steps[step_index]);
            invalidateStep(steps[step_index]);
            if (u_turn_with_name_change)
            {
                steps[one_back_index] =
                    elongate(std::move(steps[one_back_index]), steps[step_index + 1]);
                invalidateStep(steps[step_index + 1]); // will be skipped due to the
                                                       // continue statement at the
                                                       // beginning of this function
            }

            steps[one_back_index].name = steps[two_back_index].name;
            steps[one_back_index].maneuver.instruction.type = TurnType::Continue;
            steps[one_back_index].maneuver.instruction.direction_modifier =
                DirectionModifier::UTurn;
        }
    }
}

} // namespace

// Post processing can invalidate some instructions. For example StayOnRoundabout
// is turned into exit counts. These instructions are removed by the following function

std::vector<RouteStep> removeNoTurnInstructions(std::vector<RouteStep> steps)
{
    // finally clean up the post-processed instructions.
    // Remove all invalid instructions from the set of instructions.
    // An instruction is invalid, if its NO_TURN and has WaypointType::None.
    // Two valid NO_TURNs exist in each leg in the form of Depart/Arrive

    // keep valid instructions
    const auto not_is_valid = [](const RouteStep &step) {
        return step.maneuver.instruction == TurnInstruction::NO_TURN() &&
               step.maneuver.waypoint_type == WaypointType::None;
    };

    boost::remove_erase_if(steps, not_is_valid);

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
    // In this case, exits are numbered from the start of the lag.
    std::size_t last_valid_instruction = 0;
    for (std::size_t step_index = 0; step_index < steps.size(); ++step_index)
    {
        auto &step = steps[step_index];
        const auto instruction = step.maneuver.instruction;
        if (entersRoundabout(instruction))
        {
            last_valid_instruction = step_index;
            has_entered_roundabout = setUpRoundabout(step);

            if (has_entered_roundabout && step_index + 1 < steps.size())
                steps[step_index + 1].maneuver.exit = step.maneuver.exit;
        }
        else if (instruction.type == TurnType::StayOnRoundabout)
        {
            on_roundabout = true;
            // increase the exit number we require passing the exit
            step.maneuver.exit += 1;
            if (step_index + 1 < steps.size())
                steps[step_index + 1].maneuver.exit = step.maneuver.exit;
        }
        else if (leavesRoundabout(instruction))
        {
            if (!has_entered_roundabout)
            {
                // in case the we are not on a roundabout, the very first instruction
                // after the depart will be transformed into a roundabout and become
                // the first valid instruction
                last_valid_instruction = 1;
            }
            closeOffRoundabout(has_entered_roundabout, steps, step_index);
            has_entered_roundabout = false;
            on_roundabout = false;
        }
        else if (instruction.type == TurnType::Suppressed)
        {
            // count intersections. We cannot use exit, since intersections can follow directly
            // after a roundabout
            steps[last_valid_instruction] = elongate(steps[last_valid_instruction], step);
            step.maneuver.instruction = TurnInstruction::NO_TURN();
        }
        else if (!isSilent(instruction))
        {
            // Remember the last non silent instruction
            last_valid_instruction = step_index;
        }
    }

    // unterminated roundabout
    // Move backwards through the instructions until the start and remove the exit number
    // A roundabout without exit translates to enter-roundabout.
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

// Post Processing to collapse unnecessary sets of combined instructions into a single one
std::vector<RouteStep> collapseTurns(std::vector<RouteStep> steps)
{
    if (steps.size() <= 2)
        return steps;

    // Get the previous non-invalid instruction
    const auto getPreviousIndex = [&steps](std::size_t index) {
        BOOST_ASSERT(index > 0);
        BOOST_ASSERT(index < steps.size());
        --index;
        while (index > 0 && steps[index].maneuver.instruction == TurnInstruction::NO_TURN())
            --index;

        return index;
    };

    // Check for an initial unwanted new-name
    {
        const auto &current_step = steps[1];
        if (TurnType::NewName == current_step.maneuver.instruction.type &&
            current_step.name == steps[0].name)
        {
            steps[0] = elongate(std::move(steps[0]), steps[1]);
            invalidateStep(steps[1]);
        }
    }
    const auto isCollapsableInstruction = [](const TurnInstruction instruction) {
        return instruction.type == TurnType::NewName ||
               (instruction.type == TurnType::Turn &&
                instruction.direction_modifier == DirectionModifier::Straight);
    };

    // first and last instructions are waypoints that cannot be collapsed
    for (std::size_t step_index = 2; step_index < steps.size(); ++step_index)
    {
        const auto &current_step = steps[step_index];
        const auto one_back_index = getPreviousIndex(step_index);
        BOOST_ASSERT(one_back_index < steps.size());

        // cannot collapse the depart instruction
        if (one_back_index == 0 || current_step.maneuver.instruction == TurnInstruction::NO_TURN())
            continue;

        const auto &one_back_step = steps[one_back_index];
        const auto two_back_index = getPreviousIndex(one_back_index);
        BOOST_ASSERT(two_back_index < steps.size());

        // Due to empty segments, we can get name-changes from A->A
        // These have to be handled in post-processing
        if (isCollapsableInstruction(current_step.maneuver.instruction) &&
            current_step.name == steps[one_back_index].name)
        {
            steps[one_back_index] = elongate(std::move(steps[one_back_index]), steps[step_index]);
            invalidateStep(steps[step_index]);
        }
        // If we look at two consecutive name changes, we can check for a name oszillation.
        // A name oszillation changes from name A shortly to name B and back to A.
        // In these cases, the name change will be suppressed.
        else if (isCollapsableInstruction(current_step.maneuver.instruction) &&
                 isCollapsableInstruction(one_back_step.maneuver.instruction))
        {
            // valid due to step_index starting at 2
            const auto &coming_from_name = steps[two_back_index].name;
            if (current_step.name == coming_from_name)
            {
                if (current_step.mode == one_back_step.mode &&
                    one_back_step.mode == steps[two_back_index].mode)
                {
                    steps[two_back_index] =
                        elongate(elongate(std::move(steps[two_back_index]), steps[one_back_index]),
                                 steps[step_index]);
                    invalidateStep(steps[one_back_index]);
                    invalidateStep(steps[step_index]);
                }
                // TODO discuss: we could think about changing the new-name to a pure notification
                // about mode changes
            }
        }
        else if (collapsable(one_back_step))
        {
            // check for one of the multiple collapse scenarios and, if possible, collapse the turn
            collapseTurnAt(steps, two_back_index, one_back_index, step_index);
        }
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
        // fixup the coordinate
        geometry.locations.erase(geometry.locations.begin());
        geometry.annotations.erase(geometry.annotations.begin());

        // remove the initial distance value
        geometry.segment_distances.erase(geometry.segment_distances.begin());

        // We have to adjust the first step both for its name and the bearings
        if (zero_length_step)
        {
            // move offsets to front
            BOOST_ASSERT(geometry.segment_offsets[1] == 1);
            // geometry offsets have to be adjusted. Move all offsets to the front and reduce by
            // one. (This is an inplace forward one and reduce by one)
            std::transform(geometry.segment_offsets.begin() + 1, geometry.segment_offsets.end(),
                           geometry.segment_offsets.begin(),
                           [](const std::size_t val) { return val - 1; });

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
            auto& first_intersection = designated_depart.intersections.front();
            first_intersection.bearings = {first_intersection.bearings[first_intersection.out]};
            first_intersection.entry = {true};
            first_intersection.in = Intersection::NO_INDEX;
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
            std::transform(geometry.segment_offsets.begin(), geometry.segment_offsets.end(),
                           geometry.segment_offsets.begin(),
                           [](const std::size_t val) { return val - 1; });
        }

        // and update the leg geometry indices for the removed entry
        std::for_each(steps.begin(), steps.end(), [](RouteStep &step) {
            --step.geometry_begin;
            --step.geometry_end;
        });

        auto& first_step = steps.front();
        // we changed the geometry, we need to recalculate the bearing
        auto bearing = std::round(util::coordinate_calculation::bearing(
            geometry.locations[first_step.geometry_begin],
            geometry.locations[first_step.geometry_begin+1]));
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
    if (next_to_last_step.distance <= 1)
    {
        geometry.locations.pop_back();
        geometry.annotations.pop_back();
        geometry.segment_offsets.pop_back();
        BOOST_ASSERT(geometry.segment_distances.back() < 1);
        geometry.segment_distances.pop_back();

        next_to_last_step.maneuver.waypoint_type = WaypointType::Arrive;
        next_to_last_step.maneuver.instruction = TurnInstruction::NO_TURN();
        next_to_last_step.maneuver.bearing_after = 0;
        BOOST_ASSERT(next_to_last_step.intersections.size() == 1);
        auto& last_intersection = next_to_last_step.intersections.back();
        last_intersection.bearings = {last_intersection.bearings[last_intersection.in]};
        last_intersection.entry = {true};
        last_intersection.out = Intersection::NO_INDEX;
        last_intersection.in = 0;
        steps.pop_back();

        // Because we eliminated a really short segment, it was probably
        // near an intersection.  The convention is *not* to make the
        // turn, so the `arrive` instruction should be on the same road
        // as the segment before it.  Thus, we have to copy the names
        // and travel modes from the new next_to_last step.
        auto &new_next_to_last = *(steps.end() - 2);
        next_to_last_step.name = new_next_to_last.name;
        next_to_last_step.name_id = new_next_to_last.name_id;
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
        geometry.segment_offsets.back()--;
        // since the last geometry includes the location of arrival, the arrival instruction
        // geometry overlaps with the previous segment
        BOOST_ASSERT(next_to_last_step.geometry_end == steps.back().geometry_begin + 1);
        BOOST_ASSERT(next_to_last_step.geometry_begin < next_to_last_step.geometry_end);
        next_to_last_step.geometry_end--;
        auto& last_step = steps.back();
        last_step.geometry_begin--;
        last_step.geometry_end--;
        BOOST_ASSERT(next_to_last_step.geometry_end == last_step.geometry_begin + 1);
        BOOST_ASSERT(last_step.geometry_begin == last_step.geometry_end-1);
        BOOST_ASSERT(next_to_last_step.geometry_end >= 2);
        // we changed the geometry, we need to recalculate the bearing
        auto bearing = std::round(util::coordinate_calculation::bearing(
            geometry.locations[next_to_last_step.geometry_end - 2],
            geometry.locations[last_step.geometry_begin]));
        last_step.maneuver.bearing_before = bearing;
        last_step.intersections.front().bearings.front() = util::bearing::reverseBearing(bearing);
    }

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
            ? angleToDirectionModifier(util::coordinate_calculation::computeAngle(
                  source_node.input_location, leg_geometry.locations[0], leg_geometry.locations[1]))
            : extractor::guidance::DirectionModifier::UTurn;

    steps.front().maneuver.instruction.direction_modifier = initial_modifier;

    const auto distance_from_end = util::coordinate_calculation::haversineDistance(
        target_node.input_location, leg_geometry.locations.back());
    const auto final_modifier =
        distance_from_end >= MINIMAL_RELATIVE_DISTANCE &&
                distance_from_end <= MAXIMAL_RELATIVE_DISTANCE
            ? angleToDirectionModifier(util::coordinate_calculation::computeAngle(
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

} // namespace guidance
} // namespace engine
} // namespace osrm
