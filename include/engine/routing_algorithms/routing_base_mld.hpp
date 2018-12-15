#ifndef OSRM_ENGINE_ROUTING_BASE_MLD_HPP
#define OSRM_ENGINE_ROUTING_BASE_MLD_HPP

#include "engine/algorithm.hpp"
#include "engine/datafacade.hpp"
#include "engine/routing_algorithms/routing_base.hpp"
#include "engine/search_engine_data.hpp"

#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <iterator>
#include <limits>
#include <tuple>
#include <vector>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{
namespace mld
{

namespace
{
// Unrestricted search (Args is const PhantomNodes &):
//   * use partition.GetQueryLevel to find the node query level based on source and target phantoms
//   * allow to traverse all cells
template <typename MultiLevelPartition>
inline LevelID getNodeQueryLevel(const MultiLevelPartition &partition,
                                 NodeID node,
                                 const PhantomNodes &phantom_nodes)
{
    auto level = [&partition, node](const SegmentID &source, const SegmentID &target) {
        if (source.enabled && target.enabled)
            return partition.GetQueryLevel(source.id, target.id, node);
        return INVALID_LEVEL_ID;
    };
    return std::min(std::min(level(phantom_nodes.source_phantom.forward_segment_id,
                                   phantom_nodes.target_phantom.forward_segment_id),
                             level(phantom_nodes.source_phantom.forward_segment_id,
                                   phantom_nodes.target_phantom.reverse_segment_id)),
                    std::min(level(phantom_nodes.source_phantom.reverse_segment_id,
                                   phantom_nodes.target_phantom.forward_segment_id),
                             level(phantom_nodes.source_phantom.reverse_segment_id,
                                   phantom_nodes.target_phantom.reverse_segment_id)));
}

inline bool checkParentCellRestriction(CellID, const PhantomNodes &) { return true; }

// Restricted search (Args is LevelID, CellID):
//   * use the fixed level for queries
//   * check if the node cell is the same as the specified parent
template <typename MultiLevelPartition>
inline LevelID getNodeQueryLevel(const MultiLevelPartition &, NodeID, LevelID level, CellID)
{
    return level;
}

inline bool checkParentCellRestriction(CellID cell, LevelID, CellID parent)
{
    return cell == parent;
}

// Unrestricted search with a single phantom node (Args is const PhantomNode &):
//   * use partition.GetQueryLevel to find the node query level
//   * allow to traverse all cells
template <typename MultiLevelPartition>
inline LevelID getNodeQueryLevel(const MultiLevelPartition &partition,
                                 const NodeID node,
                                 const PhantomNode &phantom_node)
{
    auto highest_diffrent_level = [&partition, node](const SegmentID &phantom_node) {
        if (phantom_node.enabled)
            return partition.GetHighestDifferentLevel(phantom_node.id, node);
        return INVALID_LEVEL_ID;
    };

    const auto node_level = std::min(highest_diffrent_level(phantom_node.forward_segment_id),
                                     highest_diffrent_level(phantom_node.reverse_segment_id));

    return node_level;
}

// Unrestricted search with a single phantom node and a vector of phantom nodes:
//   * use partition.GetQueryLevel to find the node query level
//   * allow to traverse all cells
template <typename MultiLevelPartition>
inline LevelID getNodeQueryLevel(const MultiLevelPartition &partition,
                                 NodeID node,
                                 const std::vector<PhantomNode> &phantom_nodes,
                                 const std::size_t phantom_index,
                                 const std::vector<std::size_t> &phantom_indices)
{
    auto min_level = [&partition, node](const PhantomNode &phantom_node) {

        const auto &forward_segment = phantom_node.forward_segment_id;
        const auto forward_level =
            forward_segment.enabled ? partition.GetHighestDifferentLevel(node, forward_segment.id)
                                    : INVALID_LEVEL_ID;

        const auto &reverse_segment = phantom_node.reverse_segment_id;
        const auto reverse_level =
            reverse_segment.enabled ? partition.GetHighestDifferentLevel(node, reverse_segment.id)
                                    : INVALID_LEVEL_ID;

        return std::min(forward_level, reverse_level);
    };

    // Get minimum level over all phantoms of the highest different level with respect to node
    // This is equivalent to min_{∀ source, target} partition.GetQueryLevel(source, node, target)
    auto result = min_level(phantom_nodes[phantom_index]);
    for (const auto &index : phantom_indices)
    {
        result = std::min(result, min_level(phantom_nodes[index]));
    }
    return result;
}
}

// Heaps only record for each node its predecessor ("parent") on the shortest path.
// For re-constructing the actual path we need to trace back all parent "pointers".
// In contrast to the CH code MLD needs to know the edges (with clique arc property).

using PackedEdge = std::tuple</*from*/ NodeID, /*to*/ NodeID, /*from_clique_arc*/ bool>;
using PackedPath = std::vector<PackedEdge>;

template <bool DIRECTION, typename OutIter>
inline void retrievePackedPathFromSingleManyToManyHeap(
    const SearchEngineData<Algorithm>::ManyToManyQueryHeap &heap, const NodeID middle, OutIter out)
{

    NodeID current = middle;
    NodeID parent = heap.GetData(current).parent;

    while (current != parent)
    {
        const auto &data = heap.GetData(current);

        if (DIRECTION == FORWARD_DIRECTION)
        {
            *out = std::make_tuple(parent, current, data.from_clique_arc);
            ++out;
        }
        else if (DIRECTION == REVERSE_DIRECTION)
        {
            *out = std::make_tuple(current, parent, data.from_clique_arc);
            ++out;
        }

        current = parent;
        parent = heap.GetData(parent).parent;
    }
}

template <bool DIRECTION>
inline PackedPath retrievePackedPathFromSingleManyToManyHeap(
    const SearchEngineData<Algorithm>::ManyToManyQueryHeap &heap, const NodeID middle)
{

    PackedPath packed_path;
    retrievePackedPathFromSingleManyToManyHeap<DIRECTION>(
        heap, middle, std::back_inserter(packed_path));

    return packed_path;
}

template <bool DIRECTION, typename OutIter>
inline void retrievePackedPathFromSingleHeap(const SearchEngineData<Algorithm>::QueryHeap &heap,
                                             const NodeID middle,
                                             OutIter out)
{
    NodeID current = middle;
    NodeID parent = heap.GetData(current).parent;

    while (current != parent)
    {
        const auto &data = heap.GetData(current);

        if (DIRECTION == FORWARD_DIRECTION)
        {
            *out = std::make_tuple(parent, current, data.from_clique_arc);
            ++out;
        }
        else if (DIRECTION == REVERSE_DIRECTION)
        {
            *out = std::make_tuple(current, parent, data.from_clique_arc);
            ++out;
        }

        current = parent;
        parent = heap.GetData(parent).parent;
    }
}

template <bool DIRECTION>
inline PackedPath
retrievePackedPathFromSingleHeap(const SearchEngineData<Algorithm>::QueryHeap &heap,
                                 const NodeID middle)
{
    PackedPath packed_path;
    retrievePackedPathFromSingleHeap<DIRECTION>(heap, middle, std::back_inserter(packed_path));
    return packed_path;
}

// Trace path from middle to start in the forward search space (in reverse)
// and from middle to end in the reverse search space. Middle connects paths.

inline PackedPath
retrievePackedPathFromHeap(const SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                           const SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                           const NodeID middle)
{
    // Retrieve start -> middle. Is in reverse order since tracing back starts from middle.
    auto packed_path = retrievePackedPathFromSingleHeap<FORWARD_DIRECTION>(forward_heap, middle);
    std::reverse(begin(packed_path), end(packed_path));

    // Retrieve middle -> end. Is already in correct order, tracing starts from middle.
    auto into = std::back_inserter(packed_path);
    retrievePackedPathFromSingleHeap<REVERSE_DIRECTION>(reverse_heap, middle, into);

    return packed_path;
}

template <bool DIRECTION, typename Algorithm, typename... Args>
void relaxOutgoingEdges(const DataFacade<Algorithm> &facade,
                        typename SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                        const NodeID node,
                        const EdgeWeight weight,
                        Args... args)
{
    const auto &partition = facade.GetMultiLevelPartition();
    const auto &cells = facade.GetCellStorage();
    const auto &metric = facade.GetCellMetric();

    const auto level = getNodeQueryLevel(partition, node, args...);

    if (level >= 1 && !forward_heap.GetData(node).from_clique_arc)
    {
        if (DIRECTION == FORWARD_DIRECTION)
        {
            // Shortcuts in forward direction
            const auto &cell = cells.GetCell(metric, level, partition.GetCell(level, node));
            auto destination = cell.GetDestinationNodes().begin();
            for (auto shortcut_weight : cell.GetOutWeight(node))
            {
                BOOST_ASSERT(destination != cell.GetDestinationNodes().end());
                const NodeID to = *destination;

                if (shortcut_weight != INVALID_EDGE_WEIGHT && node != to)
                {
                    const EdgeWeight to_weight = weight + shortcut_weight;
                    BOOST_ASSERT(to_weight >= weight);
                    if (!forward_heap.WasInserted(to))
                    {
                        forward_heap.Insert(to, to_weight, {node, true});
                    }
                    else if (to_weight < forward_heap.GetKey(to))
                    {
                        forward_heap.GetData(to) = {node, true};
                        forward_heap.DecreaseKey(to, to_weight);
                    }
                }
                ++destination;
            }
        }
        else
        {
            // Shortcuts in backward direction
            const auto &cell = cells.GetCell(metric, level, partition.GetCell(level, node));
            auto source = cell.GetSourceNodes().begin();
            for (auto shortcut_weight : cell.GetInWeight(node))
            {
                BOOST_ASSERT(source != cell.GetSourceNodes().end());
                const NodeID to = *source;

                if (shortcut_weight != INVALID_EDGE_WEIGHT && node != to)
                {
                    const EdgeWeight to_weight = weight + shortcut_weight;
                    BOOST_ASSERT(to_weight >= weight);
                    if (!forward_heap.WasInserted(to))
                    {
                        forward_heap.Insert(to, to_weight, {node, true});
                    }
                    else if (to_weight < forward_heap.GetKey(to))
                    {
                        forward_heap.GetData(to) = {node, true};
                        forward_heap.DecreaseKey(to, to_weight);
                    }
                }
                ++source;
            }
        }
    }

    // Boundary edges
    for (const auto edge : facade.GetBorderEdgeRange(level, node))
    {
        const auto &edge_data = facade.GetEdgeData(edge);

        if ((DIRECTION == FORWARD_DIRECTION) ? facade.IsForwardEdge(edge)
                                             : facade.IsBackwardEdge(edge))
        {
            const NodeID to = facade.GetTarget(edge);

            if (!facade.ExcludeNode(to) &&
                checkParentCellRestriction(partition.GetCell(level + 1, to), args...))
            {
                const auto node_weight =
                    facade.GetNodeWeight(DIRECTION == FORWARD_DIRECTION ? node : to);
                const auto turn_penalty = facade.GetWeightPenaltyForEdgeID(edge_data.turn_id);

                // TODO: BOOST_ASSERT(edge_data.weight == node_weight + turn_penalty);

                const EdgeWeight to_weight = weight + node_weight + turn_penalty;

                if (!forward_heap.WasInserted(to))
                {
                    forward_heap.Insert(to, to_weight, {node, false});
                }
                else if (to_weight < forward_heap.GetKey(to))
                {
                    forward_heap.GetData(to) = {node, false};
                    forward_heap.DecreaseKey(to, to_weight);
                }
            }
        }
    }
}

template <bool DIRECTION, typename Algorithm, typename... Args>
void routingStep(const DataFacade<Algorithm> &facade,
                 typename SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                 typename SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                 NodeID &middle_node,
                 EdgeWeight &path_upper_bound,
                 const bool force_loop_forward,
                 const bool force_loop_reverse,
                 Args... args)
{
    const auto node = forward_heap.DeleteMin();
    const auto weight = forward_heap.GetKey(node);

    BOOST_ASSERT(!facade.ExcludeNode(node));

    // Upper bound for the path source -> target with
    // weight(source -> node) = weight weight(to -> target) ≤ reverse_weight
    // is weight + reverse_weight
    // More tighter upper bound requires additional condition reverse_heap.WasRemoved(to)
    // with weight(to -> target) = reverse_weight and all weights ≥ 0
    if (reverse_heap.WasInserted(node))
    {
        auto reverse_weight = reverse_heap.GetKey(node);
        auto path_weight = weight + reverse_weight;

        // MLD uses loops forcing only to prune single node paths in forward and/or
        // backward direction (there is no need to force loops in MLD but in CH)
        if (!(force_loop_forward && forward_heap.GetData(node).parent == node) &&
            !(force_loop_reverse && reverse_heap.GetData(node).parent == node) &&
            (path_weight >= 0) && (path_weight < path_upper_bound))
        {
            middle_node = node;
            path_upper_bound = path_weight;
        }
    }

    // Relax outgoing edges from node
    relaxOutgoingEdges<DIRECTION>(facade, forward_heap, node, weight, args...);
}

// With (s, middle, t) we trace back the paths middle -> s and middle -> t.
// This gives us a packed path (node ids) from the base graph around s and t,
// and overlay node ids otherwise. We then have to unpack the overlay clique
// edges by recursively descending unpacking the path down to the base graph.

using UnpackedNodes = std::vector<NodeID>;
using UnpackedEdges = std::vector<EdgeID>;
using UnpackedPath = std::tuple<EdgeWeight, UnpackedNodes, UnpackedEdges>;

template <typename Algorithm, typename... Args>
UnpackedPath search(SearchEngineData<Algorithm> &engine_working_data,
                    const DataFacade<Algorithm> &facade,
                    typename SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                    typename SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                    const bool force_loop_forward,
                    const bool force_loop_reverse,
                    EdgeWeight weight_upper_bound,
                    Args... args)
{
    if (forward_heap.Empty() || reverse_heap.Empty())
    {
        return std::make_tuple(INVALID_EDGE_WEIGHT, std::vector<NodeID>(), std::vector<EdgeID>());
    }

    const auto &partition = facade.GetMultiLevelPartition();

    BOOST_ASSERT(!forward_heap.Empty() && forward_heap.MinKey() < INVALID_EDGE_WEIGHT);
    BOOST_ASSERT(!reverse_heap.Empty() && reverse_heap.MinKey() < INVALID_EDGE_WEIGHT);

    // run two-Target Dijkstra routing step.
    NodeID middle = SPECIAL_NODEID;
    EdgeWeight weight = weight_upper_bound;
    EdgeWeight forward_heap_min = forward_heap.MinKey();
    EdgeWeight reverse_heap_min = reverse_heap.MinKey();
    while (forward_heap.Size() + reverse_heap.Size() > 0 &&
           forward_heap_min + reverse_heap_min < weight)
    {
        if (!forward_heap.Empty())
        {
            routingStep<FORWARD_DIRECTION>(facade,
                                           forward_heap,
                                           reverse_heap,
                                           middle,
                                           weight,
                                           force_loop_forward,
                                           force_loop_reverse,
                                           args...);
            if (!forward_heap.Empty())
                forward_heap_min = forward_heap.MinKey();
        }
        if (!reverse_heap.Empty())
        {
            routingStep<REVERSE_DIRECTION>(facade,
                                           reverse_heap,
                                           forward_heap,
                                           middle,
                                           weight,
                                           force_loop_reverse,
                                           force_loop_forward,
                                           args...);
            if (!reverse_heap.Empty())
                reverse_heap_min = reverse_heap.MinKey();
        }
    };

    // No path found for both target nodes?
    if (weight >= weight_upper_bound || SPECIAL_NODEID == middle)
    {
        return std::make_tuple(INVALID_EDGE_WEIGHT, std::vector<NodeID>(), std::vector<EdgeID>());
    }

    // Get packed path as edges {from node ID, to node ID, from_clique_arc}
    auto packed_path = retrievePackedPathFromHeap(forward_heap, reverse_heap, middle);

    // Beware the edge case when start, middle, end are all the same.
    // In this case we return a single node, no edges. We also don't unpack.
    const NodeID source_node = !packed_path.empty() ? std::get<0>(packed_path.front()) : middle;

    // Unpack path
    std::vector<NodeID> unpacked_nodes;
    std::vector<EdgeID> unpacked_edges;
    unpacked_nodes.reserve(packed_path.size());
    unpacked_edges.reserve(packed_path.size());

    unpacked_nodes.push_back(source_node);

    for (auto const &packed_edge : packed_path)
    {
        NodeID source, target;
        bool overlay_edge;
        std::tie(source, target, overlay_edge) = packed_edge;
        if (!overlay_edge)
        { // a base graph edge
            unpacked_nodes.push_back(target);
            unpacked_edges.push_back(facade.FindEdge(source, target));
        }
        else
        { // an overlay graph edge
            LevelID level = getNodeQueryLevel(partition, source, args...);
            CellID parent_cell_id = partition.GetCell(level, source);
            BOOST_ASSERT(parent_cell_id == partition.GetCell(level, target));

            LevelID sublevel = level - 1;

            // Here heaps can be reused, let's go deeper!
            forward_heap.Clear();
            reverse_heap.Clear();
            forward_heap.Insert(source, 0, {source});
            reverse_heap.Insert(target, 0, {target});

            // TODO: when structured bindings will be allowed change to
            // auto [subpath_weight, subpath_source, subpath_target, subpath] = ...
            EdgeWeight subpath_weight;
            std::vector<NodeID> subpath_nodes;
            std::vector<EdgeID> subpath_edges;
            std::tie(subpath_weight, subpath_nodes, subpath_edges) = search(engine_working_data,
                                                                            facade,
                                                                            forward_heap,
                                                                            reverse_heap,
                                                                            force_loop_forward,
                                                                            force_loop_reverse,
                                                                            INVALID_EDGE_WEIGHT,
                                                                            sublevel,
                                                                            parent_cell_id);
            BOOST_ASSERT(!subpath_edges.empty());
            BOOST_ASSERT(subpath_nodes.size() > 1);
            BOOST_ASSERT(subpath_nodes.front() == source);
            BOOST_ASSERT(subpath_nodes.back() == target);
            unpacked_nodes.insert(
                unpacked_nodes.end(), std::next(subpath_nodes.begin()), subpath_nodes.end());
            unpacked_edges.insert(unpacked_edges.end(), subpath_edges.begin(), subpath_edges.end());
        }
    }

    return std::make_tuple(weight, std::move(unpacked_nodes), std::move(unpacked_edges));
}

// Alias to be compatible with the CH-based search
template <typename Algorithm>
inline void search(SearchEngineData<Algorithm> &engine_working_data,
                   const DataFacade<Algorithm> &facade,
                   typename SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                   typename SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                   EdgeWeight &weight,
                   std::vector<NodeID> &unpacked_nodes,
                   const bool force_loop_forward,
                   const bool force_loop_reverse,
                   const PhantomNodes &phantom_nodes,
                   const EdgeWeight weight_upper_bound = INVALID_EDGE_WEIGHT)
{
    // TODO: change search calling interface to use unpacked_edges result
    std::tie(weight, unpacked_nodes, std::ignore) = search(engine_working_data,
                                                           facade,
                                                           forward_heap,
                                                           reverse_heap,
                                                           force_loop_forward,
                                                           force_loop_reverse,
                                                           weight_upper_bound,
                                                           phantom_nodes);
}

// TODO: refactor CH-related stub to use unpacked_edges
template <typename RandomIter, typename FacadeT>
void unpackPath(const FacadeT &facade,
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
    if (nodes_number > 1)
    {
        util::for_each_pair(
            packed_path_begin,
            packed_path_end,
            [&facade, &unpacked_nodes, &unpacked_edges](const auto from, const auto to) {
                unpacked_nodes.push_back(to);
                unpacked_edges.push_back(facade.FindEdge(from, to));
            });
    }

    annotatePath(facade, phantom_nodes, unpacked_nodes, unpacked_edges, unpacked_path);
}

template <typename Algorithm>
double getNetworkDistance(SearchEngineData<Algorithm> &engine_working_data,
                          const DataFacade<Algorithm> &facade,
                          typename SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                          typename SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                          const PhantomNode &source_phantom,
                          const PhantomNode &target_phantom,
                          EdgeWeight weight_upper_bound = INVALID_EDGE_WEIGHT)
{
    forward_heap.Clear();
    reverse_heap.Clear();

    const PhantomNodes phantom_nodes{source_phantom, target_phantom};
    insertNodesInHeaps(forward_heap, reverse_heap, phantom_nodes);

    EdgeWeight weight = INVALID_EDGE_WEIGHT;
    std::vector<NodeID> unpacked_nodes;
    std::vector<EdgeID> unpacked_edges;
    std::tie(weight, unpacked_nodes, unpacked_edges) = search(engine_working_data,
                                                              facade,
                                                              forward_heap,
                                                              reverse_heap,
                                                              DO_NOT_FORCE_LOOPS,
                                                              DO_NOT_FORCE_LOOPS,
                                                              weight_upper_bound,
                                                              phantom_nodes);

    if (weight == INVALID_EDGE_WEIGHT)
    {
        return std::numeric_limits<double>::max();
    }

    std::vector<PathData> unpacked_path;

    annotatePath(facade, phantom_nodes, unpacked_nodes, unpacked_edges, unpacked_path);

    return getPathDistance(facade, unpacked_path, source_phantom, target_phantom);
}

} // namespace mld
} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif // OSRM_ENGINE_ROUTING_BASE_MLD_HPP
