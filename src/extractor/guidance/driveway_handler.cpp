#include "extractor/guidance/driveway_handler.hpp"

#include "util/assert.hpp"

using osrm::extractor::guidance::getTurnDirection;
using osrm::util::angularDeviation;

namespace osrm
{
namespace extractor
{
namespace guidance
{

DrivewayHandler::DrivewayHandler(const IntersectionGenerator &intersection_generator,
                                 const util::NodeBasedDynamicGraph &node_based_graph,
                                 const EdgeBasedNodeDataContainer &node_data_container,
                                 const std::vector<util::Coordinate> &coordinates,
                                 const util::NameTable &name_table,
                                 const SuffixTable &street_name_suffix_table)
    : IntersectionHandler(node_based_graph,
                          node_data_container,
                          coordinates,
                          name_table,
                          street_name_suffix_table,
                          intersection_generator)
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
        std::count_if(intersection.begin(), intersection.end(), [this](const auto &road) {
            return node_based_graph.GetEdgeData(road.eid)
                .flags.road_classification.IsLowPriorityRoadClass();
        });

    // Process intersection if it has two edges with normal priority and one is the entry edge,
    // and also has at least one edge with lower priority
    return static_cast<std::size_t>(low_priority_count) + 2 == intersection.size();
}

Intersection DrivewayHandler::
operator()(const NodeID nid, const EdgeID source_edge_id, Intersection intersection) const
{
    auto road =
        std::find_if(intersection.begin() + 1, intersection.end(), [this](const auto &road) {
            return !node_based_graph.GetEdgeData(road.eid)
                        .flags.road_classification.IsLowPriorityRoadClass();
        });

    (void)nid;
    OSRM_ASSERT(road != intersection.end(), coordinates[nid]);

    if (road->instruction == TurnInstruction::INVALID())
        return intersection;

    OSRM_ASSERT(road->instruction.type == TurnType::Turn, coordinates[nid]);

    road->instruction.type =
        isSameName(source_edge_id, road->eid) ? TurnType::NoTurn : TurnType::NewName;

    return intersection;
}

} // namespace guidance
} // namespace extractor
} // namespace osrm
