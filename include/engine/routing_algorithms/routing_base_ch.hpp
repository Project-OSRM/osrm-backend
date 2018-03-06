#ifndef OSRM_ENGINE_ROUTING_BASE_CH_HPP
#define OSRM_ENGINE_ROUTING_BASE_CH_HPP

#include "engine/algorithm.hpp"
#include "engine/datafacade.hpp"
#include "engine/routing_algorithms/routing_base.hpp"
#include "engine/search_engine_data.hpp"
#include "engine/unpacking_cache.hpp"

#include "util/typedefs.hpp"

#include <boost/assert.hpp>

namespace osrm
{
namespace engine
{

namespace routing_algorithms
{

namespace ch
{

// Stalling
template <bool DIRECTION, typename HeapT>
bool stallAtNode(const DataFacade<Algorithm> &facade,
                 const NodeID node,
                 const EdgeWeight weight,
                 const HeapT &query_heap)
{
    for (auto edge : facade.GetAdjacentEdgeRange(node))
    {
        const auto &data = facade.GetEdgeData(edge);
        if (DIRECTION == REVERSE_DIRECTION ? data.forward : data.backward)
        {
            const NodeID to = facade.GetTarget(edge);
            const EdgeWeight edge_weight = data.weight;
            BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
            if (query_heap.WasInserted(to))
            {
                if (query_heap.GetKey(to) + edge_weight < weight)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

template <bool DIRECTION>
void relaxOutgoingEdges(const DataFacade<Algorithm> &facade,
                        const NodeID node,
                        const EdgeWeight weight,
                        SearchEngineData<Algorithm>::QueryHeap &heap)
{
    for (const auto edge : facade.GetAdjacentEdgeRange(node))
    {
        const auto &data = facade.GetEdgeData(edge);
        if (DIRECTION == FORWARD_DIRECTION ? data.forward : data.backward)
        {
            const NodeID to = facade.GetTarget(edge);
            const EdgeWeight edge_weight = data.weight;

            BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
            const EdgeWeight to_weight = weight + edge_weight;

            // New Node discovered -> Add to Heap + Node Info Storage
            if (!heap.WasInserted(to))
            {
                heap.Insert(to, to_weight, node);
            }
            // Found a shorter Path -> Update weight
            else if (to_weight < heap.GetKey(to))
            {
                // new parent
                heap.GetData(to).parent = node;
                heap.DecreaseKey(to, to_weight);
            }
        }
    }
}

/*
min_edge_offset is needed in case we use multiple
nodes as start/target nodes with different (even negative) offsets.
In that case the termination criterion is not correct
anymore.

Example:
forward heap: a(-100), b(0),
reverse heap: c(0), d(100)

a --- d
  \ /
  / \
b --- c

This is equivalent to running a bi-directional Dijkstra on the following graph:

    a --- d
   /  \ /  \
  y    x    z
   \  / \  /
    b --- c

The graph is constructed by inserting nodes y and z that are connected to the initial nodes
using edges (y, a) with weight -100, (y, b) with weight 0 and,
(d, z) with weight 100, (c, z) with weight 0 corresponding.
Since we are dealing with a graph that contains _negative_ edges,
we need to add an offset to the termination criterion.
*/
static constexpr bool ENABLE_STALLING = true;
static constexpr bool DISABLE_STALLING = false;
template <bool DIRECTION, bool STALLING = ENABLE_STALLING>
void routingStep(const DataFacade<Algorithm> &facade,
                 SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                 SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                 NodeID &middle_node_id,
                 EdgeWeight &upper_bound,
                 EdgeWeight min_edge_offset,
                 const bool force_loop_forward,
                 const bool force_loop_reverse)
{
    const NodeID node = forward_heap.DeleteMin();
    const EdgeWeight weight = forward_heap.GetKey(node);

    if (reverse_heap.WasInserted(node))
    {
        const EdgeWeight new_weight = reverse_heap.GetKey(node) + weight;
        if (new_weight < upper_bound)
        {
            // if loops are forced, they are so at the source
            if ((force_loop_forward && forward_heap.GetData(node).parent == node) ||
                (force_loop_reverse && reverse_heap.GetData(node).parent == node) ||
                // in this case we are looking at a bi-directional way where the source
                // and target phantom are on the same edge based node
                new_weight < 0)
            {
                // check whether there is a loop present at the node
                for (const auto edge : facade.GetAdjacentEdgeRange(node))
                {
                    const auto &data = facade.GetEdgeData(edge);
                    if (DIRECTION == FORWARD_DIRECTION ? data.forward : data.backward)
                    {
                        const NodeID to = facade.GetTarget(edge);
                        if (to == node)
                        {
                            const EdgeWeight edge_weight = data.weight;
                            const EdgeWeight loop_weight = new_weight + edge_weight;
                            if (loop_weight >= 0 && loop_weight < upper_bound)
                            {
                                middle_node_id = node;
                                upper_bound = loop_weight;
                            }
                        }
                    }
                }
            }
            else
            {
                BOOST_ASSERT(new_weight >= 0);

                middle_node_id = node;
                upper_bound = new_weight;
            }
        }
    }

    // make sure we don't terminate too early if we initialize the weight
    // for the nodes in the forward heap with the forward/reverse offset
    BOOST_ASSERT(min_edge_offset <= 0);
    if (weight + min_edge_offset > upper_bound)
    {
        forward_heap.DeleteAll();
        return;
    }

    // Stalling
    if (STALLING && stallAtNode<DIRECTION>(facade, node, weight, forward_heap))
    {
        return;
    }

    relaxOutgoingEdges<DIRECTION>(facade, node, weight, forward_heap);
}

template <bool UseDuration>
EdgeWeight getLoopWeight(const DataFacade<Algorithm> &facade, NodeID node)
{
    EdgeWeight loop_weight = UseDuration ? MAXIMAL_EDGE_DURATION : INVALID_EDGE_WEIGHT;
    for (auto edge : facade.GetAdjacentEdgeRange(node))
    {
        const auto &data = facade.GetEdgeData(edge);
        if (data.forward)
        {
            const NodeID to = facade.GetTarget(edge);
            if (to == node)
            {
                const auto value = UseDuration ? data.duration : data.weight;
                loop_weight = std::min(loop_weight, value);
            }
        }
    }
    return loop_weight;
}

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
 * @param callback void(const std::pair<NodeID, NodeID>, const EdgeID &) called for each
 * original edge found.
 */

// Function needs to:
// 1. SUM UP THE DURATION OF THE PATH
// 2. UNPACK THE PATH

// two stacks --
// one is for the edges
// one is for the associated durations

// edge stack
// --------------
// get edge (u,v)
// check if it's in the cache:
// - if it's in the cache, put the value in the duration stack
//     continue with the next element in the stack
// - if it's not in the cache:
//     did we process the child edge yet? yes if we have seen the edge before --> move to steps in
//     duration stack steps
//     if no, we need to determine child edges by executing the "normal body"

// -------------------------------------------------------
// duration computation using method similar to annotatePath but limited to single edge duration
// caculations
// --------------------------------------------------------

// --------------------------------------------------------
// duration computation if we have children (duration stack)
// --------------------------------------------------------
// 1. every time we see an edge we've seen before (u,v) (from a boolean flag maybe) <- this means
// we've already processed it's child edges
// 2. take the top two duration values and sum them up (pop them out of the stack)<-- this is the
// duration value for (u,v)
// 3. put this summed value, suv, into the cache
// 4. push the summed value onto the top of this durations stack

// (should have two values at the beginning of this process, and a total of one less at the end of
// it)

// ------------------------------
// have we processed the edge before? tells us if we have values in the durations stack that we can add up
// if yes, pop top values of duration_stack and add them up and push onto duration_stack
// if no, check if its in the cache
//      if yes, add duration value to stack
//      if no, either find the children OR we get to the point where we use the single edge duration method (which is like annotatePath) and add do duration_stack

template <typename BidirectionalIterator, typename Callback>
EdgeDuration unpackPath(const DataFacade<Algorithm> &facade,
                BidirectionalIterator packed_path_begin,
                BidirectionalIterator packed_path_end,
                Callback &&callback)
{
    UnpackingCache unpacking_cache;
    return unpackPath(facade, packed_path_begin, packed_path_end, unpacking_cache, callback);
}
template <typename BidirectionalIterator, typename Callback>
EdgeDuration unpackPath(const DataFacade<Algorithm> &facade,
                BidirectionalIterator packed_path_begin,
                BidirectionalIterator packed_path_end,
                UnpackingCache &unpacking_cache,
                Callback &&callback)
{
    // make sure we have at least something to unpack
    if (packed_path_begin == packed_path_end)
        return 0;

    // std::stack<std::pair<NodeID, NodeID>> recursion_stack;
    std::stack<std::tuple<NodeID, NodeID, bool>> recursion_stack;
    std::stack<EdgeDuration> duration_stack;

    // We have to push the path in reverse order onto the stack because it's LIFO.
    for (auto current = std::prev(packed_path_end); current != packed_path_begin;
         current = std::prev(current))
    {
        // recursion_stack.emplace(*std::prev(current), *current);
        recursion_stack.emplace(*std::prev(current), *current, false);
    }

    // std::pair<NodeID, NodeID> edge;
    std::tuple<NodeID, NodeID, bool> edge;
    while (!recursion_stack.empty())
    {
        edge = recursion_stack.top();
        std::cout << "Processing edge: " << std::get<0>(edge) << ", " << std::get<1>(edge) << std::endl;
        recursion_stack.pop();

        // have we processed the edge before? tells us if we have values in the durations stack that we can add up
        if (!std::get<2>(edge)) { // haven't processed edge before, so process it in the body!

            std::get<2>(edge) = true; // mark that this edge will now be processed

            if (unpacking_cache.IsEdgeInCache(std::make_pair(std::get<0>(edge), std::get<1>(edge)))) {
                std::cout << "Edge is in cache" << std::endl;
                EdgeDuration duration = unpacking_cache.GetDuration(std::make_pair(std::get<0>(edge), std::get<1>(edge)));
                duration_stack.emplace(duration);
                recursion_stack.emplace(edge);
            } else {
                // Look for an edge on the forward CH graph (.forward)
                EdgeID smaller_edge_id = facade.FindSmallestEdge(
                    // edge.first, edge.second, [](const auto &data) { return data.forward; });
                    std::get<0>(edge),
                    std::get<1>(edge),
                    [](const auto &data) { return data.forward; });

                // If we didn't find one there, the we might be looking at a part of the path that
                // was found using the backward search.  Here, we flip the node order (.second, .first)
                // and only consider edges with the `.backward` flag.
                if (SPECIAL_EDGEID == smaller_edge_id)
                {
                    // smaller_edge_id = facade.FindSmallestEdge(
                    //     edge.second, edge.first, [](const auto &data) { return data.backward; });
                    smaller_edge_id =
                        facade.FindSmallestEdge(std::get<1>(edge), std::get<0>(edge), [](const auto &data) {
                            return data.backward;
                        });
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
                    const NodeID middle_node_id = data.turn_id;
                    // Note the order here - we're adding these to a stack, so we
                    // want the first->middle to get visited before middle->second
                    // recursion_stack.emplace(middle_node_id, edge.second);
                    // recursion_stack.emplace(edge.first, middle_node_id);
                    recursion_stack.emplace(middle_node_id, std::get<1>(edge), false);
                    recursion_stack.emplace(std::get<0>(edge), middle_node_id, false);
                }
                else
                {
                    auto temp = std::make_pair(std::get<0>(edge), std::get<1>(edge));
                    // We found an original edge, call our callback.
                    // std::forward<Callback>(callback)(edge, smaller_edge_id);
                    std::forward<Callback>(callback)(temp, smaller_edge_id);
                    // compute the duration here and put it onto the duration stack using method similar to
                    // annotatePath but smaller
                    EdgeDuration duration = 10;
                    duration_stack.emplace(duration);
                    recursion_stack.emplace(edge);
                }
            }

        } else { // the edge has already been processed. this means that there are enough values in the durations stack
            if (duration_stack.size() >= 2) {
                std::cout << "I'm here!" << std::endl;
                EdgeDuration edge1 = duration_stack.top(); duration_stack.pop(); recursion_stack.pop();
                EdgeDuration edge2 = duration_stack.top(); duration_stack.pop(); recursion_stack.pop();
                duration_stack.emplace(edge1 + edge2); recursion_stack.emplace(edge);
            }
            if (duration_stack.size() == 1) {
            // if (duration_stack.size() == 1 && recursion_stack.size() == 1) {
                std::cout << "No, I'm here! recursion_stack.size():" << recursion_stack.size() << " " << std::endl;

                return duration_stack.top(); duration_stack.pop(); recursion_stack.pop();
            }
        }
        std::cout << "in loop duration_stack.size(): " << duration_stack.size() << " " << "in loop recursion_stack.size(): " << recursion_stack.size() << " recursion_stack.top(): " << std::get<0>(recursion_stack.top()) << "," << std::get<1>(recursion_stack.top()) << " " <<  std::endl;
    }

    std::cout << "Do I ever get here?!" << std::endl; return 0;
}

template <typename RandomIter, typename FacadeT>
EdgeDuration unpackPath(const FacadeT &facade,
                RandomIter packed_path_begin,
                RandomIter packed_path_end,
                const PhantomNodes &phantom_nodes,
                std::vector<PathData> &unpacked_path)
{
    const auto nodes_number = std::distance(packed_path_begin, packed_path_end);
    BOOST_ASSERT(nodes_number > 0);

    std::vector<NodeID> unpacked_nodes;
    std::vector<EdgeID> unpacked_edges;
    unpacked_nodes.reserve(nodes_number);
    unpacked_edges.reserve(nodes_number);

    unpacked_nodes.push_back(*packed_path_begin);
    EdgeDuration duration = 0;
    if (nodes_number > 1)
    {
        duration = unpackPath(facade,
                   packed_path_begin,
                   packed_path_end,
                   [&](std::pair<NodeID, NodeID> &edge, const auto &edge_id) {
                       BOOST_ASSERT(edge.first == unpacked_nodes.back());
                       unpacked_nodes.push_back(edge.second);
                       unpacked_edges.push_back(edge_id);
                   });
    }

    annotatePath(facade, phantom_nodes, unpacked_nodes, unpacked_edges, unpacked_path);
    return duration;
}

/**
 * Unpacks a single edge (NodeID->NodeID) from the CH graph down to it's original non-shortcut
 * route.
 * @param from the node the CH edge starts at
 * @param to the node the CH edge finishes at
 * @param unpacked_path the sequence of original NodeIDs that make up the expanded CH edge
 */
void unpackEdge(const DataFacade<Algorithm> &facade,
                const NodeID from,
                const NodeID to,
                std::vector<NodeID> &unpacked_path);

void retrievePackedPathFromHeap(const SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                                const SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                                const NodeID middle_node_id,
                                std::vector<NodeID> &packed_path);

void retrievePackedPathFromSingleHeap(const SearchEngineData<Algorithm>::QueryHeap &search_heap,
                                      const NodeID middle_node_id,
                                      std::vector<NodeID> &packed_path);

void retrievePackedPathFromSingleManyToManyHeap(
    const SearchEngineData<Algorithm>::ManyToManyQueryHeap &search_heap,
    const NodeID middle_node_id,
    std::vector<NodeID> &packed_path);

// assumes that heaps are already setup correctly.
// ATTENTION: This only works if no additional offset is supplied next to the Phantom Node
// Offsets.
// In case additional offsets are supplied, you might have to force a loop first.
// A forced loop might be necessary, if source and target are on the same segment.
// If this is the case and the offsets of the respective direction are larger for the source
// than the target
// then a force loop is required (e.g. source_phantom.forward_segment_id ==
// target_phantom.forward_segment_id
// && source_phantom.GetForwardWeightPlusOffset() > target_phantom.GetForwardWeightPlusOffset())
// requires
// a force loop, if the heaps have been initialized with positive offsets.
void search(SearchEngineData<Algorithm> &engine_working_data,
            const DataFacade<Algorithm> &facade,
            SearchEngineData<Algorithm>::QueryHeap &forward_heap,
            SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
            std::int32_t &weight,
            std::vector<NodeID> &packed_leg,
            const bool force_loop_forward,
            const bool force_loop_reverse,
            const PhantomNodes &phantom_nodes,
            const int duration_upper_bound = INVALID_EDGE_WEIGHT);

// Requires the heaps for be empty
// If heaps should be adjusted to be initialized outside of this function,
// the addition of force_loop parameters might be required
double getNetworkDistance(SearchEngineData<Algorithm> &engine_working_data,
                          const DataFacade<ch::Algorithm> &facade,
                          SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                          SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                          const PhantomNode &source_phantom,
                          const PhantomNode &target_phantom,
                          int duration_upper_bound = INVALID_EDGE_WEIGHT);

} // namespace ch
} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif // OSRM_ENGINE_ROUTING_BASE_CH_HPP
