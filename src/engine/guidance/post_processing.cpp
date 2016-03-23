#include "engine/guidance/post_processing.hpp"
#include "extractor/guidance/turn_instruction.hpp"

#include "engine/guidance/toolkit.hpp"

#include <boost/assert.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include <iostream>
#include <cstddef>
#include <utility>

using TurnInstruction = osrm::extractor::guidance::TurnInstruction;
using TurnType = osrm::extractor::guidance::TurnType;
using DirectionModifier = osrm::extractor::guidance::DirectionModifier;

namespace osrm
{
namespace engine
{
namespace guidance
{

namespace detail
{
bool canMergeTrivially(const RouteStep &destination, const RouteStep &source)
{
    return destination.maneuver.exit == 0 && destination.name_id == source.name_id &&
           isSilent(source.maneuver.instruction);
}

RouteStep forwardInto(RouteStep destination, const RouteStep &source)
{
    // Merge a turn into a silent turn
    // Overwrites turn instruction and increases exit NR
    destination.duration += source.duration;
    destination.distance += source.distance;
    destination.geometry_begin = std::min(destination.geometry_begin, source.geometry_begin);
    destination.geometry_end = std::max(destination.geometry_end, source.geometry_end);
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
            break;
        }
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
        instruction.type == TurnType::EnterRoundaboutAtExit)
    {
        step.maneuver.exit = 1;
        // prevent futher special case handling of these two.
        if (instruction.type == TurnType::EnterRotaryAtExit)
            step.maneuver.instruction = TurnType::EnterRotary;
        else
            step.maneuver.instruction = TurnType::EnterRoundabout;
    }

    if (leavesRoundabout(instruction))
    {
        step.maneuver.exit = 1; // count the otherwise missing exit
        if (instruction.type == TurnType::EnterRotaryAtExit)
            step.maneuver.instruction = TurnType::EnterRotary;
        else
            step.maneuver.instruction = TurnType::EnterRoundabout;
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

        // We reached a special case that requires the addition of a special route step in
        // the beginning.
        // We started in a roundabout, so to announce the exit, we move use the exit
        // instruction and
        // move it right to the beginning to make sure to immediately announce the exit.
        BOOST_ASSERT(leavesRoundabout(steps[1].maneuver.instruction) ||
                     steps[1].maneuver.instruction.type == TurnType::StayOnRoundabout);
        steps[0].geometry_end = 1;
        steps[1] = detail::forwardInto(steps[1], steps[0]);
        steps[0].duration = 0;
        steps[0].distance = 0;
        steps[1].maneuver.instruction.type = step.maneuver.instruction.type == TurnType::ExitRotary
                                                 ? TurnType::EnterRotary
                                                 : TurnType::EnterRoundabout;
    }

    // Normal exit from the roundabout, or exit from a previously fixed roundabout.
    // Propagate the index back to the entering
    // location and
    // prepare the current silent set of instructions for removal.
    if (step_index > 1)
    {
        // The very first route-step is head, so we cannot iterate past that one
        for (std::size_t propagation_index = step_index - 1; propagation_index > 0;
             --propagation_index)
        {
            auto &propagation_step = steps[propagation_index];
            propagation_step = detail::forwardInto(propagation_step, steps[propagation_index + 1]);
            if (entersRoundabout(propagation_step.maneuver.instruction))
            {
                // TODO at this point, we can remember the additional name for a rotary
                // This requires some initial thought on the data format, though
                propagation_step.maneuver.exit = step.maneuver.exit;
                propagation_step.geometry_end = step.geometry_end;
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
} // namespace detail

void print(const std::vector<RouteStep> &steps)
{
    std::cout << "Path\n";
    int segment = 0;
    for (const auto &step : steps)
    {
        const auto type = static_cast<int>(step.maneuver.instruction.type);
        const auto modifier = static_cast<int>(step.maneuver.instruction.direction_modifier);

        std::cout << "\t[" << ++segment << "]: " << type << " " << modifier
                  << " Duration: " << step.duration << " Distance: " << step.distance
                  << " Geometry: " << step.geometry_begin << " " << step.geometry_end
                  << " exit: " << step.maneuver.exit
                  << " Intersections: " << step.maneuver.intersections.size() << " [";

        for (auto intersection : step.maneuver.intersections)
            std::cout << "(" << intersection.duration << " " << intersection.distance << ")";

        std::cout << "] name[" << step.name_id << "]: " << step.name << std::endl;
    }
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

#define OSRM_POST_PROCESSING_PRINT_DEBUG 0
#if OSRM_POST_PROCESSING_PRINT_DEBUG
    std::cout << "[POSTPROCESSING ITERATION]" << std::endl;
    std::cout << "Input\n";
    print(steps);
#endif
    // Count Street Exits forward
    bool on_roundabout = false;

    // adds an intersection to the initial route step
    // It includes the length of the last step, until the intersection
    // Also updates the length of the respective segment
    auto addIntersection =
        [](RouteStep into, const RouteStep &last_step, const RouteStep &intersection)
    {
        into.maneuver.intersections.push_back(
            {last_step.duration, last_step.distance, intersection.maneuver.location});

        return detail::forwardInto(std::move(into), intersection);
    };

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
            on_roundabout = detail::setUpRoundabout(step);
            if (on_roundabout && step_index + 1 < steps.size())
                steps[step_index + 1].maneuver.exit = step.maneuver.exit;
        }
        else if (instruction.type == TurnType::StayOnRoundabout)
        {
            // increase the exit number we require passing the exit
            step.maneuver.exit += 1;
            if (step_index + 1 < steps.size())
                steps[step_index + 1].maneuver.exit = step.maneuver.exit;
        }
        else if (leavesRoundabout(instruction))
        {
            if (!on_roundabout)
            {
                // in case the we are not on a roundabout, the very first instruction
                // after the depart will be transformed into a roundabout and become
                // the first valid instruction
                last_valid_instruction = 1;
            }
            detail::closeOffRoundabout(on_roundabout, steps, step_index);
            on_roundabout = false;
        }
        else if (instruction.type == TurnType::Suppressed)
        {
            // count intersections. We cannot use exit, since intersections can follow directly
            // after a roundabout
            steps[last_valid_instruction] = addIntersection(
                std::move(steps[last_valid_instruction]), steps[step_index - 1], step);
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
    if (on_roundabout)
    {
        detail::fixFinalRoundabout(steps);
    }

    // finally clean up the post-processed instructions.
    // Remove all invalid instructions from the set of instructions.
    // An instruction is invalid, if its NO_TURN and has WaypointType::None.
    // Two valid NO_TURNs exist in each leg in the form of Depart/Arrive

    // keep valid instructions
    const auto not_is_valid = [](const RouteStep &step)
    {
        return step.maneuver.instruction == TurnInstruction::NO_TURN() &&
               step.maneuver.waypoint_type == WaypointType::None;
    };

    boost::remove_erase_if(steps, not_is_valid);

#if OSRM_POST_PROCESSING_PRINT_DEBUG
    std::cout << "Merged\n";
    print(steps);
#endif
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

    //remove the data fromt the reached-target step again
    leg_geometry.segment_offsets.pop_back();
    leg_geometry.segment_distances.pop_back();

    return leg_geometry;
}

} // namespace guidance
} // namespace engine
} // namespace osrm
