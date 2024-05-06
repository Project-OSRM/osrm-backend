#include "guidance/turn_discovery.hpp"
#include "extractor/intersection/coordinate_extractor.hpp"
#include "extractor/intersection/intersection_analysis.hpp"
#include "guidance/constants.hpp"
#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"

using osrm::util::angularDeviation;

namespace osrm::guidance::lanes
{

bool findPreviousIntersection(const NodeID node_v,
                              const EdgeID via_edge,
                              const Intersection &intersection,
                              const util::NodeBasedDynamicGraph &node_based_graph,
                              const extractor::EdgeBasedNodeDataContainer &node_data_container,
                              const std::vector<util::Coordinate> &node_coordinates,
                              const extractor::CompressedEdgeContainer &compressed_geometries,
                              const extractor::RestrictionMap &node_restriction_map,
                              const std::unordered_set<NodeID> &barrier_nodes,
                              const extractor::TurnLanesIndexedArray &turn_lanes_data,
                              // output parameters
                              NodeID &result_node,
                              EdgeID &result_via_edge,
                              extractor::intersection::IntersectionView &result_intersection)
{
    /* We need to find the intersection that is located prior to via_edge.

     *
     * NODE_U  -> PREVIOUS_ID            -> NODE_V -> VIA_EDGE -> NODE_W:INTERSECTION
     * NODE_U? <- STRAIGHTMOST           <- NODE_V <- UTURN
     * NODE_U? -> UTURN == PREVIOUSE_ID? -> NODE_V -> VIA_EDGE
     *
     * To do so, we first get the intersection atNODE and find the straightmost turn from that
     * node. This will result in NODE_X. The uturn in the intersection at NODE_X should be
     * PREVIOUS_ID. To verify that find, we check the intersection using our PREVIOUS_ID candidate
     * to check the intersection at NODE for via_edge
     */
    const constexpr double COMBINE_DISTANCE_CUTOFF = 30;

    const extractor::intersection::CoordinateExtractor coordinate_extractor(
        node_based_graph, compressed_geometries, node_coordinates);

    const auto coordinates_along_via_edge =
        coordinate_extractor.GetForwardCoordinatesAlongRoad(node_v, via_edge);
    const auto via_edge_length =
        util::coordinate_calculation::getLength(coordinates_along_via_edge.begin(),
                                                coordinates_along_via_edge.end(),
                                                &util::coordinate_calculation::greatCircleDistance);

    // we check if via-edge is too short. In this case the previous turn cannot influence the turn
    // at via_edge and the intersection at NODE_W
    if (via_edge_length > COMBINE_DISTANCE_CUTOFF)
        return false;

    // Node -> Via_Edge -> Intersection[0 == UTURN] -> reverse_of(via_edge) -> Intersection at
    // node
    // (looking at the reverse direction).
    const auto node_w = node_based_graph.GetTarget(via_edge);
    const auto u_turn_at_node_w = intersection[0].eid;

    // make sure the ID is actually valid
    BOOST_ASSERT(node_based_graph.BeginEdges(node_w) <= u_turn_at_node_w &&
                 u_turn_at_node_w <= node_based_graph.EndEdges(node_w));

    // if we can't find the correct road, stop
    if (node_based_graph.GetTarget(u_turn_at_node_w) != node_v)
        return false;

    const auto node_v_reverse_intersection =
        extractor::intersection::getConnectedRoads<true>(node_based_graph,
                                                         node_data_container,
                                                         node_coordinates,
                                                         compressed_geometries,
                                                         node_restriction_map,
                                                         barrier_nodes,
                                                         turn_lanes_data,
                                                         {node_w, u_turn_at_node_w});
    // Continue along the straightmost turn. If there is no straight turn, we cannot find a valid
    // previous intersection.
    const auto straightmost_at_v_in_reverse =
        node_v_reverse_intersection.findClosestTurn(STRAIGHT_ANGLE);

    // TODO evaluate if narrow turn is the right criterion here... Might be that other angles are
    // valid
    if (angularDeviation(straightmost_at_v_in_reverse->angle, STRAIGHT_ANGLE) > GROUP_ANGLE)
        return false;

    const auto node_u = node_based_graph.GetTarget(straightmost_at_v_in_reverse->eid);
    const auto node_u_reverse_intersection = extractor::intersection::getConnectedRoads<true>(
        node_based_graph,
        node_data_container,
        node_coordinates,
        compressed_geometries,
        node_restriction_map,
        barrier_nodes,
        turn_lanes_data,
        {node_v, straightmost_at_v_in_reverse->eid});

    // now check that the u-turn at the given intersection connects to via-edge
    // The u-turn at the now found intersection should, hopefully, represent the previous edge.
    result_node = node_u;
    result_via_edge = node_u_reverse_intersection[0].eid;
    if (node_based_graph.GetTarget(result_via_edge) != node_v)
        return false;

    // if the edge is not traversable, we obviously don't have a previous intersection or couldn't
    // find it.
    if (node_based_graph.GetEdgeData(result_via_edge).reversed)
    {
        result_via_edge = SPECIAL_EDGEID;
        result_node = SPECIAL_NODEID;
        return false;
    }

    result_intersection =
        extractor::intersection::getConnectedRoads<false>(node_based_graph,
                                                          node_data_container,
                                                          node_coordinates,
                                                          compressed_geometries,
                                                          node_restriction_map,
                                                          barrier_nodes,
                                                          turn_lanes_data,
                                                          {node_u, result_via_edge});
    const auto check_via_edge =
        result_intersection.end() !=
        std::find_if(result_intersection.begin(),
                     result_intersection.end(),
                     [via_edge](const extractor::intersection::IntersectionViewData &road)
                     { return road.eid == via_edge; });

    if (!check_via_edge)
    {
        result_via_edge = SPECIAL_EDGEID;
        result_node = SPECIAL_NODEID;
        return false;
    }

    return true;
}

} // namespace osrm::guidance::lanes
