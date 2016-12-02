#include "engine/guidance/post_processing.hpp"
#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/turn_instruction.hpp"

#include "engine/guidance/assemble_steps.hpp"
#include "engine/guidance/lane_processing.hpp"

#include "util/bearing.hpp"
#include "util/guidance/name_announcements.hpp"
#include "util/guidance/turn_lanes.hpp"

#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/iterator_range.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>

using TurnInstruction = osrm::extractor::guidance::TurnInstruction;
namespace TurnType = osrm::extractor::guidance::TurnType;
namespace DirectionModifier = osrm::extractor::guidance::DirectionModifier;
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
const constexpr std::size_t MIN_END_OF_ROAD_INTERSECTIONS = std::size_t{2};
const constexpr double MAX_COLLAPSE_DISTANCE = 30;

// check if at least one of the turns is actually a maneuver
inline bool hasManeuver(const RouteStep &first, const RouteStep &second)
{
    return (first.maneuver.instruction.type != TurnType::Suppressed ||
            second.maneuver.instruction.type != TurnType::Suppressed) &&
           (first.maneuver.instruction.type != TurnType::NoTurn &&
            second.maneuver.instruction.type != TurnType::NoTurn);
}

// forward all signage/name data from one step to another.
// When we collapse a step, we might have to transfer the name, pronunciation and similar tags.
inline void forwardStepSignage(RouteStep &destination, const RouteStep &origin)
{
    destination.name_id = origin.name_id;
    destination.name = origin.name;
    destination.pronunciation = origin.pronunciation;
    destination.destinations = origin.destinations;
    destination.destinations = origin.destinations;
    destination.ref = origin.ref;
}

inline bool choiceless(const RouteStep &step, const RouteStep &previous)
{
    // if the next turn is choiceless, we consider longer turn roads collapsable than usually
    // accepted. We might need to improve this to find out whether we merge onto a through-street.
    BOOST_ASSERT(!step.intersections.empty());
    const auto is_without_choice = previous.distance < 4 * MAX_COLLAPSE_DISTANCE &&
                                   1 >= std::count(step.intersections.front().entry.begin(),
                                                   step.intersections.front().entry.end(),
                                                   true);

    return is_without_choice;
}

// List of types that can be collapsed, if all other restrictions pass
bool isCollapsableInstruction(const TurnInstruction instruction)
{
    return instruction.type == TurnType::NewName ||
           (instruction.type == TurnType::Suppressed &&
            instruction.direction_modifier == DirectionModifier::Straight) ||
           (instruction.type == TurnType::Turn &&
            instruction.direction_modifier == DirectionModifier::Straight) ||
           (instruction.type == TurnType::Continue &&
            instruction.direction_modifier == DirectionModifier::Straight) ||
           (instruction.type == TurnType::Merge);
}

bool compatible(const RouteStep &lhs, const RouteStep &rhs) { return lhs.mode == rhs.mode; }

// invalidate a step and set its content to nothing
void invalidateStep(RouteStep &step) { step = getInvalidRouteStep(); }

// Checks if name change happens the user wants to know about.
// Treats e.g. "Name (Ref)" -> "Name" changes still as same name.
bool isNoticeableNameChange(const RouteStep &lhs, const RouteStep &rhs)
{
    // TODO: rotary_name is not handled at the moment.
    return util::guidance::requiresNameAnnounced(
        lhs.name, lhs.ref, lhs.pronunciation, rhs.name, rhs.ref, rhs.pronunciation);
}

double nameSegmentLength(std::size_t at, const std::vector<RouteStep> &steps)
{
    BOOST_ASSERT(at < steps.size());

    double result = steps[at].distance;
    while (at + 1 < steps.size() && !isNoticeableNameChange(steps[at], steps[at + 1]))
    {
        at += 1;
        result += steps[at].distance;
    }
    return result;
}

OSRM_ATTR_WARN_UNUSED
RouteStep forwardInto(RouteStep destination, const RouteStep &source)
{
    // Merge a turn into a silent turn
    // Overwrites turn instruction and increases exit NR
    destination.duration += source.duration;
    destination.distance += source.distance;
    destination.maneuver.exit = source.maneuver.exit;
    if (destination.geometry_begin < source.geometry_begin)
    {
        destination.intersections.insert(destination.intersections.end(),
                                         source.intersections.begin(),
                                         source.intersections.end());
    }
    else
    {
        destination.intersections.insert(destination.intersections.begin(),
                                         source.intersections.begin(),
                                         source.intersections.end());
    }

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
            invalidateStep(propagation_step);
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
                     steps[1].maneuver.instruction.type == TurnType::StayOnRoundabout ||
                     steps[1].maneuver.instruction.type == TurnType::Suppressed ||
                     steps[1].maneuver.instruction.type == TurnType::NoTurn ||
                     steps[1].maneuver.instruction.type == TurnType::UseLane);
        steps[0].geometry_end = 1;
        steps[1].geometry_begin = 0;
        steps[1] = forwardInto(steps[1], steps[0]);
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
            propagation_step = forwardInto(propagation_step, steps[propagation_index + 1]);
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
                    const double angle = util::angleBetweenBearings(
                        util::reverseBearing(entry_intersection.bearings[entry_intersection.in]),
                        exit_bearing);

                    auto bearings = propagation_step.intersections.front().bearings;
                    propagation_step.maneuver.instruction.direction_modifier =
                        getTurnDirection(angle);
                }

                forwardStepSignage(propagation_step, destination_copy);
                invalidateStep(steps[propagation_index + 1]);
                break;
            }
            else
            {
                invalidateStep(steps[propagation_index + 1]);
            }
        }
        // remove exit
    }
}

