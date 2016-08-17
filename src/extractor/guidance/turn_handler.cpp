#include "extractor/guidance/turn_handler.hpp"
#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/intersection_scenario_three_way.hpp"
#include "extractor/guidance/toolkit.hpp"

#include "util/guidance/toolkit.hpp"

#include <limits>
#include <utility>

#include <boost/assert.hpp>

using EdgeData = osrm::util::NodeBasedDynamicGraph::EdgeData;
using osrm::util::guidance::getTurnDirection;
using osrm::util::guidance::angularDeviation;

namespace osrm
{
namespace extractor
{
namespace guidance
{

TurnHandler::TurnHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                         const std::vector<QueryNode> &node_info_list,
                         const util::NameTable &name_table,
                         const SuffixTable &street_name_suffix_table,
                         const IntersectionGenerator &intersection_generator)
    : IntersectionHandler(node_based_graph,
                          node_info_list,
                          name_table,
                          street_name_suffix_table,
                          intersection_generator)
{
}

bool TurnHandler::canProcess(const NodeID, const EdgeID, const Intersection &) const
{
    return true;
}

Intersection TurnHandler::
operator()(const NodeID, const EdgeID via_edge, Intersection intersection) const
{
    if (intersection.size() == 1)
        return handleOneWayTurn(std::move(intersection));

    if (intersection[0].entry_allowed)
    {
        intersection[0].turn.instruction = {findBasicTurnType(via_edge, intersection[0]),
                                            DirectionModifier::UTurn};
    }

    if (intersection.size() == 2)
        return handleTwoWayTurn(via_edge, std::move(intersection));

    if (intersection.size() == 3)
        return handleThreeWayTurn(via_edge, std::move(intersection));

    return handleComplexTurn(via_edge, std::move(intersection));
}

Intersection TurnHandler::handleOneWayTurn(Intersection intersection) const
{
    BOOST_ASSERT(intersection[0].turn.angle < 0.001);
    return intersection;
}

Intersection TurnHandler::handleTwoWayTurn(const EdgeID via_edge, Intersection intersection) const
{
    BOOST_ASSERT(intersection[0].turn.angle < 0.001);
    intersection[1].turn.instruction =
        getInstructionForObvious(intersection.size(), via_edge, false, intersection[1]);

    return intersection;
}

bool TurnHandler::isObviousOfTwo(const EdgeID via_edge,
                                 const ConnectedRoad &road,
                                 const ConnectedRoad &other) const
{
    const auto &in_data = node_based_graph.GetEdgeData(via_edge);

    const auto &first_data = node_based_graph.GetEdgeData(road.turn.eid);
    const auto &second_data = node_based_graph.GetEdgeData(other.turn.eid);
    const auto &first_classification = first_data.road_classification;
    const auto &second_classification = second_data.road_classification;
    const bool is_ramp = first_classification.IsRampClass();
    const bool is_obvious_by_road_class =
        (!is_ramp &&
         (2 * first_classification.GetPriority() < second_classification.GetPriority()) &&
         in_data.road_classification == first_classification) ||
        (!first_classification.IsLowPriorityRoadClass() &&
         second_classification.IsLowPriorityRoadClass());

    if (is_obvious_by_road_class)
        return true;

    const bool other_is_obvious_by_road_class =
        (!second_classification.IsRampClass() &&
         (2 * second_classification.GetPriority() < first_classification.GetPriority()) &&
         in_data.road_classification == second_classification) ||
        (!second_classification.IsLowPriorityRoadClass() &&
         first_classification.IsLowPriorityRoadClass());

    if (other_is_obvious_by_road_class)
        return false;

    const bool turn_is_perfectly_straight =
        angularDeviation(road.turn.angle, STRAIGHT_ANGLE) < std::numeric_limits<double>::epsilon();

    if (turn_is_perfectly_straight && in_data.name_id != EMPTY_NAMEID &&
        in_data.name_id == node_based_graph.GetEdgeData(road.turn.eid).name_id)
        return true;

    const bool is_much_narrower_than_other =
        angularDeviation(other.turn.angle, STRAIGHT_ANGLE) /
                angularDeviation(road.turn.angle, STRAIGHT_ANGLE) >
            INCREASES_BY_FOURTY_PERCENT &&
        angularDeviation(angularDeviation(other.turn.angle, STRAIGHT_ANGLE),
                         angularDeviation(road.turn.angle, STRAIGHT_ANGLE)) >
            FUZZY_ANGLE_DIFFERENCE;

    return is_much_narrower_than_other;
}

Intersection TurnHandler::handleThreeWayTurn(const EdgeID via_edge, Intersection intersection) const
{
    const auto obvious_index = findObviousTurn(via_edge, intersection);
    BOOST_ASSERT(intersection[0].turn.angle < 0.001);
    /* Two nearly straight turns -> FORK
               OOOOOOO
             /
      IIIIII
             \
               OOOOOOO
     */
    const auto fork_range = findFork(via_edge, intersection);
    if (fork_range.first == 1 && fork_range.second == 2 && obvious_index == 0)
        assignFork(via_edge, intersection[2], intersection[1]);

    /*  T Intersection

        OOOOOOO T OOOOOOOO
                I
                I
                I
     */
    else if (isEndOfRoad(intersection[0], intersection[1], intersection[2]) && obvious_index == 0)
    {
        if (intersection[1].entry_allowed)
        {
            if (TurnType::OnRamp != findBasicTurnType(via_edge, intersection[1]))
                intersection[1].turn.instruction = {TurnType::EndOfRoad, DirectionModifier::Right};
            else
                intersection[1].turn.instruction = {TurnType::OnRamp, DirectionModifier::Right};
        }
        if (intersection[2].entry_allowed)
        {
            if (TurnType::OnRamp != findBasicTurnType(via_edge, intersection[2]))

                intersection[2].turn.instruction = {TurnType::EndOfRoad, DirectionModifier::Left};
            else
                intersection[2].turn.instruction = {TurnType::OnRamp, DirectionModifier::Left};
        }
    }
    else if (obvious_index != 0) // has an obvious continuing road/obvious turn
    {
        const auto direction_at_one = getTurnDirection(intersection[1].turn.angle);
        const auto direction_at_two = getTurnDirection(intersection[2].turn.angle);
        if (obvious_index == 1)
        {
            intersection[1].turn.instruction = getInstructionForObvious(
                3, via_edge, isThroughStreet(1, intersection), intersection[1]);

            const auto second_direction = (direction_at_one == direction_at_two &&
                                           direction_at_two == DirectionModifier::Straight)
                                              ? DirectionModifier::SlightLeft
                                              : direction_at_two;

            intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                second_direction};
        }
        else
        {
            BOOST_ASSERT(obvious_index == 2);
            intersection[2].turn.instruction = getInstructionForObvious(
                3, via_edge, isThroughStreet(2, intersection), intersection[2]);

            const auto first_direction = (direction_at_one == direction_at_two &&
                                          direction_at_one == DirectionModifier::Straight)
                                             ? DirectionModifier::SlightRight
                                             : direction_at_one;

            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                first_direction};
        }
    }
    else // basic turn assignment
    {
        intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                            getTurnDirection(intersection[1].turn.angle)};
        intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                            getTurnDirection(intersection[2].turn.angle)};
    }
    return intersection;
}

