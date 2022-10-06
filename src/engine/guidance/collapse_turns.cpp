#include "engine/guidance/collapse_turns.hpp"
#include "guidance/constants.hpp"
#include "guidance/turn_instruction.hpp"
#include "engine/guidance/collapse_scenario_detection.hpp"
#include "engine/guidance/collapsing_utility.hpp"
#include "util/bearing.hpp"
#include "util/guidance/name_announcements.hpp"

#include <cstddef>

#include <boost/assert.hpp>

namespace osrm
{
namespace engine
{
namespace guidance
{
using osrm::util::angularDeviation;
using namespace osrm::guidance;

namespace
{
const constexpr double MAX_COLLAPSE_DISTANCE = 30;

// find the combined turn angle for two turns. Not in all scenarios we can easily add both angles
// (e.g 90 degree left followed by 90 degree right would be no turn at all).
double findTotalTurnAngle(const RouteStep &entry_step, const RouteStep &exit_step)
{
    if (entry_step.geometry_begin > exit_step.geometry_begin)
        return findTotalTurnAngle(exit_step, entry_step);

    const auto exit_intersection = exit_step.intersections.front();
    const auto exit_step_exit_bearing = exit_intersection.bearings[exit_intersection.out];
    const auto exit_step_entry_bearing =
        util::bearing::reverse(exit_intersection.bearings[exit_intersection.in]);

    const auto entry_intersection = entry_step.intersections.front();
    const auto entry_step_entry_bearing =
        util::bearing::reverse(entry_intersection.bearings[entry_intersection.in]);
    const auto entry_step_exit_bearing = entry_intersection.bearings[entry_intersection.out];

    const auto exit_angle =
        util::bearing::angleBetween(exit_step_entry_bearing, exit_step_exit_bearing);
    const auto entry_angle =
        util::bearing::angleBetween(entry_step_entry_bearing, entry_step_exit_bearing);

    const double total_angle =
        util::bearing::angleBetween(entry_step_entry_bearing, exit_step_exit_bearing);

    // both angles are in the same direction, the total turn gets increased
    //
    // a ---- b
    //           `
    //              c
    //              |
    //              d
    //
    // Will be considered just like
    //
    // a -----b
    //        |
    //        c
    //        |
    //        d
    const auto use_total_angle = [&]() {
        // only consider actual turns in combination:
        if (angularDeviation(total_angle, 180) < 0.5 * NARROW_TURN_ANGLE)
            return false;

        // entry step is short and the exit and the exit step does not have intersections??
        if (entry_step.distance < MAX_COLLAPSE_DISTANCE)
            return true;

        // both go roughly in the same direction
        if ((entry_angle <= 185 && exit_angle <= 185) || (entry_angle >= 175 && exit_angle >= 175))
            return true;

        return false;
    }();

    // We allow for minor deviations from a straight line
    if (use_total_angle)
    {
        return total_angle;
    }
    else
    {
        // to prevent ignoring angles like
        //
        // a -- b
        //      |
        //      c -- d
        //
        // We don't combine both turn angles here but keep the very first turn angle.
        // We choose the first one, since we consider the first maneuver in a merge range the
        // important one
        return entry_angle;
    }
}

inline void handleSliproad(RouteStepIterator sliproad_step)
{
    // find the next step after the sliproad step itself (this is not necessarily the next step,
    // since we might have to skip over traffic lights/node penalties)
    auto next_step = [&sliproad_step]() {
        auto next_step = findNextTurn(sliproad_step);
        while (isTrafficLightStep(*next_step))
        {
            // in sliproad checks, we should have made sure not to include invalid modes
            BOOST_ASSERT(haveSameMode(*sliproad_step, *next_step));
            sliproad_step->ElongateBy(*next_step);
            next_step->Invalidate();
            next_step = findNextTurn(next_step);
        }
        BOOST_ASSERT(haveSameMode(*sliproad_step, *next_step));
        return next_step;
    }();

    // have we reached the end?
    if (hasWaypointType(*next_step))
    {
        setInstructionType(*sliproad_step, TurnType::Turn);
    }
    else
    {
        const auto previous_step = findPreviousTurn(sliproad_step);
        const auto connecting_same_name_roads = haveSameName(*previous_step, *next_step);
        auto sliproad_turn_type = connecting_same_name_roads ? TurnType::Continue : TurnType::Turn;
        setInstructionType(*sliproad_step, sliproad_turn_type);
        combineRouteSteps(*sliproad_step,
                          *next_step,
                          AdjustToCombinedTurnAngleStrategy(),
                          TransferSignageStrategy(),
                          TransferLanesStrategy());
    }
}

} // namespace

// STRATEGIES

// keep signage/other entries in route step intact
void NoModificationStrategy::operator()(RouteStep &, const RouteStep &) const
{
    // actually do nothing.
}

// transfer turn type from a different turn
void TransferTurnTypeStrategy::operator()(RouteStep &step_at_turn_location,
                                          const RouteStep &transfer_from_step) const
{
    step_at_turn_location.maneuver = transfer_from_step.maneuver;
}

void AdjustToCombinedTurnAngleStrategy::operator()(RouteStep &step_at_turn_location,
                                                   const RouteStep &transfer_from_step) const
{
    // Forks point to left/right. By doing a combined angle, we would risk ending up with
    // unreasonable fork instrucitons. The direction of a fork only depends on the forking location,
    // not further angles coming up
    //
    //          d
    //       .  c
    // a - b
    //
    // could end up as `fork left` for `a-b-c`, instead of fork-right
    if (hasTurnType(step_at_turn_location, TurnType::Fork))
        return;

    // TODO assert transfer_from_step == step_at_turn_location + 1
    const auto angle = findTotalTurnAngle(step_at_turn_location, transfer_from_step);
    step_at_turn_location.maneuver.instruction.direction_modifier = getTurnDirection(angle);
}

AdjustToCombinedTurnStrategy::AdjustToCombinedTurnStrategy(
    const RouteStep &step_prior_to_intersection)
    : step_prior_to_intersection(step_prior_to_intersection)
{
}

void AdjustToCombinedTurnStrategy::operator()(RouteStep &step_at_turn_location,
                                              const RouteStep &transfer_from_step) const
{
    const auto angle = findTotalTurnAngle(step_at_turn_location, transfer_from_step);

    // Forks and merges point to left/right. By doing a combined angle, we would risk ending up with
    // unreasonable fork instrucitons. The direction of a fork or a merge only depends on the
    // location,
    // not further angles coming up
    //
    //          d
    //       .  c
    // a - b
    //
    // could end up as `fork left` for `a-b-c`, instead of fork-right
    const auto new_modifier = hasTurnType(step_at_turn_location, TurnType::Fork) ||
                                      hasTurnType(step_at_turn_location, TurnType::Merge)
                                  ? step_at_turn_location.maneuver.instruction.direction_modifier
                                  : getTurnDirection(angle);

    // a turn that is a new name or straight (turn/continue)
    const auto is_non_turn = [](const RouteStep &step) {
        return hasTurnType(step, TurnType::NewName) ||
               (hasTurnType(step, TurnType::Turn) &&
                hasModifier(step, DirectionModifier::Straight)) ||
               (hasTurnType(step, TurnType::Continue) &&
                hasModifier(step, DirectionModifier::Straight));
    };

    // check if the first part is the actual turn
    const auto transferring_from_non_turn = is_non_turn(transfer_from_step);

    // or if the maneuver location does not perform an actual turn
    const auto maneuver_at_non_turn = is_non_turn(step_at_turn_location) ||
                                      hasTurnType(step_at_turn_location, TurnType::Suppressed);

    // creating turns if the original instrution wouldn't be a maneuver (also for turn straights)`
    if (transferring_from_non_turn || maneuver_at_non_turn)
    {
        if (hasTurnType(step_at_turn_location, TurnType::Suppressed))
        {
            if (new_modifier == DirectionModifier::Straight)
            {
                setInstructionType(step_at_turn_location, TurnType::NewName);
            }
            else
            {
                step_at_turn_location.maneuver.instruction.type =
                    haveSameName(step_prior_to_intersection, transfer_from_step)
                        ? TurnType::Continue
                        : TurnType::Turn;
            }
        }
        else if (hasTurnType(step_at_turn_location, TurnType::NewName) &&
                 hasTurnType(transfer_from_step, TurnType::Suppressed) &&
                 new_modifier != DirectionModifier::Straight)
        {
            setInstructionType(step_at_turn_location, TurnType::Turn);
        }
        else if (hasTurnType(step_at_turn_location, TurnType::Continue) &&
                 !haveSameName(step_prior_to_intersection, transfer_from_step))
        {
            setInstructionType(step_at_turn_location, TurnType::Turn);
        }
        else if (hasTurnType(step_at_turn_location, TurnType::Turn) &&
                 !hasTurnType(transfer_from_step, TurnType::Suppressed) &&
                 haveSameName(step_prior_to_intersection, transfer_from_step))
        {
            setInstructionType(step_at_turn_location, TurnType::Continue);
        }
    }
    // if we are turning onto a ramp, we carry the ramp (e.g. a turn onto a ramp that is modelled
    // later only)
    else if (hasTurnType(transfer_from_step, TurnType::OnRamp))
    {
        setInstructionType(step_at_turn_location, TurnType::OnRamp);
    }
    // switch two turns to a single continue, if necessary
    else if (hasTurnType(step_at_turn_location, TurnType::Turn) &&
             hasTurnType(transfer_from_step, TurnType::Turn) &&
             haveSameName(step_prior_to_intersection, transfer_from_step))
    {
        setInstructionType(step_at_turn_location, TurnType::Continue);
    }
    // switch continue to turn, if possible
    else if (hasTurnType(step_at_turn_location, TurnType::Continue) &&
             hasTurnType(transfer_from_step, TurnType::Turn) &&
             !haveSameName(step_prior_to_intersection, transfer_from_step))
    {
        setInstructionType(step_at_turn_location, TurnType::Turn);
    }

    // finally set our new modifier
    step_at_turn_location.maneuver.instruction.direction_modifier = new_modifier;
}

StaggeredTurnStrategy::StaggeredTurnStrategy(const RouteStep &step_prior_to_intersection)
    : step_prior_to_intersection(step_prior_to_intersection)
{
}

void StaggeredTurnStrategy::operator()(RouteStep &step_at_turn_location,
                                       const RouteStep &transfer_from_step) const
{
    step_at_turn_location.maneuver.instruction.direction_modifier = DirectionModifier::Straight;
    step_at_turn_location.maneuver.instruction.type =
        haveSameName(step_prior_to_intersection, transfer_from_step) ? TurnType::Suppressed
                                                                     : TurnType::NewName;
}

void CombineSegregatedStepsStrategy::operator()(RouteStep &step_at_turn_location,
                                                const RouteStep &transfer_from_step) const
{
    // Handle end of road
    if (hasTurnType(step_at_turn_location, TurnType::EndOfRoad) ||
        hasTurnType(transfer_from_step, TurnType::EndOfRoad))
    {
        setInstructionType(step_at_turn_location, TurnType::EndOfRoad);
    }
}

SegregatedTurnStrategy::SegregatedTurnStrategy(const RouteStep &step_prior_to_intersection)
    : step_prior_to_intersection(step_prior_to_intersection)
{
}

void SegregatedTurnStrategy::operator()(RouteStep &step_at_turn_location,
                                        const RouteStep &transfer_from_step) const
{
    // Used to control updating of the modifier based on turn direction
    bool update_modifier_for_turn_direction = true;

    const auto calculate_turn_angle = [](const RouteStep &entry_step, const RouteStep &exit_step) {
        return util::bearing::angleBetween(entry_step.maneuver.bearing_before,
                                           exit_step.maneuver.bearing_after);
    };

    // Calculate turn angle and direction for segregated
    const auto turn_angle = calculate_turn_angle(step_at_turn_location, transfer_from_step);
    const auto turn_direction = getTurnDirection(turn_angle);

    const auto is_straight_step = [](const RouteStep &step) {
        return ((hasTurnType(step, TurnType::NewName) || hasTurnType(step, TurnType::Continue) ||
                 hasTurnType(step, TurnType::Suppressed) || hasTurnType(step, TurnType::Turn)) &&
                (hasModifier(step, DirectionModifier::Straight) ||
                 hasModifier(step, DirectionModifier::SlightLeft) ||
                 hasModifier(step, DirectionModifier::SlightRight)));
    };

    const auto is_turn_step = [](const RouteStep &step) {
        return (hasTurnType(step, TurnType::Turn) || hasTurnType(step, TurnType::Continue) ||
                hasTurnType(step, TurnType::NewName) || hasTurnType(step, TurnType::Suppressed));
    };

    // Process end of road step
    if (hasTurnType(step_at_turn_location, TurnType::EndOfRoad) ||
        hasTurnType(transfer_from_step, TurnType::EndOfRoad))
    {
        // Keep end of road
        setInstructionType(step_at_turn_location, TurnType::EndOfRoad);
    }
    // Process fork step at turn
    else if (hasTurnType(step_at_turn_location, TurnType::Fork))
    {
        // Do not update modifier based on turn direction
        update_modifier_for_turn_direction = false;
    }
    // Process straight step
    else if ((turn_direction == guidance::DirectionModifier::Straight) &&
             is_straight_step(transfer_from_step))
    {
        // Determine if continue or new name
        setInstructionType(step_at_turn_location,
                           (haveSameName(step_prior_to_intersection, transfer_from_step)
                                ? TurnType::Suppressed
                                : TurnType::NewName));
    }
    // Process wider straight step
    else if (isWiderStraight(turn_angle) && hasSingleIntersection(step_at_turn_location) &&
             hasStraightestTurn(step_at_turn_location) && hasStraightestTurn(transfer_from_step))
    {
        // Determine if continue or new name
        setInstructionType(step_at_turn_location,
                           (haveSameName(step_prior_to_intersection, transfer_from_step)
                                ? TurnType::Suppressed
                                : TurnType::NewName));

        // Set modifier to straight
        setModifier(step_at_turn_location, osrm::guidance::DirectionModifier::Straight);

        // Do not update modifier based on turn direction
        update_modifier_for_turn_direction = false;
    }
    // Process turn step
    else if ((turn_direction != guidance::DirectionModifier::Straight) &&
             is_turn_step(transfer_from_step))
    {
        // Mark as turn
        setInstructionType(step_at_turn_location, TurnType::Turn);
    }
    // Process the others not covered above by using the transfer step turn type
    else
    {
        // Set type from transfer step
        setInstructionType(step_at_turn_location, transfer_from_step.maneuver.instruction.type);
    }

    // Update modifier based on turn direction, if needed
    if (update_modifier_for_turn_direction)
    {
        setModifier(step_at_turn_location, turn_direction);
    }
}

SetFixedInstructionStrategy::SetFixedInstructionStrategy(const TurnInstruction instruction)
    : instruction(instruction)
{
}

void SetFixedInstructionStrategy::operator()(RouteStep &step_at_turn_location,
                                             const RouteStep &) const
{
    step_at_turn_location.maneuver.instruction = instruction;
}

void TransferSignageStrategy::operator()(RouteStep &step_at_turn_location,
                                         const RouteStep &transfer_from_step) const
{
    step_at_turn_location.AdaptStepSignage(transfer_from_step);
    step_at_turn_location.rotary_name = transfer_from_step.rotary_name;
    step_at_turn_location.rotary_pronunciation = transfer_from_step.rotary_pronunciation;
}

void TransferLanesStrategy::operator()(RouteStep &step_at_turn_location,
                                       const RouteStep &transfer_from_step) const
{
    step_at_turn_location.intersections.front().lanes =
        transfer_from_step.intersections.front().lanes;
    step_at_turn_location.intersections.front().lane_description =
        transfer_from_step.intersections.front().lane_description;
}

void suppressStep(RouteStep &step_at_turn_location, RouteStep &step_after_turn_location)
{
    return combineRouteSteps(step_at_turn_location,
                             step_after_turn_location,
                             NoModificationStrategy(),
                             NoModificationStrategy(),
                             NoModificationStrategy());
}

// OTHER IMPLEMENTATIONS
OSRM_ATTR_WARN_UNUSED
RouteSteps collapseTurnInstructions(RouteSteps steps)
{
    // make sure we can safely iterate over all steps (has depart/arrive with TurnType::NoTurn)
    BOOST_ASSERT(!hasTurnType(steps.front()) && !hasTurnType(steps.back()));
    BOOST_ASSERT(hasWaypointType(steps.front()) && hasWaypointType(steps.back()));

    if (steps.size() <= 2)
        return steps;

    // start of with no-op
    for (auto current_step = steps.begin() + 1; current_step + 1 != steps.end(); ++current_step)
    {
        if (entersRoundabout(current_step->maneuver.instruction) ||
            staysOnRoundabout(current_step->maneuver.instruction))
        {
            // If postProcess is called before then all corresponding leavesRoundabout steps are
            // removed and the current roundabout step can be ignored by directly proceeding to
            // the next step.
            // If postProcess is not called before then all steps till the next leavesRoundabout
            // step must be skipped to prevent incorrect roundabouts post-processing.

            // are we done for good?
            if (current_step + 1 == steps.end())
                break;
            else
                continue;
        }

        // only operate on actual turns
        if (!hasTurnType(*current_step))
            continue;

        // handle all situations involving the sliproad turn type
        if (hasTurnType(*current_step, TurnType::Sliproad))
        {
            handleSliproad(current_step);
            continue;
        }

        // don't collapse next step if it is a waypoint alread
        const auto next_step = findNextTurn(current_step);
        if (hasWaypointType(*next_step))
            break;

        const auto previous_step = findPreviousTurn(current_step);

        // don't collapse anything that does change modes
        if (current_step->mode != next_step->mode)
            continue;

        // handle staggered intersections:
        // a staggered intersection describes to turns in rapid succession that go in opposite
        // directions (e.g. right + left) with a very short segment in between
        if (isStaggeredIntersection(previous_step, current_step, next_step))
        {
            combineRouteSteps(*current_step,
                              *next_step,
                              StaggeredTurnStrategy(*previous_step),
                              TransferSignageStrategy(),
                              NoModificationStrategy());
        }
        else if (isUTurn(previous_step, current_step, next_step))
        {
            combineRouteSteps(
                *current_step,
                *next_step,
                SetFixedInstructionStrategy({TurnType::Continue, DirectionModifier::UTurn}),
                TransferSignageStrategy(),
                NoModificationStrategy());
        }
        else if (isNameOszillation(previous_step, current_step, next_step))
        {
            // first deactivate the second name switch
            suppressStep(*current_step, *next_step);
            // and then the first (to ensure both iterators to be valid)
            suppressStep(*previous_step, *current_step);
        }
        else if (maneuverPreceededByNameChange(previous_step, current_step, next_step) ||
                 maneuverPreceededBySuppressedDirection(current_step, next_step))
        {
            const auto strategy = AdjustToCombinedTurnStrategy(*previous_step);
            strategy(*next_step, *current_step);
            // suppress previous step
            suppressStep(*previous_step, *current_step);
        }
        else if (maneuverSucceededByNameChange(current_step, next_step) ||
                 nameChangeImmediatelyAfterSuppressed(current_step, next_step) ||
                 maneuverSucceededBySuppressedDirection(current_step, next_step) ||
                 closeChoicelessTurnAfterTurn(current_step, next_step))
        {
            combineRouteSteps(*current_step,
                              *next_step,
                              AdjustToCombinedTurnStrategy(*previous_step),
                              TransferSignageStrategy(),
                              NoModificationStrategy());
        }
        else if (straightTurnFollowedByChoiceless(current_step, next_step))
        {
            combineRouteSteps(*current_step,
                              *next_step,
                              AdjustToCombinedTurnStrategy(*previous_step),
                              TransferSignageStrategy(),
                              NoModificationStrategy());
        }
        else if (suppressedStraightBetweenTurns(previous_step, current_step, next_step))
        {
            const auto far_back_step = findPreviousTurn(previous_step);
            previous_step->ElongateBy(*current_step);
            current_step->Invalidate();
            combineRouteSteps(*previous_step,
                              *next_step,
                              AdjustToCombinedTurnStrategy(*far_back_step),
                              TransferSignageStrategy(),
                              NoModificationStrategy());
        }

        // if the current collapsing triggers, we can check for advanced scenarios that only are
        // possible after an inital collapse step (e.g. name change right after a u-turn)
        //
        // f - e - d
        //     |   |
        // a - b - c
        //
        // In this scenario, bc and de might belong to a different road than a-b and f-e (since
        // there are no fix conventions how to label them in segregated intersections). These steps
        // might only become apparent after some initial collapsing
        const auto new_next_step = findNextTurn(current_step);
        if (doubleChoiceless(current_step, new_next_step))
        {
            combineRouteSteps(*current_step,
                              *new_next_step,
                              AdjustToCombinedTurnStrategy(*previous_step),
                              TransferSignageStrategy(),
                              NoModificationStrategy());
        }
        if (!hasWaypointType(*previous_step))
        {
            const auto far_back_step = findPreviousTurn(previous_step);
            // due to name changes, we can find u-turns a bit late. Thats why we check far back as
            // well
            if (isUTurn(far_back_step, previous_step, current_step))
            {
                combineRouteSteps(
                    *previous_step,
                    *current_step,
                    SetFixedInstructionStrategy({TurnType::Continue, DirectionModifier::UTurn}),
                    TransferSignageStrategy(),
                    NoModificationStrategy());
            }
        }
    }
    return steps;
}

// OTHER IMPLEMENTATIONS
OSRM_ATTR_WARN_UNUSED
RouteSteps collapseSegregatedTurnInstructions(RouteSteps steps)
{
    // make sure we can safely iterate over all steps (has depart/arrive with TurnType::NoTurn)
    BOOST_ASSERT(!hasTurnType(steps.front()) && !hasTurnType(steps.back()));
    BOOST_ASSERT(hasWaypointType(steps.front()) && hasWaypointType(steps.back()));

    if (steps.size() <= 2)
        return steps;

    auto curr_step = steps.begin() + 1;
    auto next_step = curr_step + 1;
    const auto last_step = steps.end() - 1;

    // Loop over steps to collapse the segregated intersections; ignore first and last step
    while (next_step != last_step)
    {
        const auto prev_step = findPreviousTurn(curr_step);

        // if current step and next step are both segregated then combine the steps with no turn
        // adjustment
        if (curr_step->is_segregated && next_step->is_segregated)
        {
            // Combine segregated steps
            combineRouteSteps(*curr_step,
                              *next_step,
                              CombineSegregatedStepsStrategy(),
                              TransferSignageStrategy(),
                              TransferLanesStrategy());
            ++next_step;
        }
        // else if the current step is segregated and the next step is not segregated
        // and the next step is not a roundabout then combine with turn adjustment
        else if (curr_step->is_segregated && !next_step->is_segregated &&
                 !hasRoundaboutType(curr_step->maneuver.instruction) &&
                 !hasRoundaboutType(next_step->maneuver.instruction))
        {
            // Determine if u-turn
            if (bearingsAreReversed(
                    util::bearing::reverse(curr_step->intersections.front()
                                               .bearings[curr_step->intersections.front().in]),
                    next_step->intersections.front()
                        .bearings[next_step->intersections.front().out]))
            {
                // Collapse segregated u-turn
                combineRouteSteps(
                    *curr_step,
                    *next_step,
                    SetFixedInstructionStrategy({TurnType::Continue, DirectionModifier::UTurn}),
                    TransferSignageStrategy(),
                    NoModificationStrategy());
            }
            else
            {
                // Collapse segregated turn
                combineRouteSteps(*curr_step,
                                  *next_step,
                                  SegregatedTurnStrategy(*prev_step),
                                  TransferSignageStrategy(),
                                  NoModificationStrategy());
            }

            // Segregated step has been removed
            curr_step->is_segregated = false;
            ++next_step;
        }
        // else next step
        else
        {
            curr_step = next_step;
            ++next_step;
        }
    }

    // Clean up steps
    steps = removeNoTurnInstructions(std::move(steps));

    return steps;
}

} // namespace guidance
} // namespace engine
} // namespace osrm
