#ifndef EDGE_UNPACKER_H
#define EDGE_UNPACKER_H

#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/travel_mode.hpp"
#include "engine/phantom_node.hpp"
#include "osrm/coordinate.hpp"
#include "util/guidance/turn_lanes.hpp"
#include "util/typedefs.hpp"

#include <stack>
#include <vector>

namespace osrm
{
namespace engine
{

/**
 * Given a sequence of connected `NodeID`s in the CH graph, performs a depth-first unpacking of
 * the shortcut
 * edges.  For every "original" edge found, it calls the `callback` with the two NodeIDs for the
 * edge, and the EdgeData
 * for that edge.
 *
 * The primary purpose of this unpacking is to expand a path through the CH into the original
 * route through the
 * pre-contracted graph.
 *
 * Because of the depth-first-search, the `callback` will effectively be called in sequence for
 * the original route
 * from beginning to end.
 *
 * @param packed_path_begin iterator pointing to the start of the NodeID list
 * @param packed_path_end iterator pointing to the end of the NodeID list
 * @param callback void(const std::pair<NodeID, NodeID>, const EdgeData &) called for each
 * original edge found.
 */

template <typename DataFacadeT, typename BidirectionalIterator, typename Callback>
inline void UnpackCHPath(const DataFacadeT &facade,
                         BidirectionalIterator packed_path_begin,
                         BidirectionalIterator packed_path_end,
                         Callback &&callback)
{
    // make sure we have at least something to unpack
    if (packed_path_begin == packed_path_end)
        return;

    using EdgeData = typename DataFacadeT::EdgeData;

    std::stack<std::pair<NodeID, NodeID>> recursion_stack;

    // We have to push the path in reverse order onto the stack because it's LIFO.
    for (auto current = std::prev(packed_path_end); current != packed_path_begin;
         current = std::prev(current))
    {
        recursion_stack.emplace(*std::prev(current), *current);
    }

    std::pair<NodeID, NodeID> edge;
    while (!recursion_stack.empty())
    {
        edge = recursion_stack.top();
        recursion_stack.pop();

        // Look for an edge on the forward CH graph (.forward)
        EdgeID smaller_edge_id = facade.FindSmallestEdge(
            edge.first, edge.second, [](const EdgeData &data) { return data.forward; });

        // If we didn't find one there, the we might be looking at a part of the path that
        // was found using the backward search.  Here, we flip the node order (.second, .first)
        // and only consider edges with the `.backward` flag.
        if (SPECIAL_EDGEID == smaller_edge_id)
        {
            smaller_edge_id = facade.FindSmallestEdge(
                edge.second, edge.first, [](const EdgeData &data) { return data.backward; });
        }

        // If we didn't find anything *still*, then something is broken and someone has
        // called this function with bad values.
        BOOST_ASSERT_MSG(smaller_edge_id != SPECIAL_EDGEID, "Invalid smaller edge ID");

        const auto &data = facade.GetEdgeData(smaller_edge_id);
        BOOST_ASSERT_MSG(data.weight != std::numeric_limits<EdgeWeight>::max(),
                         "edge weight invalid");

        // If the edge is a shortcut, we need to add the two halfs to the stack.
        if (data.shortcut)
        { // unpack
            const NodeID middle_node_id = data.id;
            // Note the order here - we're adding these to a stack, so we
            // want the first->middle to get visited before middle->second
            recursion_stack.emplace(middle_node_id, edge.second);
            recursion_stack.emplace(edge.first, middle_node_id);
        }
        else
        {
            // We found an original edge, call our callback.
            std::forward<Callback>(callback)(edge, data);
        }
    }
}
}
}

#endif // EDGE_UNPACKER_H
