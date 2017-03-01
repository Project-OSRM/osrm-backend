#ifndef ROUTING_BASE_HPP
#define ROUTING_BASE_HPP

#include "extractor/guidance/turn_instruction.hpp"

#include "engine/algorithm.hpp"
#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/search_engine_data.hpp"

#include "util/coordinate_calculation.hpp"
#include "util/guidance/turn_bearing.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <stack>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{

namespace routing_algorithms
{
static constexpr bool FORWARD_DIRECTION = true;
static constexpr bool REVERSE_DIRECTION = false;

// Stalling
template <bool DIRECTION, typename HeapT>
bool stallAtNode(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
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
void relaxOutgoingEdges(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
                        const NodeID node,
                        const EdgeWeight weight,
                        SearchEngineData::QueryHeap &heap)
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
static constexpr bool DO_NOT_FORCE_LOOPS = false;
template <bool DIRECTION, bool STALLING = ENABLE_STALLING>
void routingStep(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
                 SearchEngineData::QueryHeap &forward_heap,
                 SearchEngineData::QueryHeap &reverse_heap,
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
EdgeWeight
getLoopWeight(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
              NodeID node)
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
 * @param callback void(const std::pair<NodeID, NodeID>, const EdgeData &) called for each
 * original edge found.
 */
template <typename BidirectionalIterator, typename Callback>
void unpackPath(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
                BidirectionalIterator packed_path_begin,
                BidirectionalIterator packed_path_end,
                Callback &&callback)
{
    // make sure we have at least something to unpack
    if (packed_path_begin == packed_path_end)
        return;

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
            edge.first, edge.second, [](const auto &data) { return data.forward; });

        // If we didn't find one there, the we might be looking at a part of the path that
        // was found using the backward search.  Here, we flip the node order (.second, .first)
        // and only consider edges with the `.backward` flag.
        if (SPECIAL_EDGEID == smaller_edge_id)
        {
            smaller_edge_id = facade.FindSmallestEdge(
                edge.second, edge.first, [](const auto &data) { return data.backward; });
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

// Should work both for CH and not CH if the unpackPath function above is implemented a proper
// implementation.
template <typename RandomIter, typename FacadeT>
void unpackPath(const FacadeT &facade,
                RandomIter packed_path_begin,
                RandomIter packed_path_end,
                const PhantomNodes &phantom_node_pair,
                std::vector<PathData> &unpacked_path)
{
    BOOST_ASSERT(std::distance(packed_path_begin, packed_path_end) > 0);

    const bool start_traversed_in_reverse =
        (*packed_path_begin != phantom_node_pair.source_phantom.forward_segment_id.id);
    const bool target_traversed_in_reverse =
        (*std::prev(packed_path_end) != phantom_node_pair.target_phantom.forward_segment_id.id);

    BOOST_ASSERT(*packed_path_begin == phantom_node_pair.source_phantom.forward_segment_id.id ||
                 *packed_path_begin == phantom_node_pair.source_phantom.reverse_segment_id.id);
    BOOST_ASSERT(
        *std::prev(packed_path_end) == phantom_node_pair.target_phantom.forward_segment_id.id ||
        *std::prev(packed_path_end) == phantom_node_pair.target_phantom.reverse_segment_id.id);

    unpackPath(
        facade,
        packed_path_begin,
        packed_path_end,
        [&facade,
         &unpacked_path,
         &phantom_node_pair,
         &start_traversed_in_reverse,
         &target_traversed_in_reverse](std::pair<NodeID, NodeID> & /* edge */,
                                       const auto &edge_data) {

            BOOST_ASSERT_MSG(!edge_data.shortcut, "original edge flagged as shortcut");
            const auto name_index = facade.GetNameIndexFromEdgeID(edge_data.id);
            const auto turn_instruction = facade.GetTurnInstructionForEdgeID(edge_data.id);
            const extractor::TravelMode travel_mode =
                (unpacked_path.empty() && start_traversed_in_reverse)
                    ? phantom_node_pair.source_phantom.backward_travel_mode
                    : facade.GetTravelModeForEdgeID(edge_data.id);

            const auto geometry_index = facade.GetGeometryIndexForEdgeID(edge_data.id);
            std::vector<NodeID> id_vector;

            std::vector<EdgeWeight> weight_vector;
            std::vector<EdgeWeight> duration_vector;
            std::vector<DatasourceID> datasource_vector;
            if (geometry_index.forward)
            {
                id_vector = facade.GetUncompressedForwardGeometry(geometry_index.id);
                weight_vector = facade.GetUncompressedForwardWeights(geometry_index.id);
                duration_vector = facade.GetUncompressedForwardDurations(geometry_index.id);
                datasource_vector = facade.GetUncompressedForwardDatasources(geometry_index.id);
            }
            else
            {
                id_vector = facade.GetUncompressedReverseGeometry(geometry_index.id);
                weight_vector = facade.GetUncompressedReverseWeights(geometry_index.id);
                duration_vector = facade.GetUncompressedReverseDurations(geometry_index.id);
                datasource_vector = facade.GetUncompressedReverseDatasources(geometry_index.id);
            }
            BOOST_ASSERT(id_vector.size() > 0);
            BOOST_ASSERT(datasource_vector.size() > 0);
            BOOST_ASSERT(weight_vector.size() == id_vector.size() - 1);
            BOOST_ASSERT(duration_vector.size() == id_vector.size() - 1);
            const bool is_first_segment = unpacked_path.empty();

            const std::size_t start_index =
                (is_first_segment
                     ? ((start_traversed_in_reverse)
                            ? weight_vector.size() -
                                  phantom_node_pair.source_phantom.fwd_segment_position - 1
                            : phantom_node_pair.source_phantom.fwd_segment_position)
                     : 0);
            const std::size_t end_index = weight_vector.size();

            BOOST_ASSERT(start_index >= 0);
            BOOST_ASSERT(start_index < end_index);
            for (std::size_t segment_idx = start_index; segment_idx < end_index; ++segment_idx)
            {
                unpacked_path.push_back(PathData{id_vector[segment_idx + 1],
                                                 name_index,
                                                 weight_vector[segment_idx],
                                                 duration_vector[segment_idx],
                                                 extractor::guidance::TurnInstruction::NO_TURN(),
                                                 {{0, INVALID_LANEID}, INVALID_LANE_DESCRIPTIONID},
                                                 travel_mode,
                                                 INVALID_ENTRY_CLASSID,
                                                 datasource_vector[segment_idx],
                                                 util::guidance::TurnBearing(0),
                                                 util::guidance::TurnBearing(0)});
            }
            BOOST_ASSERT(unpacked_path.size() > 0);
            if (facade.hasLaneData(edge_data.id))
                unpacked_path.back().lane_data = facade.GetLaneData(edge_data.id);

            unpacked_path.back().entry_classid = facade.GetEntryClassID(edge_data.id);
            unpacked_path.back().turn_instruction = turn_instruction;
            unpacked_path.back().duration_until_turn +=
                facade.GetDurationPenaltyForEdgeID(edge_data.id);
            unpacked_path.back().weight_until_turn +=
                facade.GetWeightPenaltyForEdgeID(edge_data.id);
            unpacked_path.back().pre_turn_bearing = facade.PreTurnBearing(edge_data.id);
            unpacked_path.back().post_turn_bearing = facade.PostTurnBearing(edge_data.id);
        });

    std::size_t start_index = 0, end_index = 0;
    std::vector<unsigned> id_vector;
    std::vector<EdgeWeight> weight_vector;
    std::vector<EdgeWeight> duration_vector;
    std::vector<DatasourceID> datasource_vector;
    const bool is_local_path = (phantom_node_pair.source_phantom.packed_geometry_id ==
                                phantom_node_pair.target_phantom.packed_geometry_id) &&
                               unpacked_path.empty();

    if (target_traversed_in_reverse)
    {
        id_vector = facade.GetUncompressedReverseGeometry(
            phantom_node_pair.target_phantom.packed_geometry_id);

        weight_vector = facade.GetUncompressedReverseWeights(
            phantom_node_pair.target_phantom.packed_geometry_id);

        duration_vector = facade.GetUncompressedReverseDurations(
            phantom_node_pair.target_phantom.packed_geometry_id);

        datasource_vector = facade.GetUncompressedReverseDatasources(
            phantom_node_pair.target_phantom.packed_geometry_id);

        if (is_local_path)
        {
            start_index =
                weight_vector.size() - phantom_node_pair.source_phantom.fwd_segment_position - 1;
        }
        end_index =
            weight_vector.size() - phantom_node_pair.target_phantom.fwd_segment_position - 1;
    }
    else
    {
        if (is_local_path)
        {
            start_index = phantom_node_pair.source_phantom.fwd_segment_position;
        }
        end_index = phantom_node_pair.target_phantom.fwd_segment_position;

        id_vector = facade.GetUncompressedForwardGeometry(
            phantom_node_pair.target_phantom.packed_geometry_id);

        weight_vector = facade.GetUncompressedForwardWeights(
            phantom_node_pair.target_phantom.packed_geometry_id);

        duration_vector = facade.GetUncompressedForwardDurations(
            phantom_node_pair.target_phantom.packed_geometry_id);

        datasource_vector = facade.GetUncompressedForwardDatasources(
            phantom_node_pair.target_phantom.packed_geometry_id);
    }

    // Given the following compressed geometry:
    // U---v---w---x---y---Z
    //    s           t
    // s: fwd_segment 0
    // t: fwd_segment 3
    // -> (U, v), (v, w), (w, x)
    // note that (x, t) is _not_ included but needs to be added later.
    for (std::size_t segment_idx = start_index; segment_idx != end_index;
         (start_index < end_index ? ++segment_idx : --segment_idx))
    {
        BOOST_ASSERT(segment_idx < id_vector.size() - 1);
        BOOST_ASSERT(phantom_node_pair.target_phantom.forward_travel_mode > 0);
        unpacked_path.push_back(PathData{
            id_vector[start_index < end_index ? segment_idx + 1 : segment_idx - 1],
            phantom_node_pair.target_phantom.name_id,
            weight_vector[segment_idx],
            duration_vector[segment_idx],
            extractor::guidance::TurnInstruction::NO_TURN(),
            {{0, INVALID_LANEID}, INVALID_LANE_DESCRIPTIONID},
            target_traversed_in_reverse ? phantom_node_pair.target_phantom.backward_travel_mode
                                        : phantom_node_pair.target_phantom.forward_travel_mode,
            INVALID_ENTRY_CLASSID,
            datasource_vector[segment_idx],
            util::guidance::TurnBearing(0),
            util::guidance::TurnBearing(0)});
    }

    if (unpacked_path.size() > 0)
    {
        const auto source_weight = start_traversed_in_reverse
                                       ? phantom_node_pair.source_phantom.reverse_weight
                                       : phantom_node_pair.source_phantom.forward_weight;
        const auto source_duration = start_traversed_in_reverse
                                         ? phantom_node_pair.source_phantom.reverse_duration
                                         : phantom_node_pair.source_phantom.forward_duration;
        // The above code will create segments for (v, w), (w,x), (x, y) and (y, Z).
        // However the first segment duration needs to be adjusted to the fact that the source
        // phantom is in the middle of the segment. We do this by subtracting v--s from the
        // duration.

        // Since it's possible duration_until_turn can be less than source_weight here if
        // a negative enough turn penalty is used to modify this edge weight during
        // osrm-contract, we clamp to 0 here so as not to return a negative duration
        // for this segment.

        // TODO this creates a scenario where it's possible the duration from a phantom
        // node to the first turn would be the same as from end to end of a segment,
        // which is obviously incorrect and not ideal...
        unpacked_path.front().weight_until_turn =
            std::max(unpacked_path.front().weight_until_turn - source_weight, 0);
        unpacked_path.front().duration_until_turn =
            std::max(unpacked_path.front().duration_until_turn - source_duration, 0);
    }

    // there is no equivalent to a node-based node in an edge-expanded graph.
    // two equivalent routes may start (or end) at different node-based edges
    // as they are added with the offset how much "weight" on the edge
    // has already been traversed. Depending on offset one needs to remove
    // the last node.
    if (unpacked_path.size() > 1)
    {
        const std::size_t last_index = unpacked_path.size() - 1;
        const std::size_t second_to_last_index = last_index - 1;

        if (unpacked_path[last_index].turn_via_node ==
            unpacked_path[second_to_last_index].turn_via_node)
        {
            unpacked_path.pop_back();
        }
        BOOST_ASSERT(!unpacked_path.empty());
    }
}

/**
 * Unpacks a single edge (NodeID->NodeID) from the CH graph down to it's original non-shortcut
 * route.
 * @param from the node the CH edge starts at
 * @param to the node the CH edge finishes at
 * @param unpacked_path the sequence of original NodeIDs that make up the expanded CH edge
 */
void unpackEdge(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
                const NodeID from,
                const NodeID to,
                std::vector<NodeID> &unpacked_path);

void retrievePackedPathFromHeap(const SearchEngineData::QueryHeap &forward_heap,
                                const SearchEngineData::QueryHeap &reverse_heap,
                                const NodeID middle_node_id,
                                std::vector<NodeID> &packed_path);

void retrievePackedPathFromSingleHeap(const SearchEngineData::QueryHeap &search_heap,
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
void search(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
            SearchEngineData::QueryHeap &forward_heap,
            SearchEngineData::QueryHeap &reverse_heap,
            std::int32_t &weight,
            std::vector<NodeID> &packed_leg,
            const bool force_loop_forward,
            const bool force_loop_reverse,
            const int duration_upper_bound = INVALID_EDGE_WEIGHT);

// Alias to be compatible with the overload for CoreCH that needs 4 heaps
inline void search(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
                   SearchEngineData::QueryHeap &forward_heap,
                   SearchEngineData::QueryHeap &reverse_heap,
                   SearchEngineData::QueryHeap &,
                   SearchEngineData::QueryHeap &,
                   std::int32_t &weight,
                   std::vector<NodeID> &packed_leg,
                   const bool force_loop_forward,
                   const bool force_loop_reverse,
                   const int duration_upper_bound = INVALID_EDGE_WEIGHT)
{
    search(facade,
           forward_heap,
           reverse_heap,
           weight,
           packed_leg,
           force_loop_forward,
           force_loop_reverse,
           duration_upper_bound);
}

// assumes that heaps are already setup correctly.
// A forced loop might be necessary, if source and target are on the same segment.
// If this is the case and the offsets of the respective direction are larger for the source
// than the target
// then a force loop is required (e.g. source_phantom.forward_segment_id ==
// target_phantom.forward_segment_id
// && source_phantom.GetForwardWeightPlusOffset() > target_phantom.GetForwardWeightPlusOffset())
// requires
// a force loop, if the heaps have been initialized with positive offsets.
void search(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CoreCH> &facade,
            SearchEngineData::QueryHeap &forward_heap,
            SearchEngineData::QueryHeap &reverse_heap,
            SearchEngineData::QueryHeap &forward_core_heap,
            SearchEngineData::QueryHeap &reverse_core_heap,
            int &weight,
            std::vector<NodeID> &packed_leg,
            const bool force_loop_forward,
            const bool force_loop_reverse,
            int duration_upper_bound = INVALID_EDGE_WEIGHT);

bool needsLoopForward(const PhantomNode &source_phantom, const PhantomNode &target_phantom);

bool needsLoopBackwards(const PhantomNode &source_phantom, const PhantomNode &target_phantom);

double getPathDistance(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
                       const std::vector<NodeID> &packed_path,
                       const PhantomNode &source_phantom,
                       const PhantomNode &target_phantom);

// Requires the heaps for be empty
// If heaps should be adjusted to be initialized outside of this function,
// the addition of force_loop parameters might be required
double
getNetworkDistance(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CoreCH> &facade,
                   SearchEngineData::QueryHeap &forward_heap,
                   SearchEngineData::QueryHeap &reverse_heap,
                   SearchEngineData::QueryHeap &forward_core_heap,
                   SearchEngineData::QueryHeap &reverse_core_heap,
                   const PhantomNode &source_phantom,
                   const PhantomNode &target_phantom,
                   int duration_upper_bound = INVALID_EDGE_WEIGHT);

// Requires the heaps for be empty
// If heaps should be adjusted to be initialized outside of this function,
// the addition of force_loop parameters might be required
double
getNetworkDistance(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
                   SearchEngineData::QueryHeap &forward_heap,
                   SearchEngineData::QueryHeap &reverse_heap,
                   const PhantomNode &source_phantom,
                   const PhantomNode &target_phantom,
                   int duration_upper_bound = INVALID_EDGE_WEIGHT);

// Alias to be compatible with the overload for CoreCH that needs 4 heaps
inline double
getNetworkDistance(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
                   SearchEngineData::QueryHeap &forward_heap,
                   SearchEngineData::QueryHeap &reverse_heap,
                   SearchEngineData::QueryHeap &,
                   SearchEngineData::QueryHeap &,
                   const PhantomNode &source_phantom,
                   const PhantomNode &target_phantom,
                   int duration_upper_bound = INVALID_EDGE_WEIGHT)
{
    return getNetworkDistance(
        facade, forward_heap, reverse_heap, source_phantom, target_phantom, duration_upper_bound);
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif // ROUTING_BASE_HPP