bool bearingsAreReversed(const double bearing_in, const double bearing_out)
{
    // Nearly perfectly reversed angles have a difference close to 180 degrees (straight)
    const double left_turn_angle = [&]() {
        if (0 <= bearing_out && bearing_out <= bearing_in)
            return bearing_in - bearing_out;
        return bearing_in + 360 - bearing_out;
    }();
    return angularDeviation(left_turn_angle, 180) <= 35;
}

bool isLinkroad(const RouteStep &step)
{
    const constexpr double MAX_LINK_ROAD_LENGTH = 60.0;
    return step.distance <= MAX_LINK_ROAD_LENGTH && step.name_id == EMPTY_NAMEID;
}

bool isUTurn(const RouteStep &in_step, const RouteStep &out_step, const RouteStep &pre_in_step)
{
    const bool is_possible_candidate =
        in_step.distance <= MAX_COLLAPSE_DISTANCE || choiceless(out_step, in_step) ||
        (isLinkroad(in_step) && out_step.name_id != EMPTY_NAMEID &&
         pre_in_step.name_id != EMPTY_NAMEID && !isNoticeableNameChange(pre_in_step, out_step));
    const bool takes_u_turn = bearingsAreReversed(
        util::reverseBearing(
            in_step.intersections.front().bearings[in_step.intersections.front().in]),
        out_step.intersections.front().bearings[out_step.intersections.front().out]);

    return is_possible_candidate && takes_u_turn && compatible(in_step, out_step);
}

double findTotalTurnAngle(const RouteStep &entry_step, const RouteStep &exit_step)
{
    const auto exit_intersection = exit_step.intersections.front();
    const auto exit_step_exit_bearing = exit_intersection.bearings[exit_intersection.out];
    const auto exit_step_entry_bearing =
        util::reverseBearing(exit_intersection.bearings[exit_intersection.in]);

    const auto entry_intersection = entry_step.intersections.front();
    const auto entry_step_entry_bearing =
        util::reverseBearing(entry_intersection.bearings[entry_intersection.in]);
    const auto entry_step_exit_bearing = entry_intersection.bearings[entry_intersection.out];

    const auto exit_angle =
        util::angleBetweenBearings(exit_step_entry_bearing, exit_step_exit_bearing);
    const auto entry_angle =
        util::angleBetweenBearings(entry_step_entry_bearing, entry_step_exit_bearing);

    const double total_angle =
        util::angleBetweenBearings(entry_step_entry_bearing, exit_step_exit_bearing);
    // We allow for minor deviations from a straight line
    if (((entry_step.distance < MAX_COLLAPSE_DISTANCE && exit_step.intersections.size() == 1) ||
         (entry_angle <= 185 && exit_angle <= 185) || (entry_angle >= 175 && exit_angle >= 175)) &&
        angularDeviation(total_angle, 180) > 20)
    {
        // both angles are in the same direction, the total turn gets increased
        //
        // a ---- b
        //           \
        //              c
        //              |
        //              d
        //
        // Will be considered just like
        // a -----b
        //        |
        //        c
        //        |
        //        d
        return total_angle;
    }
    else
    {
        // to prevent ignoring angles like
        // a -- b
        //      |
        //      c -- d
        // We don't combine both turn angles here but keep the very first turn angle.
        // We choose the first one, since we consider the first maneuver in a merge range the
        // important one
        return entry_angle;
    }
}

std::size_t getPreviousIndex(std::size_t index, const std::vector<RouteStep> &steps)
{
    BOOST_ASSERT(index > 0);
    BOOST_ASSERT(index < steps.size());
    --index;
    while (index > 0 && steps[index].maneuver.instruction.type == TurnType::NoTurn)
        --index;

    return index;
}

