#include "extractor/guidance/turn_analysis.hpp"
#include "extractor/guidance/classification_data.hpp"
#include "extractor/guidance/constants.hpp"

#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/guidance/toolkit.hpp"
#include "util/simple_logger.hpp"

#include <cstddef>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

using osrm::util::guidance::getTurnDirection;

namespace osrm
{
namespace extractor
{
namespace guidance
{

using EdgeData = util::NodeBasedDynamicGraph::EdgeData;

bool requiresAnnouncement(const EdgeData &from, const EdgeData &to)
{
    return !from.IsCompatibleTo(to);
}

TurnAnalysis::TurnAnalysis(const util::NodeBasedDynamicGraph &node_based_graph,
                           const std::vector<QueryNode> &node_info_list,
                           const RestrictionMap &restriction_map,
                           const std::unordered_set<NodeID> &barrier_nodes,
                           const CompressedEdgeContainer &compressed_edge_container,
                           const util::NameTable &name_table,
                           const SuffixTable &street_name_suffix_table)
    : node_based_graph(node_based_graph), intersection_generator(node_based_graph,
                                                                 restriction_map,
                                                                 barrier_nodes,
                                                                 node_info_list,
                                                                 compressed_edge_container),
      roundabout_handler(node_based_graph,
                         node_info_list,
                         compressed_edge_container,
                         name_table,
                         street_name_suffix_table),
      motorway_handler(node_based_graph, node_info_list, name_table, street_name_suffix_table),
      turn_handler(node_based_graph, node_info_list, name_table, street_name_suffix_table)
{
}

Intersection TurnAnalysis::assignTurnTypes(const NodeID from_nid,
                                           const EdgeID via_eid,
                                           Intersection intersection) const
{
    // Roundabouts are a main priority. If there is a roundabout instruction present, we process the
    // turn as a roundabout
    if (roundabout_handler.canProcess(from_nid, via_eid, intersection))
    {
        intersection = roundabout_handler(from_nid, via_eid, std::move(intersection));
    }
    else
    {
        // set initial defaults for normal turns and modifier based on angle
        intersection = setTurnTypes(from_nid, via_eid, std::move(intersection));
        if (motorway_handler.canProcess(from_nid, via_eid, intersection))
        {
            intersection = motorway_handler(from_nid, via_eid, std::move(intersection));
        }
        else
        {
            BOOST_ASSERT(turn_handler.canProcess(from_nid, via_eid, intersection));
            intersection = turn_handler(from_nid, via_eid, std::move(intersection));
        }
    }
    // Handle sliproads
    intersection = handleSliproads(via_eid, std::move(intersection));

    // Turn On Ramps Into Off Ramps, if we come from a motorway-like road
    if (isMotorwayClass(node_based_graph.GetEdgeData(via_eid).road_classification.road_class))
    {
        std::for_each(intersection.begin(), intersection.end(), [](ConnectedRoad &road) {
            if (road.turn.instruction.type == TurnType::OnRamp)
                road.turn.instruction.type = TurnType::OffRamp;
        });
    }

    return intersection;
}

std::vector<TurnOperation>
TurnAnalysis::transformIntersectionIntoTurns(const Intersection &intersection) const
{
    std::vector<TurnOperation> turns;
    for (auto road : intersection)
        if (road.entry_allowed)
            turns.emplace_back(road.turn);

    return turns;
}

Intersection TurnAnalysis::getIntersection(const NodeID from_nid, const EdgeID via_eid) const
{
    return intersection_generator(from_nid, via_eid);
}

// Sets basic turn types as fallback for otherwise unhandled turns
Intersection
TurnAnalysis::setTurnTypes(const NodeID from_nid, const EdgeID, Intersection intersection) const
{
    for (auto &road : intersection)
    {
        if (!road.entry_allowed)
            continue;

        const EdgeID onto_edge = road.turn.eid;
        const NodeID to_nid = node_based_graph.GetTarget(onto_edge);

        road.turn.instruction = {TurnType::Turn,
                                 (from_nid == to_nid) ? DirectionModifier::UTurn
                                                      : getTurnDirection(road.turn.angle)};
    }
    return intersection;
}

// "Sliproads" occur when we've got a link between two roads (MOTORWAY_LINK, etc), but
// the two roads are *also* directly connected shortly afterwards.
// In these cases, we tag the turn-type as "sliproad", and then in post-processing
// we emit a "turn", instead of "take the ramp"+"merge"
Intersection TurnAnalysis::handleSliproads(const EdgeID source_edge_id,
                                           Intersection intersection) const
{
    auto intersection_node_id = node_based_graph.GetTarget(source_edge_id);

    const auto linkTest = [this](const ConnectedRoad &road) {
        return !node_based_graph.GetEdgeData(road.turn.eid).roundabout && road.entry_allowed &&
               angularDeviation(road.turn.angle, STRAIGHT_ANGLE) <= 2 * NARROW_TURN_ANGLE &&
               !hasRoundaboutType(road.turn.instruction);
    };

    bool hasNarrow =
        std::find_if(intersection.begin(), intersection.end(), linkTest) != intersection.end();
    if (!hasNarrow)
        return intersection;

    const auto source_edge_data = node_based_graph.GetEdgeData(source_edge_id);

    // Find the continuation of the intersection we're on
    auto next_road = std::find_if(
        intersection.begin(),
        intersection.end(),
        [this, source_edge_data](const ConnectedRoad &road) {
            const auto road_edge_data = node_based_graph.GetEdgeData(road.turn.eid);
            // Test to see if the source edge and the one we're looking at are the same road
            return road_edge_data.road_classification.road_class ==
                       source_edge_data.road_classification.road_class &&
                   road_edge_data.name_id != EMPTY_NAMEID &&
                   road_edge_data.name_id == source_edge_data.name_id && road.entry_allowed &&
                   angularDeviation(road.turn.angle, STRAIGHT_ANGLE) < FUZZY_ANGLE_DIFFERENCE;
        });

    const bool hasNext = next_road != intersection.end();

    if (!hasNext)
    {
        return intersection;
    }

    // Threshold check, if the intersection is too far away, don't bother continuing
    const auto &next_road_data = node_based_graph.GetEdgeData(next_road->turn.eid);
    if (next_road_data.distance > MAX_SLIPROAD_THRESHOLD)
    {
        return intersection;
    }

    const auto next_road_next_intersection =
        intersection_generator(intersection_node_id, next_road->turn.eid);

    const auto next_intersection_node = node_based_graph.GetTarget(next_road->turn.eid);

    std::unordered_set<NameID> target_road_names;

    for (const auto &road : next_road_next_intersection)
    {
        const auto &target_data = node_based_graph.GetEdgeData(road.turn.eid);
        target_road_names.insert(target_data.name_id);
    }

    for (auto &road : intersection)
    {
        if (linkTest(road))
        {
            auto target_intersection = intersection_generator(intersection_node_id, road.turn.eid);
            for (const auto &candidate_road : target_intersection)
            {
                const auto &candidate_data = node_based_graph.GetEdgeData(candidate_road.turn.eid);
                if (target_road_names.count(candidate_data.name_id) > 0 &&
                    node_based_graph.GetTarget(candidate_road.turn.eid) == next_intersection_node)
                {
                    road.turn.instruction.type = TurnType::Sliproad;
                    break;
                }
            }
        }
    }

    if (next_road->turn.instruction.type == TurnType::Fork)
    {
        const auto &next_data = node_based_graph.GetEdgeData(next_road->turn.eid);
        if (next_data.name_id == source_edge_data.name_id)
        {
            if (angularDeviation(next_road->turn.angle, STRAIGHT_ANGLE) < 5)
                next_road->turn.instruction.type = TurnType::Suppressed;
            else
                next_road->turn.instruction.type = TurnType::Continue;
            next_road->turn.instruction.direction_modifier =
                getTurnDirection(next_road->turn.angle);
        }
        else if (next_data.name_id != EMPTY_NAMEID)
        {
            next_road->turn.instruction.type = TurnType::NewName;
            next_road->turn.instruction.direction_modifier =
                getTurnDirection(next_road->turn.angle);
        }
        else
        {
            next_road->turn.instruction.type = TurnType::Suppressed;
        }
    }

    return intersection;
}

const IntersectionGenerator &TurnAnalysis::getGenerator() const { return intersection_generator; }

} // namespace guidance
} // namespace extractor
} // namespace osrm
