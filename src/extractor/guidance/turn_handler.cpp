#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/toolkit.hpp"
#include "extractor/guidance/turn_handler.hpp"

#include "util/simple_logger.hpp"

#include <limits>
#include <utility>

#include <boost/assert.hpp>

using EdgeData = osrm::util::NodeBasedDynamicGraph::EdgeData;

namespace osrm
{
namespace extractor
{
namespace guidance
{
namespace detail
{
inline FunctionalRoadClass roadClass(const ConnectedRoad &road,
                                     const util::NodeBasedDynamicGraph &graph)
{
    return graph.GetEdgeData(road.turn.eid).road_classification.road_class;
}

inline bool isRampClass(EdgeID eid, const util::NodeBasedDynamicGraph &node_based_graph)
{
    return isRampClass(node_based_graph.GetEdgeData(eid).road_classification.road_class);
}
}

TurnHandler::TurnHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                         const std::vector<QueryNode> &node_info_list,
                         const util::NameTable &name_table)
    : IntersectionHandler(node_based_graph, node_info_list, name_table)
{
}

TurnHandler::~TurnHandler() {}

bool TurnHandler::canProcess(const NodeID, const EdgeID, const Intersection &) const
{
    return true;
}

Intersection TurnHandler::
operator()(const NodeID, const EdgeID via_eid, Intersection intersection) const
{
    if (intersection.size() == 1)
        return handleOneWayTurn(std::move(intersection));

    if (intersection[0].entry_allowed)
    {
        intersection[0].turn.instruction = {findBasicTurnType(via_eid, intersection[0]),
                                            DirectionModifier::UTurn};
    }

    if (intersection.size() == 2)
        return handleTwoWayTurn(via_eid, std::move(intersection));

    if (intersection.size() == 3)
        return handleThreeWayTurn(via_eid, std::move(intersection));

    return handleComplexTurn(via_eid, std::move(intersection));
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

    if (intersection[1].turn.instruction.type == TurnType::Suppressed)
        intersection[1].turn.instruction.type = TurnType::NoTurn;

    return intersection;
}

Intersection TurnHandler::handleThreeWayTurn(const EdgeID via_edge, Intersection intersection) const
{
    BOOST_ASSERT(intersection[0].turn.angle < 0.001);
    const auto isObviousOfTwo = [this](const ConnectedRoad road, const ConnectedRoad other) {
        const auto first_class =
            node_based_graph.GetEdgeData(road.turn.eid).road_classification.road_class;
        const bool is_ramp = isRampClass(first_class);
        const auto second_class =
            node_based_graph.GetEdgeData(other.turn.eid).road_classification.road_class;
        const bool is_narrow_turn =
            angularDeviation(road.turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE;
        const bool other_turn_is_at_least_orthogonal =
            angularDeviation(other.turn.angle, STRAIGHT_ANGLE) > 85;
        const bool turn_is_perfectly_straight = angularDeviation(road.turn.angle, STRAIGHT_ANGLE) <
                                                std::numeric_limits<double>::epsilon();
        const bool is_obvious_by_road_class =
            (!is_ramp && (2 * getPriority(first_class) < getPriority(second_class))) ||
            (!isLowPriorityRoadClass(first_class) && isLowPriorityRoadClass(second_class));
        const bool is_much_narrower_than_other =
            angularDeviation(other.turn.angle, STRAIGHT_ANGLE) /
                angularDeviation(road.turn.angle, STRAIGHT_ANGLE) >
            INCREASES_BY_FOURTY_PERCENT;
        return (is_narrow_turn && other_turn_is_at_least_orthogonal) ||
               turn_is_perfectly_straight || is_much_narrower_than_other ||
               is_obvious_by_road_class;
    };

    /* Two nearly straight turns -> FORK
               OOOOOOO
             /
      IIIIII
             \
               OOOOOOO
     */
    if (angularDeviation(intersection[1].turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
        angularDeviation(intersection[2].turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE)
    {
        if (intersection[1].entry_allowed && intersection[2].entry_allowed)
        {
            const auto left_class = node_based_graph.GetEdgeData(intersection[2].turn.eid)
                                        .road_classification.road_class;
            const auto right_class = node_based_graph.GetEdgeData(intersection[1].turn.eid)
                                         .road_classification.road_class;
            if (canBeSeenAsFork(left_class, right_class))
            {
                assignFork(via_edge, intersection[2], intersection[1]);
            }
            else if (isObviousOfTwo(intersection[1], intersection[2]))
            {
                intersection[1].turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, false, intersection[1]);
                intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                    DirectionModifier::SlightLeft};
            }
            else if (isObviousOfTwo(intersection[2], intersection[1]))
            {
                intersection[2].turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, false, intersection[2]);
                intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                    DirectionModifier::SlightRight};
            }
            else
            {
                intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                    DirectionModifier::SlightRight};
                intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                    DirectionModifier::SlightLeft};
            }
        }
        else
        {
            if (intersection[1].entry_allowed)
                intersection[1].turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, false, intersection[1]);
            if (intersection[2].entry_allowed)
                intersection[2].turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, false, intersection[2]);
        }
    }
    /*  T Intersection

        OOOOOOO T OOOOOOOO
                I
                I
                I
     */
    else if (angularDeviation(intersection[1].turn.angle, 90) < NARROW_TURN_ANGLE &&
             angularDeviation(intersection[2].turn.angle, 270) < NARROW_TURN_ANGLE &&
             angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) >
                 NARROW_TURN_ANGLE)
    {
        if (intersection[1].entry_allowed)
        {
            if (TurnType::Ramp != findBasicTurnType(via_edge, intersection[1]))
                intersection[1].turn.instruction = {TurnType::EndOfRoad, DirectionModifier::Right};
            else
                intersection[1].turn.instruction = {TurnType::Ramp, DirectionModifier::Right};
        }
        if (intersection[2].entry_allowed)
        {
            if (TurnType::Ramp != findBasicTurnType(via_edge, intersection[2]))

                intersection[2].turn.instruction = {TurnType::EndOfRoad, DirectionModifier::Left};
            else
                intersection[2].turn.instruction = {TurnType::Ramp, DirectionModifier::Left};
        }
    }
    /* T Intersection, Cross left
                O
                O
                O
       IIIIIIII - OOOOOOOOOO
     */
    else if (angularDeviation(intersection[1].turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
             angularDeviation(intersection[2].turn.angle, 270) < NARROW_TURN_ANGLE &&
             angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) >
                 NARROW_TURN_ANGLE)
    {
        if (intersection[1].entry_allowed)
        {
            if (TurnType::Ramp != findBasicTurnType(via_edge, intersection[1]))
                intersection[1].turn.instruction =
                    getInstructionForObvious(intersection.size(), via_edge, false, intersection[1]);
            else
                intersection[1].turn.instruction = {TurnType::Ramp, DirectionModifier::Straight};
        }
        if (intersection[2].entry_allowed)
        {
            intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                DirectionModifier::Left};
        }
    }
    /* T Intersection, Cross right

       IIIIIIII T OOOOOOOOOO
                O
                O
                O
     */
    else if (angularDeviation(intersection[2].turn.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
             angularDeviation(intersection[1].turn.angle, 90) < NARROW_TURN_ANGLE &&
             angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) >
                 NARROW_TURN_ANGLE)
    {
        if (intersection[2].entry_allowed)
            intersection[2].turn.instruction =
                getInstructionForObvious(intersection.size(), via_edge, false, intersection[2]);
        if (intersection[1].entry_allowed)
            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                DirectionModifier::Right};
    }
    // merge onto a through street
    else if (INVALID_NAME_ID != node_based_graph.GetEdgeData(intersection[1].turn.eid).name_id &&
             node_based_graph.GetEdgeData(intersection[1].turn.eid).name_id ==
                 node_based_graph.GetEdgeData(intersection[2].turn.eid).name_id)
    {
        const auto findTurn = [isObviousOfTwo](const ConnectedRoad turn,
                                               const ConnectedRoad other) -> TurnInstruction {
            if (isObviousOfTwo(turn, other))
            {
                return {TurnType::Merge, turn.turn.angle < STRAIGHT_ANGLE
                                             ? DirectionModifier::SlightLeft
                                             : DirectionModifier::SlightRight};
            }
            else
            {
                return {TurnType::Turn, getTurnDirection(turn.turn.angle)};
            }
        };
        intersection[1].turn.instruction = findTurn(intersection[1], intersection[2]);
        intersection[2].turn.instruction = findTurn(intersection[2], intersection[1]);
    }
    // other street merges from the left
    else if (INVALID_NAME_ID != node_based_graph.GetEdgeData(via_edge).name_id &&
             node_based_graph.GetEdgeData(via_edge).name_id ==
                 node_based_graph.GetEdgeData(intersection[1].turn.eid).name_id)
    {
        if (isObviousOfTwo(intersection[1], intersection[2]))
        {
            intersection[1].turn.instruction =
                TurnInstruction::SUPPRESSED(DirectionModifier::Straight);
        }
        else
        {
            intersection[1].turn.instruction = {TurnType::Continue,
                                                getTurnDirection(intersection[1].turn.angle)};
        }
        intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                            getTurnDirection(intersection[2].turn.angle)};
    }
    // other street merges from the right
    else if (INVALID_NAME_ID != node_based_graph.GetEdgeData(via_edge).name_id &&
             node_based_graph.GetEdgeData(via_edge).name_id ==
                 node_based_graph.GetEdgeData(intersection[2].turn.eid).name_id)
    {
        if (isObviousOfTwo(intersection[2], intersection[1]))
        {
            intersection[2].turn.instruction =
                TurnInstruction::SUPPRESSED(DirectionModifier::Straight);
        }
        else
        {
            intersection[2].turn.instruction = {TurnType::Continue,
                                                getTurnDirection(intersection[2].turn.angle)};
        }
        intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                            getTurnDirection(intersection[1].turn.angle)};
    }
    else
    {
        if (isObviousOfTwo(intersection[1], intersection[2]))
        {
            intersection[1].turn.instruction =
                getInstructionForObvious(3, via_edge, false, intersection[1]);
        }
        else
        {
            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                getTurnDirection(intersection[1].turn.angle)};
        }

        if (isObviousOfTwo(intersection[2], intersection[1]))
        {
            intersection[2].turn.instruction =
                getInstructionForObvious(3, via_edge, false, intersection[2]);
        }
        else
        {
            intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                getTurnDirection(intersection[2].turn.angle)};
        }
    }
    return intersection;
}

