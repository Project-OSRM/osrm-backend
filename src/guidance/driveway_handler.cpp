#include "guidance/driveway_handler.hpp"

#include "util/assert.hpp"

namespace osrm::guidance
{

DrivewayHandler::DrivewayHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                                 const extractor::EdgeBasedNodeDataContainer &node_data_container,
                                 const std::vector<util::Coordinate> &node_coordinates,
                                 const extractor::CompressedEdgeContainer &compressed_geometries,
                                 const extractor::RestrictionMap &node_restriction_map,
                                 const std::unordered_set<NodeID> &barrier_nodes,
                                 const extractor::TurnLanesIndexedArray &turn_lanes_data,
                                 const extractor::NameTable &name_table,
                                 const extractor::SuffixTable &street_name_suffix_table)
    : IntersectionHandler(node_based_graph,
                          node_data_container,
                          node_coordinates,
                          compressed_geometries,
                          node_restriction_map,
                          barrier_nodes,
                          turn_lanes_data,
                          name_table,
                          street_name_suffix_table)
{
}

// The intersection without major roads needs to pass by service roads (bd, be)
//       d
//       .
//  a--->b--->c
//       .
//       e
bool DrivewayHandler::canProcess(const NodeID /*nid*/,
                                 const EdgeID /*via_eid*/,
                                 const Intersection &intersection) const
{
    const auto from_eid = intersection.getUTurnRoad().eid;

    if (intersection.size() <= 2 ||
        node_based_graph.GetEdgeData(from_eid).flags.road_classification.IsLowPriorityRoadClass())
        return false;

    auto low_priority_count =
        std::count_if(intersection.begin(),
                      intersection.end(),
                      [this](const auto &road)
                      {
                          return node_based_graph.GetEdgeData(road.eid)
                              .flags.road_classification.IsLowPriorityRoadClass();
                      });

    // Process intersection if it has two edges with normal priority and one is the entry edge,
    // and also has at least one edge with lower priority
    return static_cast<std::size_t>(low_priority_count) + 2 == intersection.size();
}

Intersection DrivewayHandler::operator()(const NodeID nid,
                                         const EdgeID source_edge_id,
                                         Intersection intersection) const
{
    auto road = std::find_if(intersection.begin() + 1,
                             intersection.end(),
                             [this](const auto &road)
                             {
                                 return !node_based_graph.GetEdgeData(road.eid)
                                             .flags.road_classification.IsLowPriorityRoadClass();
                             });

    (void)nid;
    OSRM_ASSERT(road != intersection.end(), node_coordinates[nid]);

    if (road->instruction == TurnInstruction::INVALID())
        return intersection;

    OSRM_ASSERT(road->instruction.type == TurnType::Turn, node_coordinates[nid]);

    road->instruction.type =
        isSameName(source_edge_id, road->eid) ? TurnType::NoTurn : TurnType::NewName;

    if (road->instruction.direction_modifier == DirectionModifier::Straight)
    {
        std::for_each(
            intersection.begin() + 1,
            road,
            [](auto &side_road)
            {
                if (side_road.instruction.direction_modifier == DirectionModifier::Straight)
                    side_road.instruction.direction_modifier = DirectionModifier::SlightRight;
            });
        std::for_each(
            road + 1,
            intersection.end(),
            [](auto &side_road)
            {
                if (side_road.instruction.direction_modifier == DirectionModifier::Straight)
                    side_road.instruction.direction_modifier = DirectionModifier::SlightLeft;
            });
    }

    return intersection;
}

} // namespace osrm::guidance
