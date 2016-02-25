#include "extractor/turn_analysis.hpp"

namespace osrm
{
namespace extractor
{

namespace turn_analysis
{
// configuration of turn classification
const bool constexpr INVERT = true;
const bool constexpr RESOLVE_TO_RIGHT = true;
const bool constexpr RESOLVE_TO_LEFT = false;

// what angle is interpreted as going straight
const double constexpr STRAIGHT_ANGLE = 180.;
// if a turn deviates this much from going straight, it will be kept straight
const double constexpr MAXIMAL_ALLOWED_NO_TURN_DEVIATION = 2.;
// angle that lies between two nearly indistinguishable roads
const double constexpr NARROW_TURN_ANGLE = 35.;
// angle difference that can be classified as straight, if its the only narrow turn
const double constexpr FUZZY_STRAIGHT_ANGLE = 15.;
const double constexpr DISTINCTION_RATIO = 2;

using EdgeData = util::NodeBasedDynamicGraph::EdgeData;

using engine::guidance::TurnPossibility;
using engine::guidance::TurnInstruction;
using engine::guidance::DirectionModifier;
using engine::guidance::TurnType;
using engine::guidance::FunctionalRoadClass;

using engine::guidance::classifyIntersection;
using engine::guidance::isLowPriorityRoadClass;
using engine::guidance::angularDeviation;
using engine::guidance::getTurnDirection;
using engine::guidance::getRepresentativeCoordinate;
using engine::guidance::isBasic;
using engine::guidance::isRampClass;
using engine::guidance::isUturn;
using engine::guidance::isConflict;
using engine::guidance::isSlightTurn;
using engine::guidance::isSlightModifier;
using engine::guidance::mirrorDirectionModifier;

std::vector<TurnCandidate>
getTurns(const NodeID from,
         const EdgeID via_edge,
         const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph,
         const std::vector<QueryNode> &node_info_list,
         const std::shared_ptr<RestrictionMap const> restriction_map,
         const std::unordered_set<NodeID> &barrier_nodes,
         const CompressedEdgeContainer &compressed_edge_container)
{
    auto turn_candidates = turn_analysis::getTurnCandidates(
        from, via_edge, node_based_graph, node_info_list, restriction_map, barrier_nodes,
        compressed_edge_container);
    turn_candidates =
        turn_analysis::setTurnTypes(from, via_edge, std::move(turn_candidates), node_based_graph);
#define PRINT_DEBUG_CANDIDATES 0
#if PRINT_DEBUG_CANDIDATES
    std::cout << "Initial Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                  << std::endl;
#endif
    turn_candidates = turn_analysis::optimizeCandidates(via_edge, std::move(turn_candidates),
                                                        node_based_graph, node_info_list);
#if PRINT_DEBUG_CANDIDATES
    std::cout << "Optimized Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                  << std::endl;
#endif
    turn_candidates =
        turn_analysis::suppressTurns(via_edge, std::move(turn_candidates), node_based_graph);
#if PRINT_DEBUG_CANDIDATES
    std::cout << "Suppressed Candidates:\n";
    for (auto tc : turn_candidates)
        std::cout << "\t" << tc.toString() << " "
                  << (int)node_based_graph->GetEdgeData(tc.eid).road_classification.road_class
                  << std::endl;
#endif
    return turn_candidates;
}

std::vector<TurnCandidate>
setTurnTypes(const NodeID from,
             const EdgeID via_edge,
             std::vector<TurnCandidate> turn_candidates,
             const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    NodeID turn_node = node_based_graph->GetTarget(via_edge);

    bool has_non_roundabout = false, has_roundabout_entry = false;
    for (auto &candidate : turn_candidates)
    {
        const EdgeID onto_edge = candidate.eid;
        const NodeID to_node = node_based_graph->GetTarget(onto_edge);

        const auto turn = AnalyzeTurn(from, via_edge, turn_node, onto_edge, to_node,
                                      candidate.angle, node_based_graph);

        if (candidate.valid && !entersRoundabout(turn))
            has_non_roundabout = true;
        else if (candidate.valid)
            has_roundabout_entry = true;

        auto confidence = getTurnConfidence(candidate.angle, turn);
        if (!candidate.valid)
            confidence *= 0.8; // makes invalid turns more likely to be resolved in conflicts
        candidate.instruction = turn;
        candidate.confidence = confidence;
    }

    if (has_non_roundabout && has_roundabout_entry)
    {
        for (auto &candidate : turn_candidates)
        {
            if (entersRoundabout(candidate.instruction))
            {
                if (candidate.instruction.type == TurnType::EnterRotary)
                    candidate.instruction.type = TurnType::EnterRotaryAtExit;
                if (candidate.instruction.type == TurnType::EnterRoundabout)
                    candidate.instruction.type = TurnType::EnterRoundaboutAtExit;
            }
        }
    }
    return turn_candidates;
}

std::vector<TurnCandidate>
optimizeRamps(const EdgeID via_edge,
              std::vector<TurnCandidate> turn_candidates,
              const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    EdgeID continue_eid = SPECIAL_EDGEID;
    double continue_angle = 0;
    const auto &in_edge_data = node_based_graph->GetEdgeData(via_edge);
    for (auto &candidate : turn_candidates)
    {
        if (candidate.instruction.direction_modifier == DirectionModifier::UTurn)
            continue;

        const auto &out_edge_data = node_based_graph->GetEdgeData(candidate.eid);
        if (out_edge_data.name_id == in_edge_data.name_id)
        {
            continue_eid = candidate.eid;
            continue_angle = candidate.angle;
            if (angularDeviation(candidate.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
                isRampClass(in_edge_data.road_classification.road_class))
                candidate.instruction = TurnType::Suppressed;
            break;
        }
    }

    if (continue_eid != SPECIAL_EDGEID)
    {
        bool to_the_right = true;
        for (auto &candidate : turn_candidates)
        {
            if (candidate.eid == continue_eid)
            {
                to_the_right = false;
                continue;
            }

            if (candidate.instruction.type != TurnType::Ramp)
                continue;

            if (isSlightModifier(candidate.instruction.direction_modifier))
                candidate.instruction.direction_modifier =
                    (to_the_right) ? DirectionModifier::SlightRight : DirectionModifier::SlightLeft;
        }
    }
    return turn_candidates;
}

TurnType checkForkAndEnd(const EdgeID via_eid,
                         const std::vector<TurnCandidate> &turn_candidates,
                         const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    if (turn_candidates.size() != 3 ||
        turn_candidates.front().instruction.direction_modifier != DirectionModifier::UTurn)
        return TurnType::Invalid;

    if (isOnRoundabout(turn_candidates[1].instruction))
    {
        BOOST_ASSERT(isOnRoundabout(turn_candidates[2].instruction));
        return TurnType::Invalid;
    }
    BOOST_ASSERT(!isOnRoundabout(turn_candidates[2].instruction));

    FunctionalRoadClass road_classes[3] = {
        node_based_graph->GetEdgeData(via_eid).road_classification.road_class,
        node_based_graph->GetEdgeData(turn_candidates[1].eid).road_classification.road_class,
        node_based_graph->GetEdgeData(turn_candidates[2].eid).road_classification.road_class};

    if (angularDeviation(turn_candidates[1].angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
        angularDeviation(turn_candidates[2].angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE)
    {
        if (road_classes[0] != road_classes[1] || road_classes[1] != road_classes[2])
            return TurnType::Invalid;

        if (turn_candidates[1].valid && turn_candidates[2].valid)
            return TurnType::Fork;
    }

    else if (angularDeviation(turn_candidates[1].angle, 90) < NARROW_TURN_ANGLE &&
             angularDeviation(turn_candidates[2].angle, 270) < NARROW_TURN_ANGLE)
    {
        return TurnType::EndOfRoad;
    }

    return TurnType::Invalid;
}

std::vector<TurnCandidate> handleForkAndEnd(const TurnType type,
                                            std::vector<TurnCandidate> turn_candidates)
{
    turn_candidates[1].instruction.type = type;
    turn_candidates[1].instruction.direction_modifier =
        (type == TurnType::Fork) ? DirectionModifier::SlightRight : DirectionModifier::Right;
    turn_candidates[2].instruction.type = type;
    turn_candidates[2].instruction.direction_modifier =
        (type == TurnType::Fork) ? DirectionModifier::SlightLeft : DirectionModifier::Left;
    return turn_candidates;
}

// requires sorted candidates
std::vector<TurnCandidate>
optimizeCandidates(const EdgeID via_eid,
                   std::vector<TurnCandidate> turn_candidates,
                   const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph,
                   const std::vector<QueryNode> &node_info_list)
{
    BOOST_ASSERT_MSG(std::is_sorted(turn_candidates.begin(), turn_candidates.end(),
                                    [](const TurnCandidate &left, const TurnCandidate &right)
                                    {
                                        return left.angle < right.angle;
                                    }),
                     "Turn Candidates not sorted by angle.");
    if (turn_candidates.size() <= 1)
        return turn_candidates;

    TurnType type = checkForkAndEnd(via_eid, turn_candidates, node_based_graph);
    if (type != TurnType::Invalid)
        return handleForkAndEnd(type, std::move(turn_candidates));

    turn_candidates = optimizeRamps(via_eid, std::move(turn_candidates), node_based_graph);

    const auto getLeft = [&turn_candidates](std::size_t index)
    {
        return (index + 1) % turn_candidates.size();
    };
    const auto getRight = [&turn_candidates](std::size_t index)
    {
        return (index + turn_candidates.size() - 1) % turn_candidates.size();
    };

    // handle availability of multiple u-turns (e.g. street with separated small parking roads)
    if (isUturn(turn_candidates[0].instruction) && turn_candidates[0].angle == 0)
    {
        if (isUturn(turn_candidates[getLeft(0)].instruction))
            turn_candidates[getLeft(0)].instruction.direction_modifier =
                DirectionModifier::SharpLeft;
        if (isUturn(turn_candidates[getRight(0)].instruction))
            turn_candidates[getRight(0)].instruction.direction_modifier =
                DirectionModifier::SharpRight;
    }

    const auto keepStraight = [](double angle)
    {
        return std::abs(angle - 180) < 5;
    };

    for (std::size_t turn_index = 0; turn_index < turn_candidates.size(); ++turn_index)
    {
        auto &turn = turn_candidates[turn_index];
        if (!isBasic(turn.instruction.type) || isUturn(turn.instruction) ||
            isOnRoundabout(turn.instruction))
            continue;
        auto &left = turn_candidates[getLeft(turn_index)];
        if (turn.angle == left.angle)
        {
            util::SimpleLogger().Write(logDEBUG)
                << "[warning] conflicting turn angles, identical road duplicated? "
                << node_info_list[node_based_graph->GetTarget(via_eid)].lat << " "
                << node_info_list[node_based_graph->GetTarget(via_eid)].lon << std::endl;
        }
        if (isConflict(turn.instruction, left.instruction))
        {
            // begin of a conflicting region
            std::size_t conflict_begin = turn_index;
            std::size_t conflict_end = getLeft(turn_index);
            std::size_t conflict_size = 2;
            while (
                isConflict(turn_candidates[getLeft(conflict_end)].instruction, turn.instruction) &&
                conflict_size < turn_candidates.size())
            {
                conflict_end = getLeft(conflict_end);
                ++conflict_size;
            }

            turn_index = (conflict_end < conflict_begin) ? turn_candidates.size() : conflict_end;

            if (conflict_size > 3)
            {
                // check if some turns are invalid to find out about good handling
            }

            auto &instruction_left_of_end = turn_candidates[getLeft(conflict_end)].instruction;
            auto &instruction_right_of_begin =
                turn_candidates[getRight(conflict_begin)].instruction;
            auto &candidate_at_end = turn_candidates[conflict_end];
            auto &candidate_at_begin = turn_candidates[conflict_begin];
            if (conflict_size == 2)
            {
                if (turn.instruction.direction_modifier == DirectionModifier::Straight)
                {
                    if (instruction_left_of_end.direction_modifier !=
                            DirectionModifier::SlightLeft &&
                        instruction_right_of_begin.direction_modifier !=
                            DirectionModifier::SlightRight)
                    {
                        std::int32_t resolved_count = 0;
                        // uses side-effects in resolve
                        if (!keepStraight(candidate_at_end.angle) &&
                            !resolve(candidate_at_end.instruction, instruction_left_of_end,
                                     RESOLVE_TO_LEFT))
                            util::SimpleLogger().Write(logDEBUG)
                                << "[warning] failed to resolve conflict";
                        else
                            ++resolved_count;
                        // uses side-effects in resolve
                        if (!keepStraight(candidate_at_begin.angle) &&
                            !resolve(candidate_at_begin.instruction, instruction_right_of_begin,
                                     RESOLVE_TO_RIGHT))
                            util::SimpleLogger().Write(logDEBUG)
                                << "[warning] failed to resolve conflict";
                        else
                            ++resolved_count;
                        if (resolved_count >= 1 &&
                            (!keepStraight(candidate_at_begin.angle) ||
                             !keepStraight(candidate_at_end.angle))) // should always be the
                                                                     // case, theoretically
                            continue;
                    }
                }
                if (candidate_at_begin.confidence < candidate_at_end.confidence)
                { // if right shift is cheaper, or only option
                    if (resolve(candidate_at_begin.instruction, instruction_right_of_begin,
                                RESOLVE_TO_RIGHT))
                        continue;
                    else if (resolve(candidate_at_end.instruction, instruction_left_of_end,
                                     RESOLVE_TO_LEFT))
                        continue;
                }
                else
                {
                    if (resolve(candidate_at_end.instruction, instruction_left_of_end,
                                RESOLVE_TO_LEFT))
                        continue;
                    else if (resolve(candidate_at_begin.instruction, instruction_right_of_begin,
                                     RESOLVE_TO_RIGHT))
                        continue;
                }
                if (isSlightTurn(turn.instruction) || isSharpTurn(turn.instruction))
                {
                    auto resolve_direction =
                        (turn.instruction.direction_modifier == DirectionModifier::SlightRight ||
                         turn.instruction.direction_modifier == DirectionModifier::SharpLeft)
                            ? RESOLVE_TO_RIGHT
                            : RESOLVE_TO_LEFT;
                    if (resolve_direction == RESOLVE_TO_RIGHT &&
                        resolveTransitive(
                            candidate_at_begin.instruction, instruction_right_of_begin,
                            turn_candidates[getRight(getRight(conflict_begin))].instruction,
                            RESOLVE_TO_RIGHT))
                        continue;
                    else if (resolve_direction == RESOLVE_TO_LEFT &&
                             resolveTransitive(
                                 candidate_at_end.instruction, instruction_left_of_end,
                                 turn_candidates[getLeft(getLeft(conflict_end))].instruction,
                                 RESOLVE_TO_LEFT))
                        continue;
                }
            }
            else if (conflict_size >= 3)
            {
                // a conflict of size larger than three cannot be handled with the current
                // model.
                // Handle it as best as possible and keep the rest of the conflicting turns
                if (conflict_size > 3)
                {
                    NodeID conflict_location = node_based_graph->GetTarget(via_eid);
                    util::SimpleLogger().Write(logDEBUG)
                        << "[warning] found conflict larget than size three at "
                        << node_info_list[conflict_location].lat << ", "
                        << node_info_list[conflict_location].lon;
                }

                if (!resolve(candidate_at_begin.instruction, instruction_right_of_begin,
                             RESOLVE_TO_RIGHT))
                {
                    if (isSlightTurn(turn.instruction))
                        resolveTransitive(
                            candidate_at_begin.instruction, instruction_right_of_begin,
                            turn_candidates[getRight(getRight(conflict_begin))].instruction,
                            RESOLVE_TO_RIGHT);
                    else if (isSharpTurn(turn.instruction))
                        resolveTransitive(
                            candidate_at_end.instruction, instruction_left_of_end,
                            turn_candidates[getLeft(getLeft(conflict_end))].instruction,
                            RESOLVE_TO_LEFT);
                }
                if (!resolve(candidate_at_end.instruction, instruction_left_of_end,
                             RESOLVE_TO_LEFT))
                {
                    if (isSlightTurn(turn.instruction))
                        resolveTransitive(
                            candidate_at_end.instruction, instruction_left_of_end,
                            turn_candidates[getLeft(getLeft(conflict_end))].instruction,
                            RESOLVE_TO_LEFT);
                    else if (isSharpTurn(turn.instruction))
                        resolveTransitive(
                            candidate_at_begin.instruction, instruction_right_of_begin,
                            turn_candidates[getRight(getRight(conflict_begin))].instruction,
                            RESOLVE_TO_RIGHT);
                }
            }
        }
    }
    return turn_candidates;
}

bool isObviousChoice(const EdgeID via_eid,
                     const std::size_t turn_index,
                     const std::vector<TurnCandidate> &turn_candidates,
                     const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    const auto getLeft = [&turn_candidates](std::size_t index)
    {
        return (index + 1) % turn_candidates.size();
    };
    const auto getRight = [&turn_candidates](std::size_t index)
    {
        return (index + turn_candidates.size() - 1) % turn_candidates.size();
    };
    const auto &candidate = turn_candidates[turn_index];
    const EdgeData &in_data = node_based_graph->GetEdgeData(via_eid);
    const EdgeData &out_data = node_based_graph->GetEdgeData(candidate.eid);
    const auto &candidate_to_the_left = turn_candidates[getLeft(turn_index)];

    const auto &candidate_to_the_right = turn_candidates[getRight(turn_index)];

    const auto hasValidRatio = [&](const TurnCandidate &left, const TurnCandidate &center,
                                   const TurnCandidate &right)
    {
        auto angle_left = (left.angle > 180) ? angularDeviation(left.angle, STRAIGHT_ANGLE) : 180;
        auto angle_right =
            (right.angle < 180) ? angularDeviation(right.angle, STRAIGHT_ANGLE) : 180;
        auto self_angle = angularDeviation(center.angle, STRAIGHT_ANGLE);
        return angularDeviation(center.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
               ((center.angle < STRAIGHT_ANGLE)
                    ? (angle_right > self_angle && angle_left / self_angle > DISTINCTION_RATIO)
                    : (angle_left > self_angle && angle_right / self_angle > DISTINCTION_RATIO));
    };
    // only valid turn
    if (!isLowPriorityRoadClass(
            node_based_graph->GetEdgeData(candidate.eid).road_classification.road_class))
    {
        bool is_only_normal_road = true;
        BOOST_ASSERT(turn_candidates[0].instruction.type == TurnType::Turn &&
                     turn_candidates[0].instruction.direction_modifier == DirectionModifier::UTurn);
        for (size_t i = 0; i < turn_candidates.size(); ++i)
        {
            if (i == turn_index || turn_candidates[i].angle == 0) // skip self and u-turn
                continue;
            if (!isLowPriorityRoadClass(node_based_graph->GetEdgeData(turn_candidates[i].eid)
                                            .road_classification.road_class))
            {
                is_only_normal_road = false;
                break;
            }
        }
        if (is_only_normal_road == true)
            return true;
    }

    return turn_candidates.size() == 1 ||
           // only non u-turn
           (turn_candidates.size() == 2 &&
            isUturn(candidate_to_the_left.instruction)) || // nearly straight turn
           angularDeviation(candidate.angle, STRAIGHT_ANGLE) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION ||
           hasValidRatio(candidate_to_the_left, candidate, candidate_to_the_right) ||
           (in_data.name_id != 0 && in_data.name_id == out_data.name_id &&
            angularDeviation(candidate.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE / 2);
}

std::vector<TurnCandidate>
suppressTurns(const EdgeID via_eid,
              std::vector<TurnCandidate> turn_candidates,
              const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{
    if (turn_candidates.size() == 3)
    {
        BOOST_ASSERT(turn_candidates[0].instruction.direction_modifier ==
                     DirectionModifier::UTurn);
        if (isLowPriorityRoadClass(node_based_graph->GetEdgeData(turn_candidates[1].eid)
                                       .road_classification.road_class) &&
            !isLowPriorityRoadClass(node_based_graph->GetEdgeData(turn_candidates[2].eid)
                                        .road_classification.road_class))
        {
            if (angularDeviation(turn_candidates[2].angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE)
            {
                if (node_based_graph->GetEdgeData(turn_candidates[2].eid).name_id ==
                    node_based_graph->GetEdgeData(via_eid).name_id)
                {
                    turn_candidates[2].instruction = TurnInstruction::NO_TURN();
                }
                else
                {
                    turn_candidates[2].instruction.type = TurnType::NewName;
                }
                return turn_candidates;
            }
        }
        else if (isLowPriorityRoadClass(node_based_graph->GetEdgeData(turn_candidates[2].eid)
                                            .road_classification.road_class) &&
                 !isLowPriorityRoadClass(node_based_graph->GetEdgeData(turn_candidates[1].eid)
                                             .road_classification.road_class))
        {
            if (angularDeviation(turn_candidates[1].angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE)
            {
                if (node_based_graph->GetEdgeData(turn_candidates[1].eid).name_id ==
                    node_based_graph->GetEdgeData(via_eid).name_id)
                {
                    turn_candidates[1].instruction = TurnInstruction::NO_TURN();
                }
                else
                {
                    turn_candidates[1].instruction.type = TurnType::NewName;
                }
                return turn_candidates;
            }
        }
    }

    BOOST_ASSERT_MSG(std::is_sorted(turn_candidates.begin(), turn_candidates.end(),
                                    [](const TurnCandidate &left, const TurnCandidate &right)
                                    {
                                        return left.angle < right.angle;
                                    }),
                     "Turn Candidates not sorted by angle.");

    const auto getLeft = [&turn_candidates](std::size_t index)
    {
        return (index + 1) % turn_candidates.size();
    };
    const auto getRight = [&turn_candidates](std::size_t index)
    {
        return (index + turn_candidates.size() - 1) % turn_candidates.size();
    };

    const EdgeData &in_data = node_based_graph->GetEdgeData(via_eid);

    bool has_obvious_with_same_name = false;
    double obvious_with_same_name_angle = 0;
    for (std::size_t turn_index = 0; turn_index < turn_candidates.size(); ++turn_index)
    {
        if (node_based_graph->GetEdgeData(turn_candidates[turn_index].eid).name_id ==
                in_data.name_id &&
            isObviousChoice(via_eid, turn_index, turn_candidates, node_based_graph))
        {
            has_obvious_with_same_name = true;
            obvious_with_same_name_angle = turn_candidates[turn_index].angle;
            break;
        }
    }

    for (std::size_t turn_index = 0; turn_index < turn_candidates.size(); ++turn_index)
    {
        auto &candidate = turn_candidates[turn_index];
        if (!isBasic(candidate.instruction.type))
            continue;

        const EdgeData &out_data = node_based_graph->GetEdgeData(candidate.eid);
        if (out_data.name_id == in_data.name_id && in_data.name_id != 0 &&
            candidate.instruction.direction_modifier != DirectionModifier::UTurn &&
            !has_obvious_with_same_name)
        {
            candidate.instruction.type = TurnType::Continue;
        }
        if (candidate.valid && !isUturn(candidate.instruction))
        {
            // TODO road category would be useful to indicate obviousness of turn
            // check if turn can be omitted or at least changed
            const auto &left = turn_candidates[getLeft(turn_index)];
            const auto &right = turn_candidates[getRight(turn_index)];

            // make very slight instructions straight, if they are the only valid choice going
            // with
            // at most a slight turn
            if ((!isSlightModifier(getTurnDirection(left.angle)) || !left.valid) &&
                (!isSlightModifier(getTurnDirection(right.angle)) || !right.valid) &&
                angularDeviation(candidate.angle, STRAIGHT_ANGLE) < FUZZY_STRAIGHT_ANGLE)
                candidate.instruction.direction_modifier = DirectionModifier::Straight;

            // TODO this smaller comparison for turns is DANGEROUS, has to be revised if turn
            // instructions change
            if (in_data.travel_mode ==
                out_data.travel_mode) // make sure to always announce mode changes
            {
                if (isObviousChoice(via_eid, turn_index, turn_candidates, node_based_graph))
                {

                    if (in_data.name_id == out_data.name_id) // same road
                    {
                        candidate.instruction.type = TurnType::Suppressed;
                    }

                    else if (!has_obvious_with_same_name)
                    {
                        // TODO discuss, we might want to keep the current name of the turn. But
                        // this would mean emitting a turn when you just keep on a road
                        if (isRampClass(in_data.road_classification.road_class) &&
                            !isRampClass(out_data.road_classification.road_class))
                        {
                            candidate.instruction.type = TurnType::Merge;
                            candidate.instruction.direction_modifier =
                                mirrorDirectionModifier(candidate.instruction.direction_modifier);
                        }
                        else
                        {
                            if (engine::guidance::canBeSuppressed(candidate.instruction.type))
                                candidate.instruction.type = TurnType::NewName;
                        }
                    }
                    else if (candidate.angle < obvious_with_same_name_angle)
                        candidate.instruction.direction_modifier = DirectionModifier::SlightRight;
                    else
                        candidate.instruction.direction_modifier = DirectionModifier::SlightLeft;
                }
                else if (candidate.instruction.direction_modifier == DirectionModifier::Straight &&
                         has_obvious_with_same_name)
                {
                    if (candidate.angle < obvious_with_same_name_angle)
                        candidate.instruction.direction_modifier = DirectionModifier::SlightRight;
                    else
                        candidate.instruction.direction_modifier = DirectionModifier::SlightLeft;
                }
            }
        }
    }
    return turn_candidates;
}

std::vector<TurnCandidate>
getTurnCandidates(const NodeID from_node,
                  const EdgeID via_eid,
                  const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph,
                  const std::vector<QueryNode> &node_info_list,
                  const std::shared_ptr<RestrictionMap const> restriction_map,
                  const std::unordered_set<NodeID> &barrier_nodes,
                  const CompressedEdgeContainer &compressed_edge_container)
{
    std::vector<TurnCandidate> turn_candidates;
    const NodeID turn_node = node_based_graph->GetTarget(via_eid);
    const NodeID only_restriction_to_node =
        restriction_map->CheckForEmanatingIsOnlyTurn(from_node, turn_node);
    const bool is_barrier_node = barrier_nodes.find(turn_node) != barrier_nodes.end();

    for (const EdgeID onto_edge : node_based_graph->GetAdjacentEdgeRange(turn_node))
    {
        bool turn_is_valid = true;
        if (node_based_graph->GetEdgeData(onto_edge).reversed)
        {
            turn_is_valid = false;
        }
        const NodeID to_node = node_based_graph->GetTarget(onto_edge);

        if (turn_is_valid && (only_restriction_to_node != SPECIAL_NODEID) &&
            (to_node != only_restriction_to_node))
        {
            // We are at an only_-restriction but not at the right turn.
            //            ++restricted_turns_counter;
            turn_is_valid = false;
        }

        if (turn_is_valid)
        {
            if (is_barrier_node)
            {
                if (from_node != to_node)
                {
                    //                    ++skipped_barrier_turns_counter;
                    turn_is_valid = false;
                }
            }
            else
            {
                if (from_node == to_node && node_based_graph->GetOutDegree(turn_node) > 1)
                {
                    auto number_of_emmiting_bidirectional_edges = 0;
                    for (auto edge : node_based_graph->GetAdjacentEdgeRange(turn_node))
                    {
                        auto target = node_based_graph->GetTarget(edge);
                        auto reverse_edge = node_based_graph->FindEdge(target, turn_node);
                        if (!node_based_graph->GetEdgeData(reverse_edge).reversed)
                        {
                            ++number_of_emmiting_bidirectional_edges;
                        }
                    }
                    if (number_of_emmiting_bidirectional_edges > 1)
                    {
                        //                        ++skipped_uturns_counter;
                        turn_is_valid = false;
                    }
                }
            }
        }

        // only add an edge if turn is not a U-turn except when it is
        // at the end of a dead-end street
        if (restriction_map->CheckIfTurnIsRestricted(from_node, turn_node, to_node) &&
            (only_restriction_to_node == SPECIAL_NODEID) && (to_node != only_restriction_to_node))
        {
            // We are at an only_-restriction but not at the right turn.
            //            ++restricted_turns_counter;
            turn_is_valid = false;
        }

        // unpack first node of second segment if packed

        const auto first_coordinate = getRepresentativeCoordinate(
            from_node, turn_node, via_eid, INVERT, compressed_edge_container, node_info_list);
        const auto third_coordinate = getRepresentativeCoordinate(
            turn_node, to_node, onto_edge, !INVERT, compressed_edge_container, node_info_list);

        const auto angle = util::coordinate_calculation::computeAngle(
            first_coordinate, node_info_list[turn_node], third_coordinate);

        turn_candidates.push_back(
            {onto_edge, turn_is_valid, angle, {TurnType::Invalid, DirectionModifier::UTurn}, 0});
    }

    const auto ByAngle = [](const TurnCandidate &first, const TurnCandidate second)
    {
        return first.angle < second.angle;
    };
    std::sort(std::begin(turn_candidates), std::end(turn_candidates), ByAngle);

    const auto getLeft = [&](std::size_t index)
    {
        return (index + 1) % turn_candidates.size();
    };

    const auto getRight = [&](std::size_t index)
    {
        return (index + turn_candidates.size() - 1) % turn_candidates.size();
    };

    const auto isInvalidEquivalent = [&](std::size_t this_turn, std::size_t valid_turn)
    {
        if (!turn_candidates[valid_turn].valid || turn_candidates[this_turn].valid)
            return false;

        return angularDeviation(turn_candidates[this_turn].angle,
                                turn_candidates[valid_turn].angle) < NARROW_TURN_ANGLE;
    };

    for (std::size_t index = 0; index < turn_candidates.size(); ++index)
    {
        if (isInvalidEquivalent(index, getRight(index)) ||
            isInvalidEquivalent(index, getLeft(index)))
        {
            turn_candidates.erase(turn_candidates.begin() + index);
            --index;
        }
    }
    return turn_candidates;
}

// node_u -- (edge_1) --> node_v -- (edge_2) --> node_w
TurnInstruction
AnalyzeTurn(const NodeID node_u,
            const EdgeID edge1,
            const NodeID node_v,
            const EdgeID edge2,
            const NodeID node_w,
            const double angle,
            const std::shared_ptr<const util::NodeBasedDynamicGraph> node_based_graph)
{

    const EdgeData &data1 = node_based_graph->GetEdgeData(edge1);
    const EdgeData &data2 = node_based_graph->GetEdgeData(edge2);
    bool from_ramp = isRampClass(data1.road_classification.road_class);
    bool to_ramp = isRampClass(data2.road_classification.road_class);
    if (node_u == node_w)
    {
        return {TurnType::Turn, DirectionModifier::UTurn};
    }

    // roundabouts need to be handled explicitely
    if (data1.roundabout && data2.roundabout)
    {
        // Is a turn possible? If yes, we stay on the roundabout!
        if (1 == node_based_graph->GetDirectedOutDegree(node_v))
        {
            // No turn possible.
            return TurnInstruction::NO_TURN();
        }
        return TurnInstruction::REMAIN_ROUNDABOUT(getTurnDirection(angle));
    }
    // Does turn start or end on roundabout?
    if (data1.roundabout || data2.roundabout)
    {
        // We are entering the roundabout
        if ((!data1.roundabout) && data2.roundabout)
        {
            return TurnInstruction::ENTER_ROUNDABOUT(getTurnDirection(angle));
        }
        // We are leaving the roundabout
        if (data1.roundabout && (!data2.roundabout))
        {
            return TurnInstruction::EXIT_ROUNDABOUT(getTurnDirection(angle));
        }
    }

    if (!from_ramp && to_ramp)
    {
        return {TurnType::Ramp, getTurnDirection(angle)};
    }

    // assign a designated turn angle instruction purely based on the angle
    return {TurnType::Turn, getTurnDirection(angle)};
}

} // namespace turn_analysis
} // namespace extractor
} // namespace osrm