Intersection TurnHandler::handleComplexTurn(const EdgeID via_edge, Intersection intersection) const
{
    static int fallback_count = 0;
    const std::size_t obvious_index = findObviousTurn(via_edge, intersection);
    const auto fork_range = findFork(intersection);
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
            getInstructionForObvious(intersection.size(), via_edge, isThroughStreet(obvious_index,intersection),
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
            const auto left_class =
                node_based_graph.GetEdgeData(left.turn.eid).road_classification.road_class;
            const auto right_class =
                node_based_graph.GetEdgeData(right.turn.eid).road_classification.road_class;
            if (canBeSeenAsFork(left_class, right_class))
                assignFork(via_edge, left, right);
            else if (getPriority(left_class) > getPriority(right_class))
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
        else if (fork_range.second - fork_range.second == 2)
        {
            assignFork(via_edge, intersection[fork_range.second],
                       intersection[fork_range.first + 1], intersection[fork_range.first]);
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
        if (fallback_count++ < 10)
        {
            util::SimpleLogger().Write(logWARNING)
                << "Resolved to keep fallback on complex turn assignment"
                << "Straightmost: " << straightmost_turn;
            ;
            for (const auto &road : intersection)
            {
                const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
                util::SimpleLogger().Write(logWARNING)
                    << "road: " << toString(road) << " Name: " << out_data.name_id
                    << " Road Class: " << (int)out_data.road_classification.road_class;
            }
        }
    }
    return intersection;
}

std::size_t TurnHandler::findObviousTurn(const EdgeID via_edge,
                                         const Intersection &intersection) const
{
    // no obvious road
    if (intersection.size() == 1)
        return 0;

    // a single non u-turn is obvious
    if (intersection.size() == 2)
        return 1;

    // at least three roads
    std::size_t best = 0;
    double best_deviation = 180;

    std::size_t best_continue = 0;
    double best_continue_deviation = 180;

    const EdgeData &in_data = node_based_graph.GetEdgeData(via_edge);
    for (std::size_t i = 1; i < intersection.size(); ++i)
    {
        const double deviation = angularDeviation(intersection[i].turn.angle, STRAIGHT_ANGLE);
        if (intersection[i].entry_allowed && deviation < best_deviation)
        {
            best_deviation = deviation;
            best = i;
        }

        const auto out_data = node_based_graph.GetEdgeData(intersection[i].turn.eid);
        if (intersection[i].entry_allowed && out_data.name_id == in_data.name_id &&
            deviation < best_continue_deviation)
        {
            best_continue_deviation = deviation;
            best_continue = i;
        }
    }

    if (best == 0)
        return 0;

    if (best_deviation >= 2 * NARROW_TURN_ANGLE)
        return 0;

    // TODO incorporate road class in decision
    if (best != 0 && best_deviation < MAXIMAL_ALLOWED_NO_TURN_DEVIATION)
    {
        return best;
    }

    // has no obvious continued road
    if (best_continue == 0 || true)
    {
        // Find left/right deviation
        const double left_deviation = angularDeviation(
            intersection[(best + 1) % intersection.size()].turn.angle, STRAIGHT_ANGLE);
        const double right_deviation =
            angularDeviation(intersection[best - 1].turn.angle, STRAIGHT_ANGLE);

        if (best_deviation < MAXIMAL_ALLOWED_NO_TURN_DEVIATION &&
            std::min(left_deviation, right_deviation) > FUZZY_ANGLE_DIFFERENCE)
            return best;

        // other narrow turns?
        if (angularDeviation(intersection[best - 1].turn.angle, STRAIGHT_ANGLE) <=
            FUZZY_ANGLE_DIFFERENCE)
            return 0;
        if (angularDeviation(intersection[(best + 1) % intersection.size()].turn.angle,
                             STRAIGHT_ANGLE) <= FUZZY_ANGLE_DIFFERENCE)
            return 0;

        // Well distinct turn that is nearly straight
        if (left_deviation / best_deviation >= DISTINCTION_RATIO &&
            right_deviation / best_deviation >= DISTINCTION_RATIO)
        {
            return best;
        }
    }

    return 0; // no obvious turn
}

// Can only assign three turns
Intersection TurnHandler::assignLeftTurns(const EdgeID via_edge,
                                          Intersection intersection,
                                          const std::size_t starting_at) const
{
    const auto count_valid = [&intersection, starting_at]() {
        std::size_t count = 0;
        for (std::size_t i = starting_at; i < intersection.size(); ++i)
            if (intersection[i].entry_allowed)
                ++count;
        return count;
    };
    if (starting_at == intersection.size() || count_valid() == 0)
        return intersection;
    // handle single turn
    if (intersection.size() - starting_at == 1)
    {
        if (!intersection[starting_at].entry_allowed)
            return intersection;

        if (angularDeviation(intersection[starting_at].turn.angle, STRAIGHT_ANGLE) >
                NARROW_TURN_ANGLE &&
            angularDeviation(intersection[starting_at].turn.angle, 0) > NARROW_TURN_ANGLE)
        {
            // assign left turn
            intersection[starting_at].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at]), DirectionModifier::Left};
        }
        else if (angularDeviation(intersection[starting_at].turn.angle, STRAIGHT_ANGLE) <=
                 NARROW_TURN_ANGLE)
        {
            intersection[starting_at].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at]),
                DirectionModifier::SlightLeft};
        }
        else
        {
            intersection[starting_at].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at]),
                DirectionModifier::SharpLeft};
        }
    }
    // two turns on at the side
    else if (intersection.size() - starting_at == 2)
    {
        const auto first_direction = getTurnDirection(intersection[starting_at].turn.angle);
        const auto second_direction = getTurnDirection(intersection[starting_at + 1].turn.angle);
        if (first_direction == second_direction)
        {
            // conflict
            handleDistinctConflict(via_edge, intersection[starting_at + 1],
                                   intersection[starting_at]);
        }
        else
        {
            intersection[starting_at].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at]), first_direction};
            intersection[starting_at + 1].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at + 1]), second_direction};
        }
    }
    else if (intersection.size() - starting_at == 3)
    {
        const auto first_direction = getTurnDirection(intersection[starting_at].turn.angle);
        const auto second_direction = getTurnDirection(intersection[starting_at + 1].turn.angle);
        const auto third_direction = getTurnDirection(intersection[starting_at + 2].turn.angle);
        if (first_direction != second_direction && second_direction != third_direction)
        {
            // implies first != third, based on the angles and clockwise order
            if (intersection[starting_at].entry_allowed)
                intersection[starting_at].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at]), first_direction};
            if (intersection[starting_at + 1].entry_allowed)
                intersection[starting_at + 1].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at + 1]), second_direction};
            if (intersection[starting_at + 2].entry_allowed)
                intersection[starting_at + 2].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at + 2]), third_direction};
        }
        else if (2 >= (intersection[starting_at].entry_allowed +
                       intersection[starting_at + 1].entry_allowed +
                       intersection[starting_at + 2].entry_allowed))
        {

            // at least one invalid turn
            if (!intersection[starting_at].entry_allowed)
            {
                handleDistinctConflict(via_edge, intersection[starting_at + 2],
                                       intersection[starting_at + 1]);
            }
            else if (!intersection[starting_at + 1].entry_allowed)
            {
                handleDistinctConflict(via_edge, intersection[starting_at + 2],
                                       intersection[starting_at]);
            }
            else
            {
                handleDistinctConflict(via_edge, intersection[starting_at + 1],
                                       intersection[starting_at]);
            }
        }
        else if (intersection[starting_at].entry_allowed &&
                 intersection[starting_at + 1].entry_allowed &&
                 intersection[starting_at + 2].entry_allowed &&
                 angularDeviation(intersection[starting_at].turn.angle,
                                  intersection[starting_at + 1].turn.angle) >= NARROW_TURN_ANGLE &&
                 angularDeviation(intersection[starting_at + 1].turn.angle,
                                  intersection[starting_at + 2].turn.angle) >= NARROW_TURN_ANGLE)
        {
            intersection[starting_at].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at]),
                DirectionModifier::SlightLeft};
            intersection[starting_at + 1].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at + 1]),
                DirectionModifier::Left};
            intersection[starting_at + 2].turn.instruction = {
                findBasicTurnType(via_edge, intersection[starting_at + 2]),
                DirectionModifier::SharpLeft};
        }
        else if (intersection[starting_at].entry_allowed &&
                 intersection[starting_at + 1].entry_allowed &&
                 intersection[starting_at + 2].entry_allowed &&
                 ((first_direction == second_direction && second_direction == third_direction) ||
                  (third_direction == second_direction &&
                   angularDeviation(intersection[starting_at].turn.angle,
                                    intersection[starting_at + 1].turn.angle) < GROUP_ANGLE) ||
                  (second_direction == first_direction &&
                   angularDeviation(intersection[starting_at + 1].turn.angle,
                                    intersection[starting_at + 2].turn.angle) < GROUP_ANGLE)))
        {
            intersection[starting_at].turn.instruction = {
                detail::isRampClass(intersection[starting_at].turn.eid, node_based_graph)
                    ? FirstRamp
                    : FirstTurn,
                second_direction};
            intersection[starting_at + 1].turn.instruction = {
                detail::isRampClass(intersection[starting_at + 1].turn.eid, node_based_graph)
                    ? SecondRamp
                    : SecondTurn,
                second_direction};
            intersection[starting_at + 2].turn.instruction = {
                detail::isRampClass(intersection[starting_at + 2].turn.eid, node_based_graph)
                    ? ThirdRamp
                    : ThirdTurn,
                second_direction};
        }
        else if (intersection[starting_at].entry_allowed &&
                 intersection[starting_at + 1].entry_allowed &&
                 intersection[starting_at + 2].entry_allowed &&
                 ((third_direction == second_direction &&
                   angularDeviation(intersection[starting_at].turn.angle,
                                    intersection[starting_at + 1].turn.angle) >= GROUP_ANGLE) ||
                  (second_direction == first_direction &&
                   angularDeviation(intersection[starting_at + 1].turn.angle,
                                    intersection[starting_at + 2].turn.angle) >= GROUP_ANGLE)))
        {
            // conflict one side with an additional very sharp turn
            if (angularDeviation(intersection[starting_at + 1].turn.angle,
                                 intersection[starting_at + 2].turn.angle) >= GROUP_ANGLE)
            {
                handleDistinctConflict(via_edge, intersection[starting_at + 1],
                                       intersection[starting_at]);
                intersection[starting_at + 2].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at + 2]), third_direction};
            }
            else
            {
                intersection[starting_at].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at]), first_direction};
                handleDistinctConflict(via_edge, intersection[starting_at + 2],
                                       intersection[starting_at + 1]);
            }
        }
        else if ((first_direction == second_direction &&
                  intersection[starting_at].entry_allowed !=
                      intersection[starting_at + 1].entry_allowed) ||
                 (second_direction == third_direction &&
                  intersection[starting_at + 1].entry_allowed !=
                      intersection[starting_at + 2].entry_allowed))
        {
            // no conflict, due to conflict being restricted to valid/invalid
            if (intersection[starting_at].entry_allowed)
                intersection[starting_at].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at]), first_direction};
            if (intersection[starting_at + 1].entry_allowed)
                intersection[starting_at + 1].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at + 1]), second_direction};
            if (intersection[starting_at + 2].entry_allowed)
                intersection[starting_at + 2].turn.instruction = {
                    findBasicTurnType(via_edge, intersection[starting_at + 2]), third_direction};
        }
        else
        {
            util::SimpleLogger().Write(logWARNING) << "Reached fallback for left turns, size 3";
            for (const auto road : intersection)
            {
                const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
                util::SimpleLogger().Write(logWARNING)
                    << "\troad: " << toString(road) << " Name: " << out_data.name_id
                    << " Road Class: " << (int)out_data.road_classification.road_class;
            }

            for (std::size_t i = starting_at; i < intersection.size(); ++i)
                if (intersection[i].entry_allowed)
                    intersection[i].turn.instruction = {
                        findBasicTurnType(via_edge, intersection[i]),
                        getTurnDirection(intersection[i].turn.angle)};
        }
    }
    else if (intersection.size() - starting_at == 4)
    {
        if (intersection[starting_at].entry_allowed)
            intersection[starting_at].turn.instruction = {
                detail::isRampClass(intersection[starting_at].turn.eid, node_based_graph)
                    ? FirstRamp
                    : FirstTurn,
                DirectionModifier::Left};
        if (intersection[starting_at + 1].entry_allowed)
            intersection[starting_at + 1].turn.instruction = {
                detail::isRampClass(intersection[starting_at + 1].turn.eid, node_based_graph)
                    ? SecondRamp
                    : SecondTurn,
                DirectionModifier::Left};
        if (intersection[starting_at + 2].entry_allowed)
            intersection[starting_at + 2].turn.instruction = {
                detail::isRampClass(intersection[starting_at + 2].turn.eid, node_based_graph)
                    ? ThirdRamp
                    : ThirdTurn,
                DirectionModifier::Left};
        if (intersection[starting_at + 3].entry_allowed)
            intersection[starting_at + 3].turn.instruction = {
                detail::isRampClass(intersection[starting_at + 3].turn.eid, node_based_graph)
                    ? FourthRamp
                    : FourthTurn,
                DirectionModifier::Left};
    }
    else
    {
        for (auto &road : intersection)
        {
            if (!road.entry_allowed)
                continue;
            road.turn.instruction = {detail::isRampClass(road.turn.eid, node_based_graph) ? Ramp
                                                                                          : Turn,
                                     getTurnDirection(road.turn.angle)};
        }
    }
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
        if (angularDeviation(intersection[1].turn.angle, STRAIGHT_ANGLE) > NARROW_TURN_ANGLE &&
            angularDeviation(intersection[1].turn.angle, 0) > NARROW_TURN_ANGLE)
        {
            // assign left turn
            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                DirectionModifier::Right};
        }
        else if (angularDeviation(intersection[1].turn.angle, STRAIGHT_ANGLE) <= NARROW_TURN_ANGLE)
        {
            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                DirectionModifier::SlightRight};
        }
        else
        {
            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                DirectionModifier::SharpRight};
        }
    }
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
            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                first_direction};
            intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                second_direction};
        }
    }
    else if (up_to == 4)
    {
        const auto first_direction = getTurnDirection(intersection[1].turn.angle);
        const auto second_direction = getTurnDirection(intersection[2].turn.angle);
        const auto third_direction = getTurnDirection(intersection[3].turn.angle);
        if (first_direction != second_direction && second_direction != third_direction)
        {
            if (intersection[1].entry_allowed)
                intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                    first_direction};
            if (intersection[2].entry_allowed)
                intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                    second_direction};
            if (intersection[3].entry_allowed)
                intersection[3].turn.instruction = {findBasicTurnType(via_edge, intersection[3]),
                                                    third_direction};
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
        else if (intersection[1].entry_allowed && intersection[2].entry_allowed &&
                 intersection[3].entry_allowed &&
                 angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) >=
                     NARROW_TURN_ANGLE &&
                 angularDeviation(intersection[2].turn.angle, intersection[3].turn.angle) >=
                     NARROW_TURN_ANGLE)
        {
            intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                DirectionModifier::SharpRight};
            intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                DirectionModifier::Right};
            intersection[3].turn.instruction = {findBasicTurnType(via_edge, intersection[3]),
                                                DirectionModifier::SlightRight};
        }
        else if (intersection[1].entry_allowed && intersection[2].entry_allowed &&
                 intersection[3].entry_allowed &&
                 ((first_direction == second_direction && second_direction == third_direction) ||
                  (first_direction == second_direction &&
                   angularDeviation(intersection[2].turn.angle, intersection[3].turn.angle) <
                       GROUP_ANGLE) ||
                  (second_direction == third_direction &&
                   angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) <
                       GROUP_ANGLE)))
        {
            intersection[1].turn.instruction = {
                detail::isRampClass(intersection[1].turn.eid, node_based_graph) ? ThirdRamp
                                                                                : ThirdTurn,
                second_direction};
            intersection[2].turn.instruction = {
                detail::isRampClass(intersection[2].turn.eid, node_based_graph) ? SecondRamp
                                                                                : SecondTurn,
                second_direction};
            intersection[3].turn.instruction = {
                detail::isRampClass(intersection[3].turn.eid, node_based_graph) ? FirstRamp
                                                                                : FirstTurn,
                second_direction};
        }
        else if (intersection[1].entry_allowed && intersection[2].entry_allowed &&
                 intersection[3].entry_allowed &&
                 ((first_direction == second_direction &&
                   angularDeviation(intersection[2].turn.angle, intersection[3].turn.angle) >=
                       GROUP_ANGLE) ||
                  (second_direction == third_direction &&
                   angularDeviation(intersection[1].turn.angle, intersection[2].turn.angle) >=
                       GROUP_ANGLE)))
        {
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
        else if ((first_direction == second_direction &&
                  intersection[1].entry_allowed != intersection[2].entry_allowed) ||
                 (second_direction == third_direction &&
                  intersection[2].entry_allowed != intersection[3].entry_allowed))
        {
            if (intersection[1].entry_allowed)
                intersection[1].turn.instruction = {findBasicTurnType(via_edge, intersection[1]),
                                                    first_direction};
            if (intersection[2].entry_allowed)
                intersection[2].turn.instruction = {findBasicTurnType(via_edge, intersection[2]),
                                                    second_direction};
            if (intersection[3].entry_allowed)
                intersection[3].turn.instruction = {findBasicTurnType(via_edge, intersection[3]),
                                                    third_direction};
        }
        else
        {
            util::SimpleLogger().Write(logWARNING)
                << "Reached fallback for right turns, size 3 "
                << " Valids: " << (intersection[1].entry_allowed + intersection[2].entry_allowed +
                                   intersection[3].entry_allowed);
            for (const auto road : intersection)
            {
                const auto &out_data = node_based_graph.GetEdgeData(road.turn.eid);
                util::SimpleLogger().Write(logWARNING)
                    << "\troad: " << toString(road) << " Name: " << out_data.name_id
                    << " Road Class: " << (int)out_data.road_classification.road_class;
            }

            for (std::size_t i = 1; i < up_to; ++i)
                if (intersection[i].entry_allowed)
                    intersection[i].turn.instruction = {
                        findBasicTurnType(via_edge, intersection[i]),
                        getTurnDirection(intersection[i].turn.angle)};
        }
    }
    else if (up_to == 5)
    {
        if (intersection[4].entry_allowed)
            intersection[4].turn.instruction = {
                detail::isRampClass(intersection[4].turn.eid, node_based_graph) ? FirstRamp
                                                                                : FirstTurn,
                DirectionModifier::Right};
        if (intersection[3].entry_allowed)
            intersection[3].turn.instruction = {
                detail::isRampClass(intersection[3].turn.eid, node_based_graph) ? SecondRamp
                                                                                : SecondTurn,
                DirectionModifier::Right};
        if (intersection[2].entry_allowed)
            intersection[2].turn.instruction = {
                detail::isRampClass(intersection[2].turn.eid, node_based_graph) ? ThirdRamp
                                                                                : ThirdTurn,
                DirectionModifier::Right};
        if (intersection[1].entry_allowed)
            intersection[1].turn.instruction = {
                detail::isRampClass(intersection[1].turn.eid, node_based_graph) ? FourthRamp
                                                                                : FourthTurn,
                DirectionModifier::Right};
    }
    else
    {
        for (std::size_t i = 1; i < up_to; ++i)
        {
            auto &road = intersection[i];
            if (!road.entry_allowed)
                continue;
            road.turn.instruction = {detail::isRampClass(road.turn.eid, node_based_graph) ? Ramp
                                                                                          : Turn,
                                     getTurnDirection(road.turn.angle)};
        }
    }
    return intersection;
}

