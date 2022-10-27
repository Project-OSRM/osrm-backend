#include "engine/routing_algorithms/tile_turns.hpp"

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

namespace
{
// Struct to hold info on all the EdgeBasedNodes that are visible in our tile
// When we create these, we insure that (source, target) and packed_geometry_id
// are all pointed in the same direction.
struct EdgeBasedNodeInfo
{
    bool is_geometry_forward; // Is the geometry forward or reverse?
    unsigned packed_geometry_id;
};

struct SegmentData
{
    NodeID target_node;
    EdgeID edge_based_node_id;
};

template <typename edge_extractor, typename datafacade>
std::vector<TurnData> generateTurns(const datafacade &facade,
                                    const std::vector<RTreeLeaf> &edges,
                                    const std::vector<std::size_t> &sorted_edge_indexes,
                                    edge_extractor const &find_edge)
{
    // Lookup table for edge-based-nodes
    std::unordered_map<NodeID, EdgeBasedNodeInfo> edge_based_node_info;
    std::unordered_map<NodeID, std::vector<SegmentData>> directed_graph;

    // Reserve enough space for unique edge-based-nodes on every edge.
    // Only a tile with all unique edges will use this much, but
    // it saves us a bunch of re-allocations during iteration.
    directed_graph.reserve(edges.size() * 2);

    const auto get_geometry_id = [&facade](auto edge) {
        return facade.GetGeometryIndex(edge.forward_segment_id.id).id;
    };

    // To build a tile, we can only rely on the r-tree to quickly find all data visible within the
    // tile itself. The Rtree returns a series of segments that may or may not offer turns
    // associated with them. To be able to extract turn penalties, we extract a node based graph
    // from our edge based representation.
    for (const auto &edge_index : sorted_edge_indexes)
    {
        const auto &edge = edges[edge_index];
        if (edge.forward_segment_id.enabled)
        {
            // operator[] will construct an empty vector at [edge.u] if there is no value.
            directed_graph[edge.u].push_back({edge.v, edge.forward_segment_id.id});
            if (edge_based_node_info.count(edge.forward_segment_id.id) == 0)
            {
                edge_based_node_info[edge.forward_segment_id.id] = {true, get_geometry_id(edge)};
            }
            else
            {
                BOOST_ASSERT(edge_based_node_info[edge.forward_segment_id.id].is_geometry_forward ==
                             true);
                BOOST_ASSERT(edge_based_node_info[edge.forward_segment_id.id].packed_geometry_id ==
                             get_geometry_id(edge));
            }
        }
        if (edge.reverse_segment_id.enabled)
        {
            directed_graph[edge.v].push_back({edge.u, edge.reverse_segment_id.id});
            if (edge_based_node_info.count(edge.reverse_segment_id.id) == 0)
            {
                edge_based_node_info[edge.reverse_segment_id.id] = {false, get_geometry_id(edge)};
            }
            else
            {
                BOOST_ASSERT(edge_based_node_info[edge.reverse_segment_id.id].is_geometry_forward ==
                             false);
                BOOST_ASSERT(edge_based_node_info[edge.reverse_segment_id.id].packed_geometry_id ==
                             get_geometry_id(edge));
            }
        }
    }

    // Make sure we traverse the startnodes in a consistent order
    // to ensure identical PBF encoding on all platforms.
    std::vector<NodeID> sorted_startnodes;
    sorted_startnodes.reserve(directed_graph.size());
    std::transform(directed_graph.begin(),
                   directed_graph.end(),
                   std::back_inserter(sorted_startnodes),
                   [](auto const &node) { return node.first; });
    std::sort(sorted_startnodes.begin(), sorted_startnodes.end());

    std::vector<TurnData> all_turn_data;

    // Given a turn:
    //     u---v
    //         |
    //         w
    //  uv is the "approach"
    //  vw is the "exit"
    // Look at every node in the directed graph we created
    for (const auto &startnode : sorted_startnodes)
    {
        BOOST_ASSERT(directed_graph.find(startnode) != directed_graph.end());
        const auto &nodedata = directed_graph.find(startnode)->second;
        // For all the outgoing edges from the node
        for (const auto &approachedge : nodedata)
        {
            // If the target of this edge doesn't exist in our directed
            // graph, it's probably outside the tile, so we can skip it
            if (directed_graph.count(approachedge.target_node) == 0)
                continue;

            // For each of the outgoing edges from our target coordinate
            for (const auto &exit_edge : directed_graph.find(approachedge.target_node)->second)
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
                EdgeID edge_based_edge_id =
                    find_edge(approachedge.edge_based_node_id, exit_edge.edge_based_node_id);

                if (edge_based_edge_id != SPECIAL_EDGEID)
                {
                    const auto &data = facade.GetEdgeData(edge_based_edge_id);

                    // Now, calculate the sum of the weight of all the segments.
                    const auto turn_weight = facade.GetWeightPenaltyForEdgeID(data.turn_id);
                    const auto turn_duration = facade.GetDurationPenaltyForEdgeID(data.turn_id);
                    const auto turn_instruction = facade.GetTurnInstructionForEdgeID(data.turn_id);

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
                    all_turn_data.push_back(TurnData{coord_via,
                                                     angle_in,
                                                     turn_angle,
                                                     turn_weight,
                                                     turn_duration,
                                                     turn_instruction});
                }
            }
        }
    }