Intersection TurnHandler::handleComplexTurn(const EdgeID via_edge, Intersection intersection) const
{
    const std::size_t obvious_index = findObviousTurn(via_edge, intersection);
    const auto fork_range = findFork(via_edge, intersection);
    std::size_t straightmost_turn = 0;
    double straightmost_deviation = 180;
    for (std::size_t i = 0; i < intersection.size(); ++i)
    {
        const double deviation = angularDeviation(intersection[i].turn.angle, STRAIGHT_ANGLE);
        if (deviation < straightmost_deviation)
        {
            straightmost_deviation = deviation;
            straightmost_turn = i;
        }
    }

    // check whether the obvious choice is actually a through street
    if (obvious_index != 0)
    {
        intersection[obvious_index].turn.instruction =
            getInstructionForObvious(intersection.size(),
                                     via_edge,
                                     isThroughStreet(obvious_index, intersection),
                                     intersection[obvious_index]);

        // assign left/right turns
        intersection = assignLeftTurns(via_edge, std::move(intersection), obvious_index + 1);
        intersection = assignRightTurns(via_edge, std::move(intersection), obvious_index);
    }
    else if (fork_range.first != 0 && fork_range.second - fork_range.first <= 2) // found fork
    {
        if (fork_range.second - fork_range.first == 1)
        {
            auto &left = intersection[fork_range.second];
            auto &right = intersection[fork_range.first];
            const auto left_classification =
                node_based_graph.GetEdgeData(left.turn.eid).road_classification;
            const auto right_classification =
                node_based_graph.GetEdgeData(right.turn.eid).road_classification;
            if (canBeSeenAsFork(left_classification, right_classification))
                assignFork(via_edge, left, right);
            else if (left_classification.GetPriority() > right_classification.GetPriority())
            {
                right.turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, false, right);
                left.turn.instruction = {findBasicTurnType(via_edge, left),
                                         DirectionModifier::SlightLeft};
            }
            else
            {
                left.turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, false, left);
                right.turn.instruction = {findBasicTurnType(via_edge, right),
                                          DirectionModifier::SlightRight};
            }
        }
        else if (fork_range.second - fork_range.first == 2)
        {
            assignFork(via_edge,
                       intersection[fork_range.second],
                       intersection[fork_range.first + 1],
                       intersection[fork_range.first]);
        }
        // assign left/right turns
        intersection = assignLeftTurns(via_edge, std::move(intersection), fork_range.second + 1);
        intersection = assignRightTurns(via_edge, std::move(intersection), fork_range.first);
    }
    else if (straightmost_deviation < FUZZY_ANGLE_DIFFERENCE &&
             !intersection[straightmost_turn].entry_allowed)
    {
        // invalid straight turn
        intersection = assignLeftTurns(via_edge, std::move(intersection), straightmost_turn + 1);
        intersection = assignRightTurns(via_edge, std::move(intersection), straightmost_turn);
    }
    // no straight turn
    else if (intersection[straightmost_turn].turn.angle > 180)
    {
        // at most three turns on either side
        intersection = assignLeftTurns(via_edge, std::move(intersection), straightmost_turn);
        intersection = assignRightTurns(via_edge, std::move(intersection), straightmost_turn);
    }
    else if (intersection[straightmost_turn].turn.angle < 180)
    {
        intersection = assignLeftTurns(via_edge, std::move(intersection), straightmost_turn + 1);
        intersection = assignRightTurns(via_edge, std::move(intersection), straightmost_turn + 1);
    }
    else
    {
        assignTrivialTurns(via_edge, intersection, 1, intersection.size());
    }
    return intersection;
}