std::pair<std::size_t, std::size_t> TurnHandler::findFork(const Intersection &intersection) const
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
        if (intersection[left].turn.angle >= 180)
        {
            // due to best > 1, we can safely decrement right
            --right;
            if (angularDeviation(intersection[right].turn.angle, STRAIGHT_ANGLE) >
                NARROW_TURN_ANGLE)
                return std::make_pair(std::size_t{0}, std::size_t{0});
        }
        else
        {
            ++left;
            if (left >= intersection.size() ||
                angularDeviation(intersection[left].turn.angle, STRAIGHT_ANGLE) > NARROW_TURN_ANGLE)
                return std::make_pair(std::size_t{0}, std::size_t{0});
        }
        while (left + 1 < intersection.size() &&
               angularDeviation(intersection[left].turn.angle, intersection[left + 1].turn.angle) <
                   NARROW_TURN_ANGLE)
            ++left;
        while (right > 1 &&
               angularDeviation(intersection[right].turn.angle,
                                intersection[right - 1].turn.angle) < NARROW_TURN_ANGLE)
            --right;

        // TODO check whether 2*NARROW_TURN is too large
        if (right < left &&
            angularDeviation(intersection[left].turn.angle,
                             intersection[(left + 1) % intersection.size()].turn.angle) >=
                2 * NARROW_TURN_ANGLE &&
            angularDeviation(intersection[right].turn.angle, intersection[right - 1].turn.angle) >=
                2 * NARROW_TURN_ANGLE)
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
        const auto left_class =
            node_based_graph.GetEdgeData(left.turn.eid).road_classification.road_class;
        const auto right_class =
            node_based_graph.GetEdgeData(right.turn.eid).road_classification.road_class;
        if (canBeSeenAsFork(left_class, right_class))
            assignFork(via_edge, left, right);
        else if (getPriority(left_class) > getPriority(right_class))
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
    // Both turns?
    if (TurnType::Ramp != left_type && TurnType::Ramp != right_type)
    {
        if (left.turn.angle < STRAIGHT_ANGLE)
        {
            left.turn.instruction = {TurnType::FirstTurn, getTurnDirection(left.turn.angle)};
            right.turn.instruction = {TurnType::SecondTurn, getTurnDirection(right.turn.angle)};
        }
        else
        {
            left.turn.instruction = {TurnType::SecondTurn, getTurnDirection(left.turn.angle)};
            right.turn.instruction = {TurnType::FirstTurn, getTurnDirection(right.turn.angle)};
        }
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
        if (angularDeviation(left.turn.angle, 90) > angularDeviation(right.turn.angle, 90))
        {
            left.turn.instruction = {left_type, DirectionModifier::SlightRight};
            right.turn.instruction = {right_type, DirectionModifier::Right};
        }
        else
        {
            left.turn.instruction = {left_type, DirectionModifier::Right};
            right.turn.instruction = {right_type, DirectionModifier::SharpRight};
        }
    }
    else
    {
        if (angularDeviation(left.turn.angle, 270) > angularDeviation(right.turn.angle, 270))
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