void collapseUTurn(std::vector<RouteStep> &steps,
                   const std::size_t two_back_index,
                   const std::size_t one_back_index,
                   const std::size_t step_index)
{
    BOOST_ASSERT(two_back_index < steps.size());
    BOOST_ASSERT(step_index < steps.size());
    BOOST_ASSERT(one_back_index < steps.size());
    const auto &current_step = steps[step_index];

    // the simple case is a u-turn that changes directly into the in-name again
    const bool direct_u_turn = !isNoticeableNameChange(steps[two_back_index], current_step);

    // however, we might also deal with a dual-collapse scenario in which we have to
    // additionall collapse a name-change as welll
    const auto next_step_index = step_index + 1;
    const bool continues_with_name_change =
        (next_step_index < steps.size()) && compatible(steps[step_index], steps[next_step_index]) &&
        ((steps[next_step_index].maneuver.instruction.type == TurnType::UseLane &&
          steps[next_step_index].maneuver.instruction.direction_modifier ==
              DirectionModifier::Straight) ||
         isCollapsableInstruction(steps[next_step_index].maneuver.instruction));
    const bool u_turn_with_name_change =
        continues_with_name_change && steps[next_step_index].name_id != EMPTY_NAMEID &&
        !isNoticeableNameChange(steps[two_back_index], steps[next_step_index]);

    if (direct_u_turn || u_turn_with_name_change)
    {
        steps[one_back_index] = elongate(std::move(steps[one_back_index]), steps[step_index]);
        invalidateStep(steps[step_index]);
        if (u_turn_with_name_change)
        {
            BOOST_ASSERT_MSG(compatible(steps[one_back_index], steps[next_step_index]),
                             "Compatibility should be transitive");
            steps[one_back_index] =
                elongate(std::move(steps[one_back_index]), steps[next_step_index]);
            invalidateStep(steps[next_step_index]); // will be skipped due to the
                                                    // continue statement at the
                                                    // beginning of this function
        }
        forwardStepSignage(steps[one_back_index], steps[two_back_index]);
        steps[one_back_index].maneuver.instruction.type = TurnType::Continue;
        steps[one_back_index].maneuver.instruction.direction_modifier = DirectionModifier::UTurn;
    }
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
    // Don't collapse roundabouts
    if (entersRoundabout(current_step.maneuver.instruction) ||
        entersRoundabout(one_back_step.maneuver.instruction))
        return;

    // This function assumes driving on the right hand side of the streat
    BOOST_ASSERT(!one_back_step.intersections.empty() && !current_step.intersections.empty());

    if (!hasManeuver(one_back_step, current_step))
        return;

    // A maneuver is preceded by a name change if the instruction just before can be collapsed
    // normally or the instruction itself is collapsable and does not actually present a choice
    const auto maneuverPrecededByNameChange = [](const RouteStep &turning_point,
                                                 const RouteStep &possible_name_change_location,
                                                 const RouteStep &preceeding_step) {
        // the check against merge is a workaround for motorways
        if (possible_name_change_location.maneuver.instruction.type == TurnType::Merge ||
            !compatible(possible_name_change_location, preceeding_step))
            return false;

        return collapsable(possible_name_change_location, turning_point) ||
               (isCollapsableInstruction(possible_name_change_location.maneuver.instruction) &&
                choiceless(possible_name_change_location, preceeding_step));
    };

    // check if the actual turn we wan't to announce is delayed. This situation describes a turn
    // that is expressed by two turns,
    const auto isDelayedTurn = [](const RouteStep &opening_turn, const RouteStep &finishing_turn) {
        // only possible if both are compatible
        if (!compatible(opening_turn, finishing_turn))
            return false;
        else
        {
            const auto is_short_and_collapsable =
                opening_turn.distance <= MAX_COLLAPSE_DISTANCE &&
                isCollapsableInstruction(finishing_turn.maneuver.instruction);

            const auto without_choice = choiceless(finishing_turn, opening_turn);

            const auto is_not_too_long_and_choiceless =
                opening_turn.distance <= 2 * MAX_COLLAPSE_DISTANCE && without_choice;

            // for ramps we allow longer stretches, since they are often on some major brides/large
            // roads. A combined distance of of 4 intersections would be to long for a normal
            // collapse. In case of a ramp though, we also account for situations that have the ramp
            // tagged late
            const auto is_delayed_turn_onto_a_ramp =
                opening_turn.distance <= 4 * MAX_COLLAPSE_DISTANCE && without_choice &&
                hasRampType(finishing_turn.maneuver.instruction);
            return !hasRampType(opening_turn.maneuver.instruction) &&
                   (is_short_and_collapsable || is_not_too_long_and_choiceless ||
                    isLinkroad(opening_turn) || is_delayed_turn_onto_a_ramp);
        }
    };

    // Handle possible u-turns
    if (isUTurn(one_back_step, current_step, steps[two_back_index]))
        collapseUTurn(steps, two_back_index, one_back_index, step_index);
    // Very Short New Name that will be suppressed. Turn location remains at current_step
    else if (maneuverPrecededByNameChange(current_step, one_back_step, steps[two_back_index]))
    {
        BOOST_ASSERT(two_back_index < steps.size());
        BOOST_ASSERT(!one_back_step.intersections.empty());
        if (TurnType::Merge == current_step.maneuver.instruction.type)
        {
            steps[step_index].maneuver.instruction.direction_modifier =
                mirrorDirectionModifier(steps[step_index].maneuver.instruction.direction_modifier);
            steps[step_index].maneuver.instruction.type = TurnType::Turn;
        }
        else
        {
            const bool continue_or_suppressed =
                (TurnType::Continue == current_step.maneuver.instruction.type ||
                 (TurnType::Suppressed == current_step.maneuver.instruction.type &&
                  current_step.maneuver.instruction.direction_modifier !=
                      DirectionModifier::Straight));

            const bool turning_name =
                (TurnType::NewName == current_step.maneuver.instruction.type &&
                 current_step.maneuver.instruction.direction_modifier !=
                     DirectionModifier::Straight &&
                 one_back_step.intersections.front().bearings.size() > 2);

            if (continue_or_suppressed)
                steps[step_index].maneuver.instruction.type = TurnType::Turn;
            else if (turning_name)
                steps[step_index].maneuver.instruction.type = TurnType::Turn;
            else if (TurnType::UseLane == current_step.maneuver.instruction.type &&
                     current_step.maneuver.instruction.direction_modifier !=
                         DirectionModifier::Straight &&
                     one_back_step.intersections.front().bearings.size() > 2)
                steps[step_index].maneuver.instruction.type = TurnType::Turn;

            // A new name with a continue/turning suppressed/name requires the adaption of the
            // direction modifier. The combination of the in-bearing and the out bearing gives the
            // new modifier for the turn
            if (continue_or_suppressed || turning_name)
            {
                const auto in_bearing = [](const RouteStep &step) {
                    return util::reverseBearing(
                        step.intersections.front().bearings[step.intersections.front().in]);
                };
                const auto out_bearing = [](const RouteStep &step) {
                    return step.intersections.front().bearings[step.intersections.front().out];
                };

                const auto first_angle = util::angleBetweenBearings(in_bearing(one_back_step),
                                                                    out_bearing(one_back_step));
                const auto second_angle =
                    util::angleBetweenBearings(in_bearing(current_step), out_bearing(current_step));
                const auto bearing_turn_angle = util::angleBetweenBearings(
                    in_bearing(one_back_step), out_bearing(current_step));

                // When looking at an intersection, some angles, even though present, feel more like
                // a straight turn. This happens most often at segregated intersections.
                // We consider two cases
                // I) a shift in the road:
                //
                // a       g      h
                //     .   |      |
                //         b ---- c
                //         |      |    .
                //         f      e        d
                //
                // Where a-d technicall continues straight, even though the shift models it as a
                // slight left and a slight right turn.
                //
                // II) A curved road
                //
                //         g      h
                //         |      |
                //         b ---- c
                //    .    |      |    .
                // a       f      e        d
                //
                // where a-d is a curve passing by an intersection.
                //
                // We distinguish this case from other bearings though where the interpretation as
                // straight would end up disguising turns.

                // check if there is another similar turn next to the turn itself
                const auto hasSimilarAngle = [&](const std::size_t index,
                                                 const std::vector<short> &bearings) {
                    return (angularDeviation(bearings[index],
                                             bearings[(index + 1) % bearings.size()]) <
                            extractor::guidance::NARROW_TURN_ANGLE) ||
                           (angularDeviation(
                                bearings[index],
                                bearings[(index + bearings.size() - 1) % bearings.size()]) <
                            extractor::guidance::NARROW_TURN_ANGLE);
                };

                const auto is_shift_or_curve = [&]() -> bool {
                    // since we move an intersection modifier from a slight turn to a straight, we
                    // need to make sure that there is not a similar angle which could prevent this
                    // perception of angles to be true.
                    if (hasSimilarAngle(one_back_step.intersections.front().in,
                                        one_back_step.intersections.front().bearings) ||
                        hasSimilarAngle(current_step.intersections.front().out,
                                        current_step.intersections.front().bearings))
                        return false;

                    // Check if we are on a potential curve, both angles go in the same direction
                    if (angularDeviation(first_angle, second_angle) <
                        extractor::guidance::FUZZY_ANGLE_DIFFERENCE)
                    {
                        // We limit perceptive angles to narrow turns. If the total turn is going to
                        // be not-narrow, we assume it to be more than a simple curve.
                        return angularDeviation(bearing_turn_angle,
                                                extractor::guidance::STRAIGHT_ANGLE) <
                               extractor::guidance::NARROW_TURN_ANGLE;
                    }
                    // if one of the angles is a left turn and the other one is a right turn, we
                    // nearly reverse the angle
                    else if ((first_angle > extractor::guidance::STRAIGHT_ANGLE) !=
                             (second_angle > extractor::guidance::STRAIGHT_ANGLE))
                    {
                        // since we are not in a curve, we can check for a shift. If we are going
                        // nearly straight, we call it a shift.
                        return angularDeviation(bearing_turn_angle,
                                                extractor::guidance::STRAIGHT_ANGLE) <
                               extractor::guidance::NARROW_TURN_ANGLE;
                    }
                    else
                    {
                        return false;
                    }
                }();

                // if the angles continue similar, it looks like we might be in a normal curve
                if (is_shift_or_curve)
                    steps[step_index].maneuver.instruction.direction_modifier =
                        DirectionModifier::Straight;
                else
                    steps[step_index].maneuver.instruction.direction_modifier =
                        getTurnDirection(bearing_turn_angle);

                // if the total direction of this turn is now straight, we can keep it suppressed/as
                // a new name. Else we have to interpret it as a turn.
                if (!is_shift_or_curve)
                    steps[step_index].maneuver.instruction.type = TurnType::Turn;
                else
                    steps[step_index].maneuver.instruction.type = TurnType::NewName;
            }
            else
            {
                const auto total_angle = findTotalTurnAngle(steps[one_back_index], current_step);
                steps[step_index].maneuver.instruction.direction_modifier =
                    getTurnDirection(total_angle);
            }
        }

        steps[two_back_index] = elongate(std::move(steps[two_back_index]), one_back_step);
        // If the previous instruction asked to continue, the name change will have to
        // be changed into a turn
        invalidateStep(steps[one_back_index]);
    }
    // very short segment after turn, turn location remains at one_back_step
    else if (isDelayedTurn(one_back_step, current_step)) // checks for compatibility
    {
        steps[one_back_index] = elongate(std::move(steps[one_back_index]), steps[step_index]);
        // TODO check for lanes (https://github.com/Project-OSRM/osrm-backend/issues/2553)
        if (TurnType::Continue == one_back_step.maneuver.instruction.type &&
            isNoticeableNameChange(steps[two_back_index], current_step))
        {
            if (current_step.maneuver.instruction.type == TurnType::OnRamp ||
                current_step.maneuver.instruction.type == TurnType::OffRamp)
                steps[one_back_index].maneuver.instruction.type =
                    current_step.maneuver.instruction.type;
            else
                steps[one_back_index].maneuver.instruction.type = TurnType::Turn;
        }
        else if (TurnType::Turn == one_back_step.maneuver.instruction.type &&
                 !isNoticeableNameChange(steps[two_back_index], current_step))
        {
            steps[one_back_index].maneuver.instruction.type = TurnType::Continue;

            const auto getBearing = [](bool in, const RouteStep &step) {
                const auto index =
                    in ? step.intersections.front().in : step.intersections.front().out;
                return step.intersections.front().bearings[index];
            };

            // If we Merge onto the same street, we end up with a u-turn in some cases
            if (bearingsAreReversed(util::reverseBearing(getBearing(true, one_back_step)),
                                    getBearing(false, current_step)))
            {
                steps[one_back_index].maneuver.instruction.direction_modifier =
                    DirectionModifier::UTurn;
            }
            forwardStepSignage(steps[one_back_index], current_step);
        }
        else if (TurnType::NewName == one_back_step.maneuver.instruction.type ||
                 (TurnType::NewName == current_step.maneuver.instruction.type &&
                  steps[one_back_index].maneuver.instruction.type == TurnType::Suppressed))
            steps[one_back_index].maneuver.instruction.type = TurnType::Turn;

        if (TurnType::Merge == one_back_step.maneuver.instruction.type &&
            current_step.maneuver.instruction.type !=
                TurnType::Suppressed) // This suppressed is a check for highways. We might
                                      // need a highway-suppressed to get the turn onto a
                                      // highway...
        {
            steps[one_back_index].maneuver.instruction.direction_modifier = mirrorDirectionModifier(
                steps[one_back_index].maneuver.instruction.direction_modifier);
        }
        // on non merge-types, we check for a combined turn angle
        else if (TurnType::Merge != one_back_step.maneuver.instruction.type)
        {
            const auto combined_angle = findTotalTurnAngle(one_back_step, current_step);
            steps[one_back_index].maneuver.instruction.direction_modifier =
                getTurnDirection(combined_angle);
        }

        steps[one_back_index].name = current_step.name;
        steps[one_back_index].name_id = current_step.name_id;
        invalidateStep(steps[step_index]);
    }
    else if (TurnType::Suppressed == current_step.maneuver.instruction.type &&
             !isNoticeableNameChange(one_back_step, current_step) &&
             compatible(one_back_step, current_step))
    {
        steps[one_back_index] = elongate(std::move(steps[one_back_index]), current_step);
        const auto angle = findTotalTurnAngle(one_back_step, current_step);
        steps[one_back_index].maneuver.instruction.direction_modifier = getTurnDirection(angle);

        invalidateStep(steps[step_index]);
    }
    else if (TurnType::Turn == one_back_step.maneuver.instruction.type &&
             TurnType::OnRamp == current_step.maneuver.instruction.type &&
             compatible(one_back_step, current_step))
    {
        // turning onto a ramp makes the first turn into a ramp
        steps[one_back_index] = elongate(std::move(steps[one_back_index]), current_step);
        steps[one_back_index].maneuver.instruction.type = TurnType::OnRamp;
        const auto angle = findTotalTurnAngle(one_back_step, current_step);
        steps[one_back_index].maneuver.instruction.direction_modifier = getTurnDirection(angle);

        forwardStepSignage(steps[one_back_index], current_step);
        invalidateStep(steps[step_index]);
    }
}