// Assignment of left turns hands of to right turns.
// To do so, we mirror every road segment and reverse the order.
// After the mirror and reversal / we assign right turns and
// mirror again and restore the original order.
Intersection TurnHandler::assignLeftTurns(const EdgeID via_edge,
                                          Intersection intersection,
                                          const std::size_t starting_at) const
{
    BOOST_ASSERT(starting_at <= intersection.size());
    const auto switch_left_and_right = [](Intersection &intersection) {
        BOOST_ASSERT(!intersection.empty());

        for (auto &road : intersection)
            road = mirror(std::move(road));

        std::reverse(intersection.begin() + 1, intersection.end());
    };

    switch_left_and_right(intersection);
    // account for the u-turn in the beginning
    const auto count = intersection.size() - starting_at + 1;
    intersection = assignRightTurns(via_edge, std::move(intersection), count);
    switch_left_and_right(intersection);

    return intersection;
}

// can only assign three turns
Intersection TurnHandler::assignRightTurns(const EdgeID via_edge,
                                           Intersection intersection,
                                           const std::size_t up_to) const
{
    BOOST_ASSERT(up_to <= intersection.size());
    const auto count_valid = [&intersection, up_to]() {
        std::size_t count = 0;
        for (std::size_t i = 1; i < up_to; ++i)
            if (intersection[i].entry_allowed)
                ++count;
        return count;
    };
    if (up_to <= 1 || count_valid() == 0)
        return intersection;
    // handle single turn
    if (up_to == 2)
    {
        assignTrivialTurns(via_edge, intersection, 1, up_to);
    }
    // Handle Turns 1-3
    else if (up_to == 3)
    {
        const auto first_direction = getTurnDirection(intersection[1].turn.angle);
        const auto second_direction = getTurnDirection(intersection[2].turn.angle);
        if (first_direction == second_direction)
        {
            // conflict
            handleDistinctConflict(via_edge, intersection[2], intersection[1]);
        }
        else
        {
            assignTrivialTurns(via_edge, intersection, 1, up_to);
        }
    }
    // Handle Turns 1-4
    else if (up_to == 4)
    {
        const auto first_direction = getTurnDirection(intersection[1].turn.angle);
        const auto second_direction = getTurnDirection(intersection[2].turn.angle);
        const auto third_direction = getTurnDirection(intersection[3].turn.angle);
        if (first_direction != second_direction && second_direction != third_direction)
        {
            // due to the circular order, the turn directions are unique
            // first_direction != third_direction is implied
            BOOST_ASSERT(first_direction != third_direction);
            assignTrivialTurns(via_edge, intersection, 1, up_to);
        }
        else if (2 >= (intersection[1].entry_allowed + intersection[2].entry_allowed +
                       intersection[3].entry_allowed))
        {
            // at least a single invalid
            if (!intersection[3].entry_allowed)
            {
                handleDistinctConflict(via_edge, intersection[2], intersection[1]);
            }
            else if (!intersection[1].entry_allowed)
            {
                handleDistinctConflict(via_edge, intersection[3], intersection[2]);
            }
            else // handles one-valid as well as two valid (1,3)
            {
                handleDistinctConflict(via_edge, intersection[3], intersection[1]);
            }
        }
        // From here on out, intersection[1-3].entry_allowed has to be true (Otherwise we would have
        // triggered 2>= ...)
        //
        // Conflicting Turns, but at least farther than what we call a narrow turn
        else if (angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) >=
                     NARROW_TURN_ANGLE &&
                 angularDeviation(intersection[2].turn.angle, intersection[3].turn.angle) >=
                     NARROW_TURN_ANGLE)
        {
            BOOST_ASSERT(intersection[1].entry_allowed && intersection[2].entry_allowed &&
                         intersection[3].entry_allowed);

            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                DirectionModifier::SharpRight};
            intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                DirectionModifier::Right};
            intersection[3].turn.instruction = {findBasicTurnType(via_edge, intersection[3]),
                                                DirectionModifier::SlightRight};
        }
        else if (((first_direction == second_direction && second_direction == third_direction) ||
                  (first_direction == second_direction &&
                   angularDeviation(intersection[2].turn.angle, intersection[3].turn.angle) <
                       GROUP_ANGLE) ||
                  (second_direction == third_direction &&
                   angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) <
                       GROUP_ANGLE)))
        {
            BOOST_ASSERT(intersection[1].entry_allowed && intersection[2].entry_allowed &&
                         intersection[3].entry_allowed);
            // count backwards from the slightest turn
            assignTrivialTurns(via_edge, intersection, 1, up_to);
        }
        else if (((first_direction == second_direction &&
                   angularDeviation(intersection[2].turn.angle, intersection[3].turn.angle) >=
                       GROUP_ANGLE) ||
                  (second_direction == third_direction &&
                   angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) >=
                       GROUP_ANGLE)))
        {
            BOOST_ASSERT(intersection[1].entry_allowed && intersection[2].entry_allowed &&
                         intersection[3].entry_allowed);

            if (angularDeviation(intersection[2].turn.angle, intersection[3].turn.angle) >=
                GROUP_ANGLE)
            {
                handleDistinctConflict(via_edge, intersection[2], intersection[1]);
                intersection[3].turn.instruction = {findBasicTurnType(via_edge, intersection[3]),
                                                    third_direction};
            }
            else
            {
                intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                    first_direction};
                handleDistinctConflict(via_edge, intersection[3], intersection[2]);
            }
        }
        else
        {
            assignTrivialTurns(via_edge, intersection, 1, up_to);
        }
    }
    else
    {
        assignTrivialTurns(via_edge, intersection, 1, up_to);
    }
    return intersection;
}