    return all_turn_data;
}

} // namespace

// CH Version of finding all turn penalties. Here is where the actual work is happening
std::vector<TurnData> getTileTurns(const DataFacade<ch::Algorithm> &facade,
                                   const std::vector<RTreeLeaf> &edges,
                                   const std::vector<std::size_t> &sorted_edge_indexes)
{
    // Define how to find the representative edge between two edge based nodes for a CH
    struct EdgeFinderCH
    {
        EdgeFinderCH(const DataFacade<ch::Algorithm> &facade) : facade(facade) {}
        const DataFacade<ch::Algorithm> &facade;

        EdgeID operator()(const NodeID approach_node, const NodeID exit_node) const
        {
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
            EdgeID edge_id = facade.FindSmallestEdge(
                approach_node, exit_node, [](const contractor::QueryEdge::EdgeData &data) {
                    return data.forward && !data.shortcut;
                });

            // Depending on how the graph is constructed, we might have to look for
            // a backwards edge instead.  They're equivalent, just one is available for
            // a forward routing search, and one is used for the backwards dijkstra
            // steps.  Their weight should be the same, we can use either one.
            // If we didn't find a forward edge, try for a backward one
            if (SPECIAL_EDGEID == edge_id)
            {
                edge_id = facade.FindSmallestEdge(
                    exit_node, approach_node, [](const contractor::QueryEdge::EdgeData &data) {
                        return data.backward && !data.shortcut;
                    });
            }

            BOOST_ASSERT_MSG(edge_id == SPECIAL_EDGEID || !facade.GetEdgeData(edge_id).shortcut,
                             "Connecting edge must not be a shortcut");
            return edge_id;
        }
    };

    EdgeFinderCH edge_finder(facade);
    return generateTurns(facade, edges, sorted_edge_indexes, edge_finder);
}

// MLD version to find all turns
std::vector<TurnData> getTileTurns(const DataFacade<mld::Algorithm> &facade,
                                   const std::vector<RTreeLeaf> &edges,
                                   const std::vector<std::size_t> &sorted_edge_indexes)
{
    // Define how to find the representative edge between two edge-based-nodes for a MLD
    struct EdgeFinderMLD
    {
        EdgeFinderMLD(const DataFacade<mld::Algorithm> &facade) : facade(facade) {}
        const DataFacade<mld::Algorithm> &facade;

        EdgeID operator()(const NodeID approach_node, const NodeID exit_node) const
        {
            return facade.FindEdge(approach_node, exit_node);
        }
    };

    EdgeFinderMLD edge_finder(facade);

    return generateTurns(facade, edges, sorted_edge_indexes, edge_finder);
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
