#include "engine/routing_algorithms/tile_turns.hpp"

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

std::vector<TurnData>
getTileTurns(const datafacade::ContiguousInternalMemoryDataFacade<ch::Algorithm> &facade,
             const std::vector<RTreeLeaf> &edges,
             const std::vector<std::size_t> &sorted_edge_indexes)
{
    std::vector<TurnData> all_turn_data;

    // Struct to hold info on all the EdgeBasedNodes that are visible in our tile
    // When we create these, we insure that (source, target) and packed_geometry_id
    // are all pointed in the same direction.
    struct EdgeBasedNodeInfo
    {
        bool is_geometry_forward; // Is the geometry forward or reverse?
        unsigned packed_geometry_id;
    };
    // Lookup table for edge-based-nodes
    std::unordered_map<NodeID, EdgeBasedNodeInfo> edge_based_node_info;

    struct SegmentData
    {
        NodeID target_node;
        EdgeID edge_based_node_id;
    };

    std::unordered_map<NodeID, std::vector<SegmentData>> directed_graph;
    // Reserve enough space for unique edge-based-nodes on every edge.
    // Only a tile with all unique edges will use this much, but
    // it saves us a bunch of re-allocations during iteration.
    directed_graph.reserve(edges.size() * 2);

    // Build an adjacency list for all the road segments visible in
    // the tile
    for (const auto &edge_index : sorted_edge_indexes)
    {
        const auto &edge = edges[edge_index];
        if (edge.forward_segment_id.enabled)
        {
            // operator[] will construct an empty vector at [edge.u] if there is no value.
            directed_graph[edge.u].push_back({edge.v, edge.forward_segment_id.id});
            if (edge_based_node_info.count(edge.forward_segment_id.id) == 0)
            {
                edge_based_node_info[edge.forward_segment_id.id] = {true, edge.packed_geometry_id};
            }
            else
            {
                BOOST_ASSERT(edge_based_node_info[edge.forward_segment_id.id].is_geometry_forward ==
                             true);
                BOOST_ASSERT(edge_based_node_info[edge.forward_segment_id.id].packed_geometry_id ==
                             edge.packed_geometry_id);
            }
        }
        if (edge.reverse_segment_id.enabled)
        {
            directed_graph[edge.v].push_back({edge.u, edge.reverse_segment_id.id});
            if (edge_based_node_info.count(edge.reverse_segment_id.id) == 0)
            {
                edge_based_node_info[edge.reverse_segment_id.id] = {false, edge.packed_geometry_id};
            }
            else
            {
                BOOST_ASSERT(edge_based_node_info[edge.reverse_segment_id.id].is_geometry_forward ==
                             false);
                BOOST_ASSERT(edge_based_node_info[edge.reverse_segment_id.id].packed_geometry_id ==
                             edge.packed_geometry_id);
            }
        }
    }

    // Given a turn:
    //     u---v
    //         |
    //         w
    //  uv is the "approach"
    //  vw is the "exit"
    std::vector<contractor::QueryEdge::EdgeData> unpacked_shortcut;
    std::vector<EdgeWeight> approach_weight_vector;
    std::vector<EdgeWeight> approach_duration_vector;

    // Make sure we traverse the startnodes in a consistent order
    // to ensure identical PBF encoding on all platforms.
    std::vector<NodeID> sorted_startnodes;
    sorted_startnodes.reserve(directed_graph.size());
    for (const auto &startnode : directed_graph)
        sorted_startnodes.push_back(startnode.first);
    std::sort(sorted_startnodes.begin(), sorted_startnodes.end());

    // Look at every node in the directed graph we created
    for (const auto &startnode : sorted_startnodes)
    {
        const auto &nodedata = directed_graph[startnode];
        // For all the outgoing edges from the node
        for (const auto &approachedge : nodedata)
        {
            // If the target of this edge doesn't exist in our directed
            // graph, it's probably outside the tile, so we can skip it
            if (directed_graph.count(approachedge.target_node) == 0)
                continue;

            // For each of the outgoing edges from our target coordinate
            for (const auto &exit_edge : directed_graph[approachedge.target_node])
            {
                // If the next edge has the same edge_based_node_id, then it's
                // not a turn, so skip it
                if (approachedge.edge_based_node_id == exit_edge.edge_based_node_id)
                    continue;

                // Skip u-turns
                if (startnode == exit_edge.target_node)
                    continue;

                // Find the connection between our source road and the target node
                // Since we only want to find direct edges, we cannot check shortcut edges here.
                // Otherwise we might find a forward edge even though a shorter backward edge
                // exists (due to oneways).
                //
                // a > - > - > - b
                // |             |
                // |------ c ----|
                //
                // would offer a backward edge at `b` to `a` (due to the oneway from a to b)
                // but could also offer a shortcut (b-c-a) from `b` to `a` which is longer.
                EdgeID smaller_edge_id =
                    facade.FindSmallestEdge(approachedge.edge_based_node_id,
                                            exit_edge.edge_based_node_id,
                                            [](const contractor::QueryEdge::EdgeData &data) {
                                                return data.forward && !data.shortcut;
                                            });

                // Depending on how the graph is constructed, we might have to look for
                // a backwards edge instead.  They're equivalent, just one is available for
                // a forward routing search, and one is used for the backwards dijkstra
                // steps.  Their weight should be the same, we can use either one.
                // If we didn't find a forward edge, try for a backward one
                if (SPECIAL_EDGEID == smaller_edge_id)
                {
                    smaller_edge_id =
                        facade.FindSmallestEdge(exit_edge.edge_based_node_id,
                                                approachedge.edge_based_node_id,
                                                [](const contractor::QueryEdge::EdgeData &data) {
                                                    return data.backward && !data.shortcut;
                                                });
                }

                // If no edge was found, it means that there's no connection between these
                // nodes, due to oneways or turn restrictions. Given the edge-based-nodes
                // that we're examining here, we *should* only find directly-connected
                // edges, not shortcuts
                if (smaller_edge_id != SPECIAL_EDGEID)
                {
                    const auto &data = facade.GetEdgeData(smaller_edge_id);
                    BOOST_ASSERT_MSG(!data.shortcut, "Connecting edge must not be a shortcut");

                    // Now, calculate the sum of the weight of all the segments.
                    if (edge_based_node_info[approachedge.edge_based_node_id].is_geometry_forward)
                    {
                        approach_weight_vector = facade.GetUncompressedForwardWeights(
                            edge_based_node_info[approachedge.edge_based_node_id]
                                .packed_geometry_id);
                        approach_duration_vector = facade.GetUncompressedForwardDurations(
                            edge_based_node_info[approachedge.edge_based_node_id]
                                .packed_geometry_id);
                    }
                    else
                    {
                        approach_weight_vector = facade.GetUncompressedReverseWeights(
                            edge_based_node_info[approachedge.edge_based_node_id]
                                .packed_geometry_id);
                        approach_duration_vector = facade.GetUncompressedReverseDurations(
                            edge_based_node_info[approachedge.edge_based_node_id]
                                .packed_geometry_id);
                    }
                    const auto sum_node_weight = std::accumulate(approach_weight_vector.begin(),
                                                                 approach_weight_vector.end(),
                                                                 EdgeWeight{0});
                    const auto sum_node_duration = std::accumulate(approach_duration_vector.begin(),
                                                                   approach_duration_vector.end(),
                                                                   EdgeWeight{0});

                    // The edge.weight is the whole edge weight, which includes the turn
                    // cost.
                    // The turn cost is the edge.weight minus the sum of the individual road
                    // segment weights.  This might not be 100% accurate, because some
                    // intersections include stop signs, traffic signals and other
                    // penalties, but at this stage, we can't divide those out, so we just
                    // treat the whole lot as the "turn cost" that we'll stick on the map.
                    const auto turn_weight = data.weight - sum_node_weight;
                    const auto turn_duration = data.duration - sum_node_duration;

                    // Find the three nodes that make up the turn movement)
                    const auto node_from = startnode;
                    const auto node_via = approachedge.target_node;
                    const auto node_to = exit_edge.target_node;

                    const auto coord_from = facade.GetCoordinateOfNode(node_from);
                    const auto coord_via = facade.GetCoordinateOfNode(node_via);
                    const auto coord_to = facade.GetCoordinateOfNode(node_to);

                    // Calculate the bearing that we approach the intersection at
                    const auto angle_in = static_cast<int>(
                        util::coordinate_calculation::bearing(coord_from, coord_via));

                    const auto exit_bearing = static_cast<int>(
                        util::coordinate_calculation::bearing(coord_via, coord_to));

                    // Figure out the angle of the turn
                    auto turn_angle = exit_bearing - angle_in;
                    while (turn_angle > 180)
                    {
                        turn_angle -= 360;
                    }
                    while (turn_angle < -180)
                    {
                        turn_angle += 360;
                    }

                    // Save everything we need to later add all the points to the tile.
                    // We need the coordinate of the intersection, the angle in, the turn
                    // angle and the turn cost.
                    all_turn_data.push_back(
                        TurnData{coord_via, angle_in, turn_angle, turn_weight, turn_duration});
                }
            }
        }
    }

    return all_turn_data;
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