std::pair<std::size_t, std::size_t> TurnHandler::findFork(const EdgeID via_edge,
                                                          const Intersection &intersection) const
{

    std::size_t best = 0;
    double best_deviation = 180;

    // TODO handle road classes
    for (std::size_t i = 1; i < intersection.size(); ++i)
    {
        const double deviation = angularDeviation(intersection[i].turn.angle, STRAIGHT_ANGLE);
        if (intersection[i].entry_allowed && deviation < best_deviation)
        {
            best_deviation = deviation;
            best = i;
        }
    }
    if (best_deviation <= NARROW_TURN_ANGLE)
    {
        std::size_t left = best, right = best;
        while (left + 1 < intersection.size() &&
               (angularDeviation(intersection[left + 1].turn.angle, STRAIGHT_ANGLE) <=
                    NARROW_TURN_ANGLE ||
                (angularDeviation(intersection[left].turn.angle,
                                  intersection[left + 1].turn.angle) <= NARROW_TURN_ANGLE &&
                 angularDeviation(intersection[left].turn.angle, STRAIGHT_ANGLE) <= GROUP_ANGLE)))
            ++left;
        while (
            right > 1 &&
            (angularDeviation(intersection[right - 1].turn.angle, STRAIGHT_ANGLE) <=
                 NARROW_TURN_ANGLE ||
             (angularDeviation(intersection[right].turn.angle, intersection[right - 1].turn.angle) <
                  NARROW_TURN_ANGLE &&
              angularDeviation(intersection[right - 1].turn.angle, STRAIGHT_ANGLE) <= GROUP_ANGLE)))
            --right;

        if (left == right)
            return std::make_pair(std::size_t{0}, std::size_t{0});

        const bool valid_indices = 0 < right && right < left;
        const bool separated_at_left_side =
            angularDeviation(intersection[left].turn.angle,
                             intersection[(left + 1) % intersection.size()].turn.angle) >=
            GROUP_ANGLE;
        const bool separated_at_right_side =
            right > 0 &&
            angularDeviation(intersection[right].turn.angle, intersection[right - 1].turn.angle) >=
                GROUP_ANGLE;

        const bool not_more_than_three = (left - right) <= 2;
        const bool has_obvious = [&]() {
            if (left - right == 1)
            {
                return isObviousOfTwo(via_edge, intersection[left], intersection[right]) ||
                       isObviousOfTwo(via_edge, intersection[right], intersection[left]);
            }
            else if (left - right == 2)
            {
                return isObviousOfTwo(via_edge, intersection[right + 1], intersection[right]) ||
                       isObviousOfTwo(via_edge, intersection[right], intersection[right + 1]) ||
                       isObviousOfTwo(via_edge, intersection[left], intersection[right + 1]) ||
                       isObviousOfTwo(via_edge, intersection[right + 1], intersection[left]);
            }
            return false;
        }();

        // A fork can only happen between edges of similar types where none of the ones is obvious
        const bool has_compatible_classes = [&]() {
            const bool ramp_class = node_based_graph.GetEdgeData(intersection[right].turn.eid)
                                        .road_classification.IsLinkClass();
            for (std::size_t index = right + 1; index <= left; ++index)
                if (ramp_class !=
                    node_based_graph.GetEdgeData(intersection[index].turn.eid)
                        .road_classification.IsLinkClass())
                    return false;

            const auto in_classification =
                node_based_graph.GetEdgeData(intersection[0].turn.eid).road_classification;
            for (std::size_t base_index = right; base_index <= left; ++base_index)
            {
                const auto base_classification =
                    node_based_graph.GetEdgeData(intersection[base_index].turn.eid)
                        .road_classification;
                for (std::size_t compare_index = right; compare_index <= left; ++compare_index)
                {
                    if (base_index == compare_index)
                        continue;

                    const auto compare_classification =
                        node_based_graph.GetEdgeData(intersection[compare_index].turn.eid)
                            .road_classification;
                    if (obviousByRoadClass(
                            in_classification, base_classification, compare_classification))
                        return false;
                }
            }
            return true;
        }();

        // check if all entries in the fork range allow entry
        const bool only_valid_entries = [&]() {
            BOOST_ASSERT(right <= left && left < intersection.size());

            // one past the end of the fork range
            const auto end_itr = intersection.begin() + left + 1;

            const auto has_entry_forbidden = [](const ConnectedRoad &road) {
                return !road.entry_allowed;
            };

            const auto first_disallowed_entry =
                std::find_if(intersection.begin() + right, end_itr, has_entry_forbidden);
            // if no entry was found that forbids entry, the intersection entries are all valid.
            return first_disallowed_entry == end_itr;
        }();

        // TODO check whether 2*NARROW_TURN is too large
        if (valid_indices && separated_at_left_side && separated_at_right_side &&
            not_more_than_three && !has_obvious && has_compatible_classes && only_valid_entries)
            return std::make_pair(right, left);
    }
    return std::make_pair(std::size_t{0}, std::size_t{0});
}

