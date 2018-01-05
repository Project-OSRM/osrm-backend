#ifndef OSRM_ENGINE_GUIDANCE_COLLAPSING_UTILITY_HPP_
#define OSRM_ENGINE_GUIDANCE_COLLAPSING_UTILITY_HPP_

#include "guidance/turn_instruction.hpp"
#include "engine/guidance/route_step.hpp"
#include "util/attributes.hpp"
#include "util/bearing.hpp"
#include "util/guidance/name_announcements.hpp"

#include <boost/range/algorithm_ext/erase.hpp>
#include <cstddef>

namespace osrm
{
namespace engine
{
namespace guidance
{

using RouteSteps = std::vector<RouteStep>;
using RouteStepIterator = typename RouteSteps::iterator;
const constexpr std::size_t MIN_END_OF_ROAD_INTERSECTIONS = std::size_t{2};
const constexpr double MAX_COLLAPSE_DISTANCE = 30.0;
// a bit larger than 100 to avoid oscillation in tests
const constexpr double NAME_SEGMENT_CUTOFF_LENGTH = 105.0;

// check if a step is completely without turn type
inline bool hasTurnType(const RouteStep &step)
{
    return step.maneuver.instruction.type != osrm::guidance::TurnType::NoTurn;
}
inline bool hasWaypointType(const RouteStep &step)
{
    return step.maneuver.waypoint_type != WaypointType::None;
}

// skip backwards through possibly disabled turns until we find a turn type (or the first step)
inline RouteStepIterator findPreviousTurn(RouteStepIterator current_step)
{
    BOOST_ASSERT(!hasWaypointType(*current_step));
    // find the first element preceeding the current step that has an actual turn type (not
    // necessarily announced)
    do
    {
        // safety to do this loop is asserted in collapseTurnInstructions
        --current_step;
    } while (!hasTurnType(*current_step) && !hasWaypointType(*current_step));
    return current_step;
}

// skip forwards over possible NoTurn entries (e.g. ferries) until we find the next instruction with
// a turn type
inline RouteStepIterator findNextTurn(RouteStepIterator current_step)
{
    BOOST_ASSERT(!hasWaypointType(*current_step));
    // find the first element preceeding the current step that has an actual turn type (not
    // necessarily announced)
    do
    {
        // safety to do this loop is asserted in collapseTurnInstructions
        ++current_step;
    } while (!hasTurnType(*current_step) && !hasWaypointType(*current_step));
    return current_step;
}

// alias for comparisons
inline bool hasTurnType(const RouteStep &step, const osrm::guidance::TurnType::Enum type)
{
    return type == step.maneuver.instruction.type;
}
// alias for comparisons
inline bool hasModifier(const RouteStep &step,
                        const osrm::guidance::DirectionModifier::Enum modifier)
{
    return modifier == step.maneuver.instruction.direction_modifier;
}
inline bool hasLanes(const RouteStep &step)
{
    return step.intersections.front().lanes.lanes_in_turn != 0;
}

// alias for detectors, gives the number of connected roads
inline std::size_t numberOfAvailableTurns(const RouteStep &step)
{
    return step.intersections.front().entry.size();
}
// alias for detectors, counts only the allowed turns
inline std::size_t numberOfAllowedTurns(const RouteStep &step)
{
    return std::count(
        step.intersections.front().entry.begin(), step.intersections.front().entry.end(), true);
}
// traffic lights are very specifically modelled. Sometimes we need to skip them. All checks need to
// fulfill:
inline bool isTrafficLightStep(const RouteStep &step)
{
    return hasTurnType(step, osrm::guidance::TurnType::Suppressed) &&
           numberOfAvailableTurns(step) == 2 && numberOfAllowedTurns(step) == 1;
}

// alias for readability
inline void setInstructionType(RouteStep &step, const osrm::guidance::TurnType::Enum type)
{
    step.maneuver.instruction.type = type;
}

// alias for readability
inline bool haveSameMode(const RouteStep &lhs, const RouteStep &rhs)
{
    return lhs.mode == rhs.mode;
}

// alias for readability
inline bool haveSameMode(const RouteStep &first, const RouteStep &second, const RouteStep &third)
{
    return haveSameMode(first, second) && haveSameMode(second, third);
}

// alias for readability
inline bool haveSameName(const RouteStep &lhs, const RouteStep &rhs)
{
    const auto has_name_or_ref = [](auto const &step) {
        return !step.name.empty() || !step.ref.empty();
    };

    // make sure empty is not involved
    if (!has_name_or_ref(lhs) || !has_name_or_ref(rhs))
    {
        return false;
    }

    // easy check to not go over the strings if not necessary
    else if (lhs.name_id == rhs.name_id)
        return true;

    // ok, bite the sour grape and check the strings already
    else
        return !util::guidance::requiresNameAnnounced(lhs.name,
                                                      lhs.ref,
                                                      lhs.pronunciation,
                                                      lhs.exits,
                                                      rhs.name,
                                                      rhs.ref,
                                                      rhs.pronunciation,
                                                      rhs.exits);
}

// alias for readability, both turn right | left
inline bool areSameSide(const RouteStep &lhs, const RouteStep &rhs)
{
    const auto is_left = [](const RouteStep &step) {
        return hasModifier(step, osrm::guidance::DirectionModifier::Straight) ||
               hasLeftModifier(step.maneuver.instruction);
    };

    const auto is_right = [](const RouteStep &step) {
        return hasModifier(step, osrm::guidance::DirectionModifier::Straight) ||
               hasRightModifier(step.maneuver.instruction);
    };

    return (is_left(lhs) && is_left(rhs)) || (is_right(lhs) && is_right(rhs));
}

// do this after invalidating any steps to compress the step array again
OSRM_ATTR_WARN_UNUSED
inline std::vector<RouteStep> removeNoTurnInstructions(std::vector<RouteStep> steps)
{
    // finally clean up the post-processed instructions.
    // Remove all invalid instructions from the set of instructions.
    // An instruction is invalid, if its NO_TURN and has WaypointType::None.
    // Two valid NO_TURNs exist in each leg in the form of Depart/Arrive

    // keep valid instructions
    const auto not_is_valid = [](const RouteStep &step) {
        return step.maneuver.instruction == osrm::guidance::TurnInstruction::NO_TURN() &&
               step.maneuver.waypoint_type == WaypointType::None;
    };

    boost::remove_erase_if(steps, not_is_valid);

    // the steps should still include depart and arrive at least
    BOOST_ASSERT(steps.size() >= 2);

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

inline double totalTurnAngle(const RouteStep &entry_step, const RouteStep &exit_step)
{
    if (entry_step.geometry_begin > exit_step.geometry_begin)
        return totalTurnAngle(exit_step, entry_step);

    const auto exit_intersection = exit_step.intersections.front();
    const auto entry_intersection = entry_step.intersections.front();
    if ((exit_intersection.out >= exit_intersection.bearings.size()) ||
        (entry_intersection.in >= entry_intersection.bearings.size()))
        return entry_intersection.bearings[entry_intersection.out];

    const auto exit_step_exit_bearing = exit_intersection.bearings[exit_intersection.out];
    const auto entry_step_entry_bearing =
        util::bearing::reverse(entry_intersection.bearings[entry_intersection.in]);

    const double total_angle =
        util::bearing::angleBetween(entry_step_entry_bearing, exit_step_exit_bearing);

    return total_angle;
}

} /* namespace guidance */
} /* namespace engine */
} /* namespace osrm */

#endif /* OSRM_ENGINE_GUIDANCE_COLLAPSING_UTILITY_HPP_ */
