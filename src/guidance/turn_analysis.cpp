#include "guidance/turn_analysis.hpp"
#include "extractor/road_classification.hpp"
#include "guidance/constants.hpp"

#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"

#include <cstddef>
#include <set>
#include <unordered_set>
#include <utility>

using osrm::guidance::getTurnDirection;

namespace osrm
{
namespace guidance
{

using EdgeData = util::NodeBasedDynamicGraph::EdgeData;

TurnAnalysis::TurnAnalysis(const util::NodeBasedDynamicGraph &node_based_graph,
                           const extractor::EdgeBasedNodeDataContainer &node_data_container,
                           const std::vector<util::Coordinate> &node_coordinates,
                           const extractor::CompressedEdgeContainer &compressed_edge_container,
                           const extractor::RestrictionMap &restriction_map,
                           const std::unordered_set<NodeID> &barrier_nodes,
                           const extractor::TurnLanesIndexedArray &turn_lanes_data,
                           const extractor::NameTable &name_table,
                           const extractor::SuffixTable &street_name_suffix_table)
    : node_based_graph(node_based_graph), roundabout_handler(node_based_graph,
                                                             node_data_container,
                                                             node_coordinates,
                                                             compressed_edge_container,
                                                             restriction_map,
                                                             barrier_nodes,
                                                             turn_lanes_data,
                                                             name_table,
                                                             street_name_suffix_table),
      motorway_handler(node_based_graph,
                       node_data_container,
                       node_coordinates,
                       compressed_edge_container,
                       restriction_map,
                       barrier_nodes,
                       turn_lanes_data,
                       name_table,
                       street_name_suffix_table),
      turn_handler(node_based_graph,
                   node_data_container,
                   node_coordinates,
                   compressed_edge_container,
                   restriction_map,
                   barrier_nodes,
                   turn_lanes_data,
                   name_table,
                   street_name_suffix_table),
      sliproad_handler(node_based_graph,
                       node_data_container,
                       node_coordinates,
                       compressed_edge_container,
                       restriction_map,
                       barrier_nodes,
                       turn_lanes_data,
                       name_table,
                       street_name_suffix_table),
      suppress_mode_handler(node_based_graph,
                            node_data_container,
                            node_coordinates,
                            compressed_edge_container,
                            restriction_map,
                            barrier_nodes,
                            turn_lanes_data,
                            name_table,
                            street_name_suffix_table),
      driveway_handler(node_based_graph,
                       node_data_container,
                       node_coordinates,
                       compressed_edge_container,
                       restriction_map,
                       barrier_nodes,
                       turn_lanes_data,
                       name_table,
                       street_name_suffix_table),
      statistics_handler(node_based_graph,
                         node_data_container,
                         node_coordinates,
                         compressed_edge_container,
                         restriction_map,
                         barrier_nodes,
                         turn_lanes_data,
                         name_table,
                         street_name_suffix_table)
{
}

Intersection TurnAnalysis::AssignTurnTypes(
    const NodeID node_prior_to_intersection,
    const EdgeID entering_via_edge,
    const extractor::intersection::IntersectionView &intersection_view) const
{
    // Roundabouts are a main priority. If there is a roundabout instruction present, we process the
    // turn as a roundabout

    // the following lines create a partly invalid intersection object. We might want to refactor
    // this at some point
    Intersection intersection;
    intersection.reserve(intersection_view.size());
    std::transform(intersection_view.begin(),
                   intersection_view.end(),
                   std::back_inserter(intersection),
                   [&](const extractor::intersection::IntersectionViewData &data) {
                       return ConnectedRoad(data,
                                            {TurnType::Invalid, DirectionModifier::UTurn},
                                            INVALID_LANE_DATAID);
                   });

    // Suppress turns on ways between mode types that do not need guidance, think ferry routes.
    // This handler has to come first and when it triggers we're done with the intersection: there's
    // nothing left to be done once we suppressed instructions on such routes. Exit early.
    if (suppress_mode_handler.canProcess(
            node_prior_to_intersection, entering_via_edge, intersection))
    {
        intersection = suppress_mode_handler(
            node_prior_to_intersection, entering_via_edge, std::move(intersection));

        return intersection;
    }

    if (roundabout_handler.canProcess(node_prior_to_intersection, entering_via_edge, intersection))
    {
        intersection = roundabout_handler(
            node_prior_to_intersection, entering_via_edge, std::move(intersection));
    }
    else
    {
        // set initial defaults for normal turns and modifier based on angle
        intersection =
            setTurnTypes(node_prior_to_intersection, entering_via_edge, std::move(intersection));
        if (driveway_handler.canProcess(
                node_prior_to_intersection, entering_via_edge, intersection))
        {
            intersection = driveway_handler(
                node_prior_to_intersection, entering_via_edge, std::move(intersection));
        }
        else if (motorway_handler.canProcess(
                     node_prior_to_intersection, entering_via_edge, intersection))
        {
            intersection = motorway_handler(
                node_prior_to_intersection, entering_via_edge, std::move(intersection));
        }
        else
        {
            BOOST_ASSERT(turn_handler.canProcess(
                node_prior_to_intersection, entering_via_edge, intersection));
            intersection = turn_handler(
                node_prior_to_intersection, entering_via_edge, std::move(intersection));
        }
    }
    // Handle sliproads
    if (sliproad_handler.canProcess(node_prior_to_intersection, entering_via_edge, intersection))
        intersection = sliproad_handler(
            node_prior_to_intersection, entering_via_edge, std::move(intersection));

    // Turn On Ramps Into Off Ramps, if we come from a motorway-like road
    if (node_based_graph.GetEdgeData(entering_via_edge).flags.road_classification.IsMotorwayClass())
    {
        std::for_each(intersection.begin(), intersection.end(), [](ConnectedRoad &road) {
            if (road.instruction.type == TurnType::OnRamp)
                road.instruction.type = TurnType::OffRamp;
        });
    }

    // After we ran all handlers and determined instruction type
    // and direction modifier gather statistics about our decisions.
    if (statistics_handler.canProcess(node_prior_to_intersection, entering_via_edge, intersection))
        intersection = statistics_handler(
            node_prior_to_intersection, entering_via_edge, std::move(intersection));

    return intersection;
}

// Sets basic turn types as fallback for otherwise unhandled turns
Intersection TurnAnalysis::setTurnTypes(const NodeID node_prior_to_intersection,
                                        const EdgeID,
                                        Intersection intersection) const
{
    for (auto &road : intersection)
    {
        if (!road.entry_allowed)
            continue;

        const EdgeID onto_edge = road.eid;
        const NodeID to_nid = node_based_graph.GetTarget(onto_edge);

        if (node_prior_to_intersection == to_nid)
            road.instruction = {TurnType::Continue, DirectionModifier::UTurn};
        else
            road.instruction = {TurnType::Turn, getTurnDirection(road.angle)};
    }
    return intersection;
}

} // namespace guidance
} // namespace osrm