// Staggered intersection are very short zig-zags of a few meters.
// We do not want to announce these short left-rights or right-lefts:
//
//      * -> b      a -> *
//      |       or       |       becomes  a   ->   b
// a -> *                * -> b
//
bool isStaggeredIntersection(const std::vector<RouteStep> &steps,
                             const std::size_t &current_index,
                             const std::size_t &previous_index)
{
    const RouteStep previous = steps[previous_index];
    const RouteStep current = steps[current_index];

    // don't touch roundabouts
    if (entersRoundabout(previous.maneuver.instruction) ||
        entersRoundabout(current.maneuver.instruction))
        return false;
    // Base decision on distance since the zig-zag is a visual clue.
    // If adjusted, make sure to check validity of the is_right/is_left classification below
    const constexpr auto MAX_STAGGERED_DISTANCE = 3; // debatable, but keep short to be on safe side

    const auto angle = [](const RouteStep &step) {
        const auto &intersection = step.intersections.front();
        const auto entry_bearing = intersection.bearings[intersection.in];
        const auto exit_bearing = intersection.bearings[intersection.out];
        return util::angleBetweenBearings(entry_bearing, exit_bearing);
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

    const auto left_right = is_left(angle(previous)) && is_right(angle(current));
    const auto right_left = is_right(angle(previous)) && is_left(angle(current));

    // A RouteStep holds distance/duration from the maneuver to the subsequent step.
    // We are only interested in the distance between the first and the second.
    const auto is_short = previous.distance < MAX_STAGGERED_DISTANCE;

    auto intermediary_mode_change = false;
    if (current_index > 1)
    {
        const auto &two_back_index = getPreviousIndex(previous_index, steps);
        const auto two_back_step = steps[two_back_index];
        intermediary_mode_change =
            two_back_step.mode == current.mode && previous.mode != current.mode;
    }

    // previous step maneuver intersections should be length 1 to indicate that
    // there are no intersections between the two potentially collapsible turns
    const auto no_intermediary_intersections = previous.intersections.size() == 1;

    return is_short && (left_right || right_left) && !intermediary_mode_change &&
           no_intermediary_intersections;
}

} // namespace

