#include "extractor/guidance/turn_analysis.hpp"
#include "extractor/guidance/constants.hpp"
#include "extractor/guidance/road_classification.hpp"

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
      turn_handler(node_based_graph, node_info_list, name_table, street_name_suffix_table),
      sliproad_handler(intersection_generator,
                       node_based_graph,
                       node_info_list,
                       name_table,
                       street_name_suffix_table)
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
    intersection = sliproad_handler(from_nid, via_eid, std::move(intersection));

    // Turn On Ramps Into Off Ramps, if we come from a motorway-like road
    if (node_based_graph.GetEdgeData(via_eid).road_classification.IsMotorwayClass())
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

const IntersectionGenerator &TurnAnalysis::getGenerator() const { return intersection_generator; }

} // namespace guidance
} // namespace extractor
} // namespace osrm
