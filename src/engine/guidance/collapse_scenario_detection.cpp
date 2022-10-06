#include "engine/guidance/collapse_scenario_detection.hpp"
#include "guidance/constants.hpp"
#include "util/bearing.hpp"

#include <numeric>

#include <boost/assert.hpp>

namespace osrm
{
namespace engine
{
namespace guidance
{
using namespace osrm::guidance;

namespace
{

// to collapse steps, we focus on short segments that don't interact with other roads. To collapse
// two instructions into one, we need to look at to instrutions immediately after each other.
bool noIntermediaryIntersections(const RouteStep &step)
{
    return std::all_of(step.intersections.begin() + 1,
                       step.intersections.end(),
                       [](const auto &intersection) { return intersection.entry.size() == 2; });
}

// Link roads, as far as we are concerned, are short unnamed segments between to named segments.
bool isLinkRoad(const RouteStep &pre_link_step,
                const RouteStep &link_step,
                const RouteStep &post_link_step)
{
    const constexpr double MAX_LINK_ROAD_LENGTH = 2 * MAX_COLLAPSE_DISTANCE;
    const auto is_short = link_step.distance <= MAX_LINK_ROAD_LENGTH;
    const auto unnamed = link_step.name.empty();
    const auto between_named = !pre_link_step.name.empty() && !post_link_step.name.empty();

    return is_short && unnamed && between_named && noIntermediaryIntersections(link_step);
}

// Just like a link step, but shorter and no name restrictions.
bool isShortAndUndisturbed(const RouteStep &step)
{
    const auto is_short = step.distance <= MAX_COLLAPSE_DISTANCE;
    return is_short && noIntermediaryIntersections(step);
}

// On dual carriageways, we might want to use u-turns in combination with new-name instructions.
// Otherwise a u-turn should never be part of a collapsing instructions.
bool noBadUTurnCombination(const RouteStepIterator first, const RouteStepIterator second)
{
    auto has_uturn = hasModifier(*first, DirectionModifier::UTurn) ||
                     hasModifier(*second, DirectionModifier::UTurn);

    auto const from_name_change_into_uturn =
        hasTurnType(*first, TurnType::NewName) && hasModifier(*second, DirectionModifier::UTurn);

    auto const uturn_into_name_change =
        hasTurnType(*second, TurnType::NewName) && hasModifier(*first, DirectionModifier::UTurn);

    return !has_uturn || from_name_change_into_uturn || uturn_into_name_change;
}

} // namespace

bool basicCollapsePreconditions(const RouteStepIterator first, const RouteStepIterator second)
{
    const auto has_roundabout_type = hasRoundaboutType(first->maneuver.instruction) ||
                                     hasRoundaboutType(second->maneuver.instruction);

    const auto waypoint_type = hasWaypointType(*first) || hasWaypointType(*second);

    const auto contains_bad_uturn = !noBadUTurnCombination(first, second);

    return !has_roundabout_type && !waypoint_type && haveSameMode(*first, *second) &&
           !contains_bad_uturn;
}

bool basicCollapsePreconditions(const RouteStepIterator first,
                                const RouteStepIterator second,
                                const RouteStepIterator third)
{
    const auto has_roundabout_type = hasRoundaboutType(first->maneuver.instruction) ||
                                     hasRoundaboutType(second->maneuver.instruction) ||
                                     hasRoundaboutType(third->maneuver.instruction);

    const auto contains_bad_uturn =
        !noBadUTurnCombination(first, second) && !noBadUTurnCombination(second, third);

    // require modes to match up
    return !has_roundabout_type && haveSameMode(*first, *second, *third) && !contains_bad_uturn;
}

bool isStaggeredIntersection(const RouteStepIterator step_prior_to_intersection,
                             const RouteStepIterator step_entering_intersection,
                             const RouteStepIterator step_leaving_intersection)
{
    BOOST_ASSERT(!hasWaypointType(*step_entering_intersection) &&
                 !(hasWaypointType(*step_leaving_intersection)));
    // don't touch roundabouts
    if (entersRoundabout(step_entering_intersection->maneuver.instruction) ||
        entersRoundabout(step_leaving_intersection->maneuver.instruction))
        return false;
    // Base decision on distance since the zig-zag is a visual clue.
    // If adjusted, make sure to check validity of the is_right/is_left classification below
    const constexpr auto MAX_STAGGERED_DISTANCE = 3; // debatable, but keep short to be on safe side

    const auto angle = [](const RouteStep &step) {
        const auto &intersection = step.intersections.front();
        const auto entry_bearing = util::bearing::reverse(intersection.bearings[intersection.in]);
        const auto exit_bearing = intersection.bearings[intersection.out];
        return util::bearing::angleBetween(entry_bearing, exit_bearing);
    };

    // Instead of using turn modifiers (e.g. as in isRightTurn) we want to be more strict here.
    // We do not want to trigger e.g. on sharp uturn'ish turns or going straight "turns".
    // Therefore we use the turn angle to derive 90 degree'ish right / left turns.
    // This more closely resembles what we understand as Staggered Intersection.
    // We have to be careful in cases with larger MAX_STAGGERED_DISTANCE values. If the distance
    // gets large, sharper angles might be not obvious enough to consider them a staggered
    // intersection. We might need to consider making the decision here dependent on the actual turn
    // angle taken. To do so, we could scale the angle-limits by a factor depending on the distance
    // between the turns.
    const auto is_right = [](const double angle) { return angle > 45 && angle < 135; };
    const auto is_left = [](const double angle) { return angle > 225 && angle < 315; };

    const auto left_right =
        is_left(angle(*step_entering_intersection)) && is_right(angle(*step_leaving_intersection));
    const auto right_left =
        is_right(angle(*step_entering_intersection)) && is_left(angle(*step_leaving_intersection));

    // A RouteStep holds distance/duration from the maneuver to the subsequent step.
    // We are only interested in the distance between the first and the second.
    const auto is_short = step_entering_intersection->distance < MAX_STAGGERED_DISTANCE;
    const auto intermediary_mode_change =
        step_prior_to_intersection->mode == step_leaving_intersection->mode &&
        step_entering_intersection->mode != step_leaving_intersection->mode;

    const auto mode_change_when_entering =
        step_prior_to_intersection->mode != step_entering_intersection->mode;

    // previous step maneuver intersections should be length 1 to indicate that
    // there are no intersections between the two potentially collapsible turns
    return is_short && (left_right || right_left) && !intermediary_mode_change &&
           !mode_change_when_entering && noIntermediaryIntersections(*step_entering_intersection);
}

bool isUTurn(const RouteStepIterator step_prior_to_intersection,
             const RouteStepIterator step_entering_intersection,
             const RouteStepIterator step_leaving_intersection)
{
    if (!basicCollapsePreconditions(
            step_prior_to_intersection, step_entering_intersection, step_leaving_intersection))
        return false;

    // uturns only allowed on turns
    if (!hasTurnType(*step_entering_intersection, TurnType::Turn) &&
        !hasTurnType(*step_entering_intersection, TurnType::Continue) &&
        !hasTurnType(*step_entering_intersection, TurnType::EndOfRoad))
        return false;

    // the most basic condition for a uturn is that we actually turn around
    const bool takes_u_turn = bearingsAreReversed(
        util::bearing::reverse(step_entering_intersection->intersections.front()
                                   .bearings[step_entering_intersection->intersections.front().in]),
        step_leaving_intersection->intersections.front()
            .bearings[step_leaving_intersection->intersections.front().out]);

    if (!takes_u_turn)
        return false;

    // TODO check for name match after additional step
    const auto names_match = haveSameName(*step_prior_to_intersection, *step_leaving_intersection);

    // names within a u-turn road have to match from entry step to exit step
    if (!names_match)
        return false;

    const auto collapsable = isShortAndUndisturbed(*step_entering_intersection);
    const auto only_allowed_turn = (numberOfAllowedTurns(*step_leaving_intersection) == 1) &&
                                   noIntermediaryIntersections(*step_entering_intersection);

    return collapsable ||
           isLinkRoad(*step_prior_to_intersection,
                      *step_entering_intersection,
                      *step_leaving_intersection) ||
           only_allowed_turn;
}

bool isNameOszillation(const RouteStepIterator step_prior_to_intersection,
                       const RouteStepIterator step_entering_intersection,
                       const RouteStepIterator step_leaving_intersection)
{
    if (!basicCollapsePreconditions(
            step_prior_to_intersection, step_entering_intersection, step_leaving_intersection))
        return false;

    const auto are_name_changes =
        (hasTurnType(*step_entering_intersection, TurnType::NewName) ||
         (hasTurnType(*step_entering_intersection, TurnType::Turn) &&
          hasModifier(*step_entering_intersection, DirectionModifier::Straight))) &&
        (hasTurnType(*step_leaving_intersection, TurnType::NewName) ||
         (hasTurnType(*step_leaving_intersection, TurnType::Suppressed) &&
          step_leaving_intersection->name_id == EMPTY_NAMEID) ||
         (hasTurnType(*step_leaving_intersection, TurnType::Turn) &&
          hasModifier(*step_leaving_intersection, DirectionModifier::Straight)));

    if (!are_name_changes)
        return false;

    const auto names_match =
        // accept empty names as well as same names
        step_prior_to_intersection->name_id == step_leaving_intersection->name_id ||
        haveSameName(*step_prior_to_intersection, *step_leaving_intersection);
    return names_match;
}

bool maneuverPreceededByNameChange(const RouteStepIterator step_prior_to_intersection,
                                   const RouteStepIterator step_entering_intersection,
                                   const RouteStepIterator step_leaving_intersection)
{
    if (!basicCollapsePreconditions(
            step_prior_to_intersection, step_entering_intersection, step_leaving_intersection))
        return false;

    const auto short_and_undisturbed = isShortAndUndisturbed(*step_entering_intersection);
    const auto is_name_change =
        hasTurnType(*step_entering_intersection, TurnType::NewName) ||
        ((hasTurnType(*step_entering_intersection, TurnType::Turn) ||
          hasTurnType(*step_entering_intersection, TurnType::Continue)) &&
         hasModifier(*step_entering_intersection, DirectionModifier::Straight));

    const auto followed_by_maneuver =
        hasTurnType(*step_leaving_intersection) &&
        !hasTurnType(*step_leaving_intersection, TurnType::Suppressed);

    return short_and_undisturbed && is_name_change && followed_by_maneuver;
}

bool maneuverPreceededBySuppressedDirection(const RouteStepIterator step_entering_intersection,
                                            const RouteStepIterator step_leaving_intersection)
{
    if (!basicCollapsePreconditions(step_entering_intersection, step_leaving_intersection))
        return false;

    const auto short_and_undisturbed = isShortAndUndisturbed(*step_entering_intersection);

    const auto is_suppressed_direction =
        hasTurnType(*step_entering_intersection, TurnType::Suppressed) &&
        !hasModifier(*step_entering_intersection, DirectionModifier::Straight);

    const auto followed_by_maneuver =
        hasTurnType(*step_leaving_intersection) &&
        !hasTurnType(*step_leaving_intersection, TurnType::Suppressed);

    const auto keeps_direction =
        areSameSide(*step_entering_intersection, *step_leaving_intersection);

    const auto has_choice = numberOfAllowedTurns(*step_entering_intersection) > 1;

    return short_and_undisturbed && has_choice && is_suppressed_direction && followed_by_maneuver &&
           keeps_direction;
}

bool suppressedStraightBetweenTurns(const RouteStepIterator step_entering_intersection,
                                    const RouteStepIterator step_at_center_of_intersection,
                                    const RouteStepIterator step_leaving_intersection)
{
    if (!basicCollapsePreconditions(
            step_entering_intersection, step_at_center_of_intersection, step_leaving_intersection))
        return false;

    const auto both_short_enough =
        step_entering_intersection->distance < 0.8 * MAX_COLLAPSE_DISTANCE &&
        step_at_center_of_intersection->distance < 0.8 * MAX_COLLAPSE_DISTANCE;

    const auto similar_length =
        (step_entering_intersection->distance < 5 &&
         step_at_center_of_intersection->distance < 5) ||
        std::min(step_entering_intersection->distance, step_at_center_of_intersection->distance) /
                std::max(step_entering_intersection->distance,
                         step_at_center_of_intersection->distance) >
            0.75;

    const auto correct_types =
        hasTurnType(*step_at_center_of_intersection, TurnType::Suppressed) &&
        hasModifier(*step_at_center_of_intersection, DirectionModifier::Straight) &&
        (hasTurnType(*step_entering_intersection, TurnType::Turn) ||
         hasTurnType(*step_entering_intersection, TurnType::Continue)) &&
        (hasTurnType(*step_leaving_intersection, TurnType::Turn) ||
         hasTurnType(*step_leaving_intersection, TurnType::Continue) ||
         hasTurnType(*step_leaving_intersection, TurnType::OnRamp));

    const auto total_angle =
        totalTurnAngle(*step_entering_intersection, *step_leaving_intersection);
    const auto total_angle_is_not_uturn =
        (total_angle > NARROW_TURN_ANGLE) && (total_angle < 360 - NARROW_TURN_ANGLE);

    return both_short_enough && similar_length && correct_types && total_angle_is_not_uturn;
}

bool maneuverSucceededByNameChange(const RouteStepIterator step_entering_intersection,
                                   const RouteStepIterator step_leaving_intersection)
{
    if (!basicCollapsePreconditions(step_entering_intersection, step_leaving_intersection))
        return false;

    const auto short_and_undisturbed = isShortAndUndisturbed(*step_entering_intersection);
    const auto followed_by_name_change =
        hasTurnType(*step_leaving_intersection, TurnType::NewName) ||
        ((hasTurnType(*step_leaving_intersection, TurnType::Turn) ||
          hasTurnType(*step_leaving_intersection, TurnType::Continue)) &&
         hasModifier(*step_leaving_intersection, DirectionModifier::Straight));

    const auto is_maneuver = hasTurnType(*step_entering_intersection) &&
                             !hasTurnType(*step_entering_intersection, TurnType::Suppressed);

    // a straight name change can overrule max collapse distance
    const auto is_strong_name_change =
        hasTurnType(*step_leaving_intersection, TurnType::NewName) &&
        hasModifier(*step_leaving_intersection, DirectionModifier::Straight) &&
        step_entering_intersection->distance <= 1.5 * MAX_COLLAPSE_DISTANCE;

    // also allow a bit more, if the new name is without choice
    const auto is_choiceless_name_change =
        hasTurnType(*step_leaving_intersection, TurnType::NewName) &&
        numberOfAllowedTurns(*step_leaving_intersection) == 1 &&
        step_entering_intersection->distance <= 1.5 * MAX_COLLAPSE_DISTANCE;

    return (short_and_undisturbed || is_strong_name_change || is_choiceless_name_change) &&
           followed_by_name_change && is_maneuver;
}

bool maneuverSucceededBySuppressedDirection(const RouteStepIterator step_entering_intersection,
                                            const RouteStepIterator step_leaving_intersection)
{
    if (!basicCollapsePreconditions(step_entering_intersection, step_leaving_intersection))
        return false;

    const auto short_and_undisturbed = isShortAndUndisturbed(*step_entering_intersection);

    const auto followed_by_suppressed_direction =
        hasTurnType(*step_leaving_intersection, TurnType::Suppressed) &&
        !hasModifier(*step_leaving_intersection, DirectionModifier::Straight);

    const auto is_maneuver = hasTurnType(*step_entering_intersection) &&
                             !hasTurnType(*step_entering_intersection, TurnType::Suppressed);

    const auto keeps_direction =
        areSameSide(*step_entering_intersection, *step_leaving_intersection);

    return short_and_undisturbed && followed_by_suppressed_direction && is_maneuver &&
           keeps_direction;
}

bool nameChangeImmediatelyAfterSuppressed(const RouteStepIterator step_entering_intersection,
                                          const RouteStepIterator step_leaving_intersection)
{
    if (!basicCollapsePreconditions(step_entering_intersection, step_leaving_intersection))
        return false;

    const auto very_short = step_entering_intersection->distance < 0.25 * MAX_COLLAPSE_DISTANCE;
    const auto correct_types = hasTurnType(*step_entering_intersection, TurnType::Suppressed) &&
                               hasTurnType(*step_leaving_intersection, TurnType::NewName);

    return very_short && correct_types;
}

bool closeChoicelessTurnAfterTurn(const RouteStepIterator step_entering_intersection,
                                  const RouteStepIterator step_leaving_intersection)
{
    if (!basicCollapsePreconditions(step_entering_intersection, step_leaving_intersection))
        return false;

    const auto short_and_undisturbed = isShortAndUndisturbed(*step_entering_intersection);
    const auto is_turn = !hasModifier(*step_entering_intersection, DirectionModifier::Straight);

    const auto followed_by_choiceless = numberOfAllowedTurns(*step_leaving_intersection) == 1;
    const auto followed_by_suppressed =
        hasTurnType(*step_leaving_intersection, TurnType::Suppressed);

    return short_and_undisturbed && is_turn && followed_by_choiceless && !followed_by_suppressed;
}

bool doubleChoiceless(const RouteStepIterator step_entering_intersection,
                      const RouteStepIterator step_leaving_intersection)
{
    if (!basicCollapsePreconditions(step_entering_intersection, step_leaving_intersection))
        return false;

    const auto double_choiceless =
        (numberOfAllowedTurns(*step_leaving_intersection) == 1) &&
        (step_entering_intersection->intersections.size() == 2) &&
        (std::count(step_entering_intersection->intersections.back().entry.begin(),
                    step_entering_intersection->intersections.back().entry.end(),
                    true) == 1);

    const auto short_enough = step_entering_intersection->distance < 1.5 * MAX_COLLAPSE_DISTANCE;

    return double_choiceless && short_enough;
}

bool straightTurnFollowedByChoiceless(const RouteStepIterator step_entering_intersection,
                                      const RouteStepIterator step_leaving_intersection)
{
    if (!basicCollapsePreconditions(step_entering_intersection, step_leaving_intersection))
        return false;

    const auto is_short = step_entering_intersection->distance <= 2 * MAX_COLLAPSE_DISTANCE;
    const auto has_correct_type = hasTurnType(*step_entering_intersection, TurnType::Continue) ||
                                  hasTurnType(*step_entering_intersection, TurnType::Turn);

    const auto is_straight = hasModifier(*step_entering_intersection, DirectionModifier::Straight);

    const auto only_choice = numberOfAllowedTurns(*step_leaving_intersection) == 1;

    return is_short && has_correct_type && is_straight && only_choice &&
           noIntermediaryIntersections(*step_entering_intersection);
}

} /* namespace guidance */
} /* namespace engine */
} /* namespace osrm */