// A check whether two instructions can be treated as one. This is only the case for very short
// maneuvers that can, in some form, be seen as one. Lookahead of one step.
bool collapsable(const RouteStep &step, const RouteStep &next)
{
    const auto is_short_step = step.distance < MAX_COLLAPSE_DISTANCE;
    const auto instruction_can_be_collapsed = isCollapsableInstruction(step.maneuver.instruction);

    const auto is_use_lane = step.maneuver.instruction.type == TurnType::UseLane;
    const auto lanes_dont_change =
        step.intersections.front().lanes == next.intersections.front().lanes;

    if (is_short_step && instruction_can_be_collapsed)
        return true;

    // Prevent collapsing away important lane change steps
    if (is_short_step && is_use_lane && lanes_dont_change)
        return true;

    return false;
}

// elongate a step by another. the data is added either at the front, or the back
OSRM_ATTR_WARN_UNUSED
RouteStep elongate(RouteStep step, const RouteStep &by_step)
{
    BOOST_ASSERT(step.mode == by_step.mode);

    step.duration += by_step.duration;
    step.distance += by_step.distance;
    BOOST_ASSERT(step.mode == by_step.mode);

    // by_step comes after step -> we append at the end
    if (step.geometry_end == by_step.geometry_begin + 1)
    {
        step.geometry_end = by_step.geometry_end;

        // if we elongate in the back, we only need to copy the intersections to the beginning.
        // the bearings remain the same, as the location of the turn doesn't change
        step.intersections.insert(
            step.intersections.end(), by_step.intersections.begin(), by_step.intersections.end());
    }
    // by_step comes before step -> we append at the front
    else
    {
        BOOST_ASSERT(step.maneuver.waypoint_type == WaypointType::None &&
                     by_step.maneuver.waypoint_type == WaypointType::None);
        BOOST_ASSERT(by_step.geometry_end == step.geometry_begin + 1);
        step.geometry_begin = by_step.geometry_begin;

        // elongating in the front changes the location of the maneuver
        step.maneuver = by_step.maneuver;

        step.intersections.insert(
            step.intersections.begin(), by_step.intersections.begin(), by_step.intersections.end());
    }
    return step;
}

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

    const auto getPreviousNameIndex = [&steps](std::size_t index) {
        BOOST_ASSERT(index > 0);
        BOOST_ASSERT(index < steps.size());
        --index; // make sure to skip the current name
        while (index > 0 && steps[index].name_id == EMPTY_NAMEID)
        {
            --index;
        }
        return index;
    };

    // a series of turns is only possible to collapse if its only name changes and suppressed turns.
    const auto canCollapseAll = [&steps](std::size_t index, const std::size_t end_index) {
        BOOST_ASSERT(end_index <= steps.size());
        if (!compatible(steps[index], steps[index + 1]))
            return false;
        ++index;
        for (; index < end_index; ++index)
        {
            if (steps[index].maneuver.instruction.type != TurnType::Suppressed &&
                steps[index].maneuver.instruction.type != TurnType::NewName)
                return false;
            if (index + 1 < end_index && !compatible(steps[index], steps[index + 1]))
                return false;
        }
        return true;
    };

    // first and last instructions are waypoints that cannot be collapsed
    for (std::size_t step_index = 1; step_index + 1 < steps.size(); ++step_index)
    {
        const auto &current_step = steps[step_index];
        const auto next_step_index = step_index + 1;
        const auto one_back_index = getPreviousIndex(step_index, steps);
        BOOST_ASSERT(one_back_index < steps.size());

        const auto &one_back_step = steps[one_back_index];

        if (!hasManeuver(one_back_step, current_step))
            continue;

        // how long has a name change to be so that we announce it, even as a bridge?
        const constexpr auto name_segment_cutoff_length = 100;
        const auto isBasicNameChange = [](const RouteStep &step) {
            return step.intersections.size() == 1 &&
                   step.intersections.front().bearings.size() == 2 &&
                   DirectionModifier::Straight == step.maneuver.instruction.direction_modifier;
        };

        // Handle sliproads from motorways in urban areas, save from modifying depart, since
        // TurnType::Sliproad != TurnType::NoTurn
        if (one_back_step.maneuver.instruction.type == TurnType::Sliproad)
        {
            if (current_step.maneuver.instruction.type == TurnType::Suppressed &&
                compatible(one_back_step, current_step))
            {
                // Traffic light on the sliproad, the road itself will be handled in the next
                // iteration, when one-back-index again points to the sliproad.
                steps[one_back_index] =
                    elongate(std::move(steps[one_back_index]), steps[step_index]);
                invalidateStep(steps[step_index]);
            }
            else
            {
                // Handle possible u-turns between highways that look like slip-roads
                if (steps[getPreviousIndex(one_back_index, steps)].name_id ==
                        steps[step_index].name_id &&
                    steps[step_index].name_id != EMPTY_NAMEID)
                {
                    steps[one_back_index].maneuver.instruction.type = TurnType::Continue;
                }
                else
                {
                    steps[one_back_index].maneuver.instruction.type = TurnType::Turn;
                }
                if (compatible(one_back_step, current_step))
                {
                    // Turn Types in the response depend on whether we find the same road name
                    // (sliproad indcating a u-turn) or if we are turning onto a different road, in
                    // which case we use a turn.
                    if (!isNoticeableNameChange(steps[getPreviousIndex(one_back_index, steps)],
                                                current_step) &&
                        current_step.name_id != EMPTY_NAMEID)
                        steps[one_back_index].maneuver.instruction.type = TurnType::Continue;
                    else
                        steps[one_back_index].maneuver.instruction.type = TurnType::Turn;

                    steps[one_back_index] =
                        elongate(std::move(steps[one_back_index]), steps[step_index]);

                    forwardStepSignage(steps[one_back_index], steps[step_index]);
                    // the turn lanes for this turn are on the sliproad itself, so we have to
                    // remember  them
                    steps[one_back_index].intersections.front().lanes =
                        current_step.intersections.front().lanes;
                    steps[one_back_index].intersections.front().lane_description =
                        current_step.intersections.front().lane_description;

                    const auto angle = findTotalTurnAngle(one_back_step, current_step);
                    steps[one_back_index].maneuver.instruction.direction_modifier =
                        getTurnDirection(angle);
                    invalidateStep(steps[step_index]);
                }
                else
                {
                    // the sliproad turn is incompatible. So we handle it as a turn
                    steps[one_back_index].maneuver.instruction.type = TurnType::Turn;
                }
            }
        }
        // Due to empty segments, we can get name-changes from A->A
        // These have to be handled in post-processing
        else if (isCollapsableInstruction(current_step.maneuver.instruction) &&
                 current_step.maneuver.instruction.type != TurnType::Suppressed &&
                 !isNoticeableNameChange(steps[getPreviousNameIndex(step_index)], current_step) &&
                 // canCollapseAll is also checking for compatible(step,step+1) for all indices
                 canCollapseAll(getPreviousNameIndex(step_index), next_step_index))
        {
            BOOST_ASSERT(step_index > 0);
            const std::size_t last_available_name_index = getPreviousNameIndex(step_index);

            for (std::size_t index = last_available_name_index + 1; index <= step_index; ++index)
            {
                steps[last_available_name_index] =
                    elongate(std::move(steps[last_available_name_index]), steps[index]);
                invalidateStep(steps[index]);
            }
        }
        // If we look at two consecutive name changes, we can check for a name oscillation.
        // A name oscillation changes from name A shortly to name B and back to A.
        // In these cases, the name change will be suppressed.
        else if (one_back_index > 0 && compatible(current_step, one_back_step) &&
                 ((isCollapsableInstruction(current_step.maneuver.instruction) &&
                   isCollapsableInstruction(one_back_step.maneuver.instruction)) ||
                  isStaggeredIntersection(steps, step_index, one_back_index)))
        {
            const auto two_back_index = getPreviousIndex(one_back_index, steps);
            BOOST_ASSERT(two_back_index < steps.size());
            // valid, since one_back is collapsable or a turn and therefore not depart:
            if (!isNoticeableNameChange(steps[two_back_index], current_step))
            {
                if (compatible(one_back_step, steps[two_back_index]))
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
            else if (nameSegmentLength(one_back_index, steps) < name_segment_cutoff_length &&
                     isBasicNameChange(one_back_step) && isBasicNameChange(current_step))
            {
                if (compatible(steps[two_back_index], steps[one_back_index]))
                {
                    steps[two_back_index] =
                        elongate(std::move(steps[two_back_index]), steps[one_back_index]);
                    invalidateStep(steps[one_back_index]);
                    if (nameSegmentLength(step_index, steps) < name_segment_cutoff_length &&
                        compatible(steps[two_back_index], steps[step_index]))
                    {
                        steps[two_back_index] =
                            elongate(std::move(steps[two_back_index]), steps[step_index]);
                        invalidateStep(steps[step_index]);
                    }
                }
            }
            else if (step_index + 2 < steps.size() &&
                     current_step.maneuver.instruction.type == TurnType::NewName &&
                     steps[next_step_index].maneuver.instruction.type == TurnType::NewName &&
                     !isNoticeableNameChange(one_back_step, steps[next_step_index]))
            {
                if (compatible(steps[step_index], steps[next_step_index]))
                {
                    // if we are crossing an intersection and go immediately after into a name
                    // change,
                    // we don't wan't to collapse the initial intersection.
                    // a - b ---BRIDGE -- c
                    steps[one_back_index] =
                        elongate(std::move(steps[one_back_index]),
                                 elongate(std::move(steps[step_index]), steps[next_step_index]));
                    invalidateStep(steps[step_index]);
                    invalidateStep(steps[next_step_index]);
                }
            }
            else if (choiceless(current_step, one_back_step) ||
                     one_back_step.distance <= MAX_COLLAPSE_DISTANCE)
            {
                // check for one of the multiple collapse scenarios and, if possible, collapse the
                // turn
                const auto two_back_index = getPreviousIndex(one_back_index, steps);
                BOOST_ASSERT(two_back_index < steps.size());
                collapseTurnAt(steps, two_back_index, one_back_index, step_index);
            }
        }
        else if (one_back_index > 0 &&
                 (one_back_step.distance <= MAX_COLLAPSE_DISTANCE ||
                  choiceless(current_step, one_back_step) || isLinkroad(one_back_step)))
        {
            // check for one of the multiple collapse scenarios and, if possible, collapse the turn
            const auto two_back_index = getPreviousIndex(one_back_index, steps);
            BOOST_ASSERT(two_back_index < steps.size());
            // all turns that are handled lower down are also compatible
            collapseTurnAt(steps, two_back_index, one_back_index, step_index);
        }

        if (steps[step_index].maneuver.instruction.type == TurnType::Turn)
        {
            const auto u_turn_one_back_index = getPreviousIndex(step_index, steps);
            if (u_turn_one_back_index > 0)
            {
                const auto u_turn_two_back_index = getPreviousIndex(u_turn_one_back_index, steps);
                if (isUTurn(steps[u_turn_one_back_index],
                            steps[step_index],
                            steps[u_turn_two_back_index]))
                {
                    collapseUTurn(steps, u_turn_two_back_index, u_turn_one_back_index, step_index);
                }
            }
        }
    }

    // handle final sliproad
    if (steps.size() >= 3 &&
        steps[getPreviousIndex(steps.size() - 1, steps)].maneuver.instruction.type ==
            TurnType::Sliproad)
    {
        steps[getPreviousIndex(steps.size() - 1, steps)].maneuver.instruction.type = TurnType::Turn;
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
            // coordinates here.
            // move offsets to front
            // geometry offsets have to be adjusted. Move all offsets to the front and reduce by
            // one. (This is an inplace forward one and reduce by one)
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
        last_intersection.out = Intersection::NO_INDEX;
        last_intersection.in = 0;
        steps.pop_back();

        // Because we eliminated a really short segment, it was probably
        // near an intersection.  The convention is *not* to make the
        // turn, so the `arrive` instruction should be on the same road
        // as the segment before it.  Thus, we have to copy the names
        // and travel modes from the new next_to_last step.
        auto &new_next_to_last = *(steps.end() - 2);
        forwardStepSignage(next_to_last_step, new_next_to_last);
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
        last_step.intersections.front().bearings.front() = util::reverseBearing(bearing);
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
            BOOST_ASSERT(compatible(steps[last_valid_instruction], step));
            // count intersections. We cannot use exit, since intersections can follow directly
            // after a roundabout
            steps[last_valid_instruction] =
                elongate(std::move(steps[last_valid_instruction]), step);
            step.maneuver.instruction = TurnInstruction::NO_TURN();
            invalidateStep(steps[step_index]);
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

// `useLane` steps are only returned on `straight` maneuvers when there
// are surrounding lanes also tagged as `straight`. If there are no other `straight`
// lanes, it is not an ambiguous maneuver, and we can collapse the `useLane` step.
std::vector<RouteStep> collapseUseLane(std::vector<RouteStep> steps)
{
    const auto containsTag = [](const extractor::guidance::TurnLaneType::Mask mask,
                                const extractor::guidance::TurnLaneType::Mask tag) {
        return (mask & tag) != extractor::guidance::TurnLaneType::empty;
    };

    const auto canCollapseUseLane = [containsTag](const RouteStep &step) {
        // the lane description is given left to right, lanes are counted from the right.
        // Therefore we access the lane description using the reverse iterator

        auto right_most_lanes = step.lanesToTheRight();
        if (!right_most_lanes.empty() && containsTag(right_most_lanes.front(),
                                                     (extractor::guidance::TurnLaneType::straight |
                                                      extractor::guidance::TurnLaneType::none)))
            return false;

        auto left_most_lanes = step.lanesToTheLeft();
        if (!left_most_lanes.empty() && containsTag(left_most_lanes.back(),
                                                    (extractor::guidance::TurnLaneType::straight |
                                                     extractor::guidance::TurnLaneType::none)))
            return false;

        return true;
    };

    for (std::size_t step_index = 1; step_index < steps.size(); ++step_index)
    {
        const auto &step = steps[step_index];
        if (step.maneuver.instruction.type == TurnType::UseLane && canCollapseUseLane(step))
        {
            const auto previous = getPreviousIndex(step_index, steps);
            if (compatible(steps[previous], step))
            {
                steps[previous] = elongate(std::move(steps[previous]), steps[step_index]);
                invalidateStep(steps[step_index]);
            }
        }
    }
    return removeNoTurnInstructions(std::move(steps));
}

} // namespace guidance
} // namespace engine
} // namespace osrm