void TurnHandler::handleDistinctConflict(const EdgeID via_edge,
                                         ConnectedRoad &left,
                                         ConnectedRoad &right) const
{
    // single turn of both is valid (don't change the valid one)
    // or multiple identical angles -> bad OSM intersection
    if ((!left.entry_allowed || !right.entry_allowed) || (left.turn.angle == right.turn.angle))
    {
        if (left.entry_allowed)
            left.turn.instruction = {findBasicTurnType(via_edge, left),
                                     getTurnDirection(left.turn.angle)};
        if (right.entry_allowed)
            right.turn.instruction = {findBasicTurnType(via_edge, right),
                                      getTurnDirection(right.turn.angle)};
        return;
    }

    if (getTurnDirection(left.turn.angle) == DirectionModifier::Straight ||
        getTurnDirection(left.turn.angle) == DirectionModifier::SlightLeft ||
        getTurnDirection(right.turn.angle) == DirectionModifier::SlightRight)
    {
        const auto left_classification =
            node_based_graph.GetEdgeData(left.turn.eid).road_classification;
        const auto right_classification =
            node_based_graph.GetEdgeData(right.turn.eid).road_classification;
        if (canBeSeenAsFork(left_classification, right_classification))
            assignFork(via_edge, left, right);
        else if (left_classification.GetPriority() > right_classification.GetPriority())
        {
            // FIXME this should possibly know about the actual roads?
            // here we don't know about the intersection size. To be on the save side,
            // we declare it
            // as complex (at least size 4)
            right.turn.instruction = getInstructionForObvious(4, via_edge, false, right);
            left.turn.instruction = {findBasicTurnType(via_edge, left),
                                     DirectionModifier::SlightLeft};
        }
        else
        {
            // FIXME this should possibly know about the actual roads?
            // here we don't know about the intersection size. To be on the save side,
            // we declare it
            // as complex (at least size 4)
            left.turn.instruction = getInstructionForObvious(4, via_edge, false, left);
            right.turn.instruction = {findBasicTurnType(via_edge, right),
                                      DirectionModifier::SlightRight};
        }
    }
    const auto left_type = findBasicTurnType(via_edge, left);
    const auto right_type = findBasicTurnType(via_edge, right);
    // Two Right Turns
    if (angularDeviation(left.turn.angle, 90) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        // Keep left perfect, shift right
        left.turn.instruction = {left_type, DirectionModifier::Right};
        right.turn.instruction = {right_type, DirectionModifier::SharpRight};
        return;
    }
    if (angularDeviation(right.turn.angle, 90) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        // Keep Right perfect, shift left
        left.turn.instruction = {left_type, DirectionModifier::SlightRight};
        right.turn.instruction = {right_type, DirectionModifier::Right};
        return;
    }
    // Two Right Turns
    if (angularDeviation(left.turn.angle, 270) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        // Keep left perfect, shift right
        left.turn.instruction = {left_type, DirectionModifier::Left};
        right.turn.instruction = {right_type, DirectionModifier::SlightLeft};
        return;
    }
    if (angularDeviation(right.turn.angle, 270) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        // Keep Right perfect, shift left
        left.turn.instruction = {left_type, DirectionModifier::SharpLeft};
        right.turn.instruction = {right_type, DirectionModifier::Left};
        return;
    }
    // Shift the lesser penalty
    if (getTurnDirection(left.turn.angle) == DirectionModifier::SharpLeft)
    {
        left.turn.instruction = {left_type, DirectionModifier::SharpLeft};
        right.turn.instruction = {right_type, DirectionModifier::Left};
        return;
    }
    if (getTurnDirection(right.turn.angle) == DirectionModifier::SharpRight)
    {
        left.turn.instruction = {left_type, DirectionModifier::Right};
        right.turn.instruction = {right_type, DirectionModifier::SharpRight};
        return;
    }

    if (getTurnDirection(left.turn.angle) == DirectionModifier::Right)
    {
        if (angularDeviation(left.turn.angle, 85) >= angularDeviation(right.turn.angle, 85))
        {
            left.turn.instruction = {left_type, DirectionModifier::Right};
            right.turn.instruction = {right_type, DirectionModifier::SharpRight};
        }
        else
        {
            left.turn.instruction = {left_type, DirectionModifier::SlightRight};
            right.turn.instruction = {right_type, DirectionModifier::Right};
        }
    }
    else
    {
        if (angularDeviation(left.turn.angle, 265) >= angularDeviation(right.turn.angle, 265))
        {
            left.turn.instruction = {left_type, DirectionModifier::SharpLeft};
            right.turn.instruction = {right_type, DirectionModifier::Left};
        }
        else
        {
            left.turn.instruction = {left_type, DirectionModifier::Left};
            right.turn.instruction = {right_type, DirectionModifier::SlightLeft};
        }
    }
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
