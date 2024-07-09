#ifndef OSRM_ENGINE_ROUTING_BASE_MLD_HPP
#define OSRM_ENGINE_ROUTING_BASE_MLD_HPP

#include "engine/algorithm.hpp"
#include "engine/datafacade.hpp"
#include "engine/routing_algorithms/routing_base.hpp"
#include "engine/search_engine_data.hpp"

#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <boost/core/ignore_unused.hpp>
#include <iterator>
#include <limits>
#include <tuple>
#include <vector>

namespace osrm::engine::routing_algorithms::mld
{

namespace
{
// Unrestricted search (Args is const PhantomNodes &):
//   * use partition.GetQueryLevel to find the node query level based on source and target phantoms
//   * allow to traverse all cells
template <typename MultiLevelPartition>
inline LevelID getNodeQueryLevel(const MultiLevelPartition &partition,
                                 NodeID node,
                                 const PhantomNode &source,
                                 const PhantomNode &target)
{
    auto level = [&partition, node](const SegmentID &source, const SegmentID &target)
    {
        if (source.enabled && target.enabled)
            return partition.GetQueryLevel(source.id, target.id, node);
        return INVALID_LEVEL_ID;
    };

    return std::min(std::min(level(source.forward_segment_id, target.forward_segment_id),
                             level(source.forward_segment_id, target.reverse_segment_id)),
                    std::min(level(source.reverse_segment_id, target.forward_segment_id),
                             level(source.reverse_segment_id, target.reverse_segment_id)));
}

template <typename MultiLevelPartition>
inline LevelID getNodeQueryLevel(const MultiLevelPartition &partition,
                                 NodeID node,
                                 const PhantomEndpoints &endpoints)
{
    return getNodeQueryLevel(partition, node, endpoints.source_phantom, endpoints.target_phantom);
}

template <typename MultiLevelPartition>
inline LevelID getNodeQueryLevel(const MultiLevelPartition &partition,
                                 NodeID node,
                                 const PhantomCandidatesToTarget &endpoint_candidates)
{
    auto min_level = std::accumulate(
        endpoint_candidates.source_phantoms.begin(),
        endpoint_candidates.source_phantoms.end(),
        INVALID_LEVEL_ID,
        [&](LevelID current_level, const PhantomNode &source)
        {
            return std::min(
                current_level,
                getNodeQueryLevel(partition, node, source, endpoint_candidates.target_phantom));
        });
    return min_level;
}

template <typename MultiLevelPartition>
inline LevelID getNodeQueryLevel(const MultiLevelPartition &partition,
                                 NodeID node,
                                 const PhantomEndpointCandidates &endpoint_candidates)
{
    auto min_level = std::accumulate(
        endpoint_candidates.source_phantoms.begin(),
        endpoint_candidates.source_phantoms.end(),
        INVALID_LEVEL_ID,
        [&](LevelID level_1, const PhantomNode &source)
        {
            return std::min(
                level_1,
                std::accumulate(endpoint_candidates.target_phantoms.begin(),
                                endpoint_candidates.target_phantoms.end(),
                                level_1,
                                [&](LevelID level_2, const PhantomNode &target) {
                                    return std::min(
                                        level_2,
                                        getNodeQueryLevel(partition, node, source, target));
                                }));
        });
    return min_level;
}

template <typename PhantomCandidateT>
inline bool checkParentCellRestriction(CellID, const PhantomCandidateT &)
{
    return true;
}

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
                                 const PhantomNodeCandidates &candidates)
{
    auto highest_different_level = [&partition, node](const SegmentID &segment)
    {
        return segment.enabled ? partition.GetHighestDifferentLevel(segment.id, node)
                               : INVALID_LEVEL_ID;
    };

    auto node_level =
        std::accumulate(candidates.begin(),
                        candidates.end(),
                        INVALID_LEVEL_ID,
                        [&](LevelID current_level, const PhantomNode &phantom_node)
                        {
                            auto highest_level =
                                std::min(highest_different_level(phantom_node.forward_segment_id),
                                         highest_different_level(phantom_node.reverse_segment_id));
                            return std::min(current_level, highest_level);
                        });
    return node_level;
}

// Unrestricted search with a single phantom node and a vector of phantom nodes:
//   * use partition.GetQueryLevel to find the node query level
//   * allow to traverse all cells
template <typename MultiLevelPartition>
inline LevelID getNodeQueryLevel(const MultiLevelPartition &partition,
                                 NodeID node,
                                 const std::vector<PhantomNodeCandidates> &candidates_list,
                                 const std::size_t phantom_index,
                                 const std::vector<std::size_t> &phantom_indices)
{
    // Get minimum level over all phantoms of the highest different level with respect to node
    // This is equivalent to min_{∀ source, target} partition.GetQueryLevel(source, node, target)
    auto init = getNodeQueryLevel(partition, node, candidates_list[phantom_index]);
    auto result = std::accumulate(
        phantom_indices.begin(),
        phantom_indices.end(),
        init,
        [&](LevelID level, size_t index)
        { return std::min(level, getNodeQueryLevel(partition, node, candidates_list[index])); });
    return result;
}
} // namespace

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

template <typename Heap>
void insertOrUpdate(Heap &heap,
                    const NodeID node,
                    const EdgeWeight weight,
                    const typename Heap::DataType &data)
{
    const auto heapNode = heap.GetHeapNodeIfWasInserted(node);
    if (!heapNode)
    {
        heap.Insert(node, weight, data);
    }
    else if (weight < heapNode->weight)
    {
        heapNode->data = data;
        heapNode->weight = weight;
        heap.DecreaseKey(*heapNode);
    }
}

template <bool DIRECTION, typename Algorithm, typename Heap, typename... Args>
void relaxOutgoingEdges(const DataFacade<Algorithm> &facade,
                        Heap &forward_heap,
                        const typename Heap::HeapNode &heapNode,
                        const Args &...args)
{
    const auto &partition = facade.GetMultiLevelPartition();
    const auto &cells = facade.GetCellStorage();
    const auto &metric = facade.GetCellMetric();

    const auto level = getNodeQueryLevel(partition, heapNode.node, args...);

    static constexpr auto IS_MAP_MATCHING =
        std::is_same_v<typename SearchEngineData<mld::Algorithm>::MapMatchingQueryHeap, Heap>;

    if (level >= 1 && !heapNode.data.from_clique_arc)
    {
        if constexpr (DIRECTION == FORWARD_DIRECTION)
        {
            // Shortcuts in forward direction
            const auto &cell =
                cells.GetCell(metric, level, partition.GetCell(level, heapNode.node));
            auto destination = cell.GetDestinationNodes().begin();
            auto distance = [&cell, node = heapNode.node ]() -> auto
            {
                if constexpr (IS_MAP_MATCHING)
                {

                    return cell.GetOutDistance(node).begin();
                }
                else
                {
                    boost::ignore_unused(cell, node);
                    return 0;
                }
            }
            ();
            for (auto shortcut_weight : cell.GetOutWeight(heapNode.node))
            {
                BOOST_ASSERT(destination != cell.GetDestinationNodes().end());
                const NodeID to = *destination;

                if (shortcut_weight != INVALID_EDGE_WEIGHT && heapNode.node != to)
                {
                    const EdgeWeight to_weight = heapNode.weight + shortcut_weight;
                    BOOST_ASSERT(to_weight >= heapNode.weight);

                    if constexpr (IS_MAP_MATCHING)
                    {
                        const EdgeDistance to_distance = heapNode.data.distance + *distance;
                        insertOrUpdate(
                            forward_heap, to, to_weight, {heapNode.node, true, to_distance});
                    }
                    else
                    {
                        insertOrUpdate(forward_heap, to, to_weight, {heapNode.node, true});
                    }
                }
                ++destination;
                if constexpr (IS_MAP_MATCHING)
                {
                    ++distance;
                }
            }
        }
        else
        {
            // Shortcuts in backward direction
            const auto &cell =
                cells.GetCell(metric, level, partition.GetCell(level, heapNode.node));
            auto source = cell.GetSourceNodes().begin();
            auto distance = [&cell, node = heapNode.node ]() -> auto
            {
                if constexpr (IS_MAP_MATCHING)
                {

                    return cell.GetInDistance(node).begin();
                }
                else
                {
                    boost::ignore_unused(cell, node);
                    return 0;
                }
            }
            ();
            for (auto shortcut_weight : cell.GetInWeight(heapNode.node))
            {
                BOOST_ASSERT(source != cell.GetSourceNodes().end());
                const NodeID to = *source;

                if (shortcut_weight != INVALID_EDGE_WEIGHT && heapNode.node != to)
                {
                    const EdgeWeight to_weight = heapNode.weight + shortcut_weight;
                    BOOST_ASSERT(to_weight >= heapNode.weight);
                    if constexpr (IS_MAP_MATCHING)
                    {
                        const EdgeDistance to_distance = heapNode.data.distance + *distance;
                        insertOrUpdate(
                            forward_heap, to, to_weight, {heapNode.node, true, to_distance});
                    }
                    else
                    {
                        insertOrUpdate(forward_heap, to, to_weight, {heapNode.node, true});
                    }
                }
                ++source;
                if constexpr (IS_MAP_MATCHING)
                {
                    ++distance;
                }
            }
        }
    }

    // Boundary edges
    for (const auto edge : facade.GetBorderEdgeRange(level, heapNode.node))
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
                    facade.GetNodeWeight(DIRECTION == FORWARD_DIRECTION ? heapNode.node : to);
                const auto turn_penalty = facade.GetWeightPenaltyForEdgeID(edge_data.turn_id);

                // TODO: BOOST_ASSERT(edge_data.weight == node_weight + turn_penalty);

                const EdgeWeight to_weight =
                    heapNode.weight + node_weight + alias_cast<EdgeWeight>(turn_penalty);

                if constexpr (IS_MAP_MATCHING)
                {
                    const auto node_distance =
                        facade.GetNodeDistance(DIRECTION == FORWARD_DIRECTION ? heapNode.node : to);

                    const EdgeDistance to_distance = heapNode.data.distance + node_distance;
                    insertOrUpdate(
                        forward_heap, to, to_weight, {heapNode.node, false, to_distance});
                }
                else
                {
                    insertOrUpdate(forward_heap, to, to_weight, {heapNode.node, false});
                }
            }
        }
    }
}

template <bool DIRECTION, typename Algorithm, typename Heap, typename... Args>
void routingStep(const DataFacade<Algorithm> &facade,
                 Heap &forward_heap,
                 Heap &reverse_heap,
                 NodeID &middle_node,
                 EdgeWeight &path_upper_bound,
                 const std::vector<NodeID> &force_step_nodes,
                 const Args &...args)
{
    const auto heapNode = forward_heap.DeleteMinGetHeapNode();
    const auto weight = heapNode.weight;

    BOOST_ASSERT(!facade.ExcludeNode(heapNode.node));

    // Upper bound for the path source -> target with
    // weight(source -> node) = weight weight(to -> target) ≤ reverse_weight
    // is weight + reverse_weight
    // More tighter upper bound requires additional condition reverse_heap.WasRemoved(to)
    // with weight(to -> target) = reverse_weight and all weights ≥ 0
    const auto reverseHeapNode = reverse_heap.GetHeapNodeIfWasInserted(heapNode.node);
    if (reverseHeapNode)
    {
        auto reverse_weight = reverseHeapNode->weight;
        auto path_weight = weight + reverse_weight;

        if (!shouldForceStep(force_step_nodes, heapNode, *reverseHeapNode) &&
            (path_weight >= EdgeWeight{0}) && (path_weight < path_upper_bound))
        {
            middle_node = heapNode.node;
            path_upper_bound = path_weight;
        }
    }

    // Relax outgoing edges from node
    relaxOutgoingEdges<DIRECTION>(facade, forward_heap, heapNode, args...);
}

// With (s, middle, t) we trace back the paths middle -> s and middle -> t.
// This gives us a packed path (node ids) from the base graph around s and t,
// and overlay node ids otherwise. We then have to unpack the overlay clique
// edges by recursively descending unpacking the path down to the base graph.

using UnpackedNodes = std::vector<NodeID>;
using UnpackedEdges = std::vector<EdgeID>;

struct UnpackedPath
{
    EdgeWeight weight;
    UnpackedNodes nodes;
    UnpackedEdges edges;
};

template <typename Algorithm, typename Heap, typename... Args>
std::optional<std::pair<NodeID, EdgeWeight>> runSearch(const DataFacade<Algorithm> &facade,
                                                       Heap &forward_heap,
                                                       Heap &reverse_heap,
                                                       const std::vector<NodeID> &force_step_nodes,
                                                       EdgeWeight weight_upper_bound,
                                                       const Args &...args)
{
    if (forward_heap.Empty() || reverse_heap.Empty())
    {
        return {};
    }

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
            routingStep<FORWARD_DIRECTION>(
                facade, forward_heap, reverse_heap, middle, weight, force_step_nodes, args...);
            if (!forward_heap.Empty())
                forward_heap_min = forward_heap.MinKey();
        }
        if (!reverse_heap.Empty())
        {
            routingStep<REVERSE_DIRECTION>(
                facade, reverse_heap, forward_heap, middle, weight, force_step_nodes, args...);
            if (!reverse_heap.Empty())
                reverse_heap_min = reverse_heap.MinKey();
        }
    };

    // No path found for both target nodes?
    if (weight >= weight_upper_bound || SPECIAL_NODEID == middle)
    {
        return {};
    }

    return {{middle, weight}};
}

template <typename Algorithm, typename... Args>
UnpackedPath search(SearchEngineData<Algorithm> &engine_working_data,
                    const DataFacade<Algorithm> &facade,
                    typename SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                    typename SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                    const std::vector<NodeID> &force_step_nodes,
                    EdgeWeight weight_upper_bound,
                    const Args &...args)
{
    auto searchResult = runSearch(
        facade, forward_heap, reverse_heap, force_step_nodes, weight_upper_bound, args...);
    if (!searchResult)
    {
        return {INVALID_EDGE_WEIGHT, std::vector<NodeID>(), std::vector<EdgeID>()};
    }

    auto [middle, weight] = *searchResult;

    const auto &partition = facade.GetMultiLevelPartition();

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
        auto [source, target, overlay_edge] = packed_edge;
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
            forward_heap.Insert(source, {0}, {source});
            reverse_heap.Insert(target, {0}, {target});

            auto unpacked_subpath = search(engine_working_data,
                                           facade,
                                           forward_heap,
                                           reverse_heap,
                                           force_step_nodes,
                                           INVALID_EDGE_WEIGHT,
                                           sublevel,
                                           parent_cell_id);
            BOOST_ASSERT(!unpacked_subpath.edges.empty());
            BOOST_ASSERT(unpacked_subpath.nodes.size() > 1);
            BOOST_ASSERT(unpacked_subpath.nodes.front() == source);
            BOOST_ASSERT(unpacked_subpath.nodes.back() == target);
            unpacked_nodes.insert(unpacked_nodes.end(),
                                  std::next(unpacked_subpath.nodes.begin()),
                                  unpacked_subpath.nodes.end());
            unpacked_edges.insert(
                unpacked_edges.end(), unpacked_subpath.edges.begin(), unpacked_subpath.edges.end());
        }
    }

    return {weight, std::move(unpacked_nodes), std::move(unpacked_edges)};
}

template <typename Algorithm, typename... Args>
EdgeDistance
searchDistance(SearchEngineData<Algorithm> &,
               const DataFacade<Algorithm> &facade,
               typename SearchEngineData<Algorithm>::MapMatchingQueryHeap &forward_heap,
               typename SearchEngineData<Algorithm>::MapMatchingQueryHeap &reverse_heap,
               const std::vector<NodeID> &force_step_nodes,
               EdgeWeight weight_upper_bound,
               const Args &...args)
{

    auto searchResult = runSearch(
        facade, forward_heap, reverse_heap, force_step_nodes, weight_upper_bound, args...);
    if (!searchResult)
    {
        return INVALID_EDGE_DISTANCE;
    }

    auto [middle, _] = *searchResult;

    auto distance = forward_heap.GetData(middle).distance + reverse_heap.GetData(middle).distance;

    return distance;
}

// Alias to be compatible with the CH-based search
template <typename Algorithm, typename PhantomEndpointT>
inline void search(SearchEngineData<Algorithm> &engine_working_data,
                   const DataFacade<Algorithm> &facade,
                   typename SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                   typename SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                   EdgeWeight &weight,
                   std::vector<NodeID> &unpacked_nodes,
                   const std::vector<NodeID> &force_step_nodes,
                   const PhantomEndpointT &endpoints,
                   const EdgeWeight weight_upper_bound = INVALID_EDGE_WEIGHT)
{
    // TODO: change search calling interface to use unpacked_edges result
    auto unpacked_path = search(engine_working_data,
                                facade,
                                forward_heap,
                                reverse_heap,
                                force_step_nodes,
                                weight_upper_bound,
                                endpoints);
    weight = unpacked_path.weight;
    unpacked_nodes = std::move(unpacked_path.nodes);
}

// TODO: refactor CH-related stub to use unpacked_edges
template <typename RandomIter, typename FacadeT>
void unpackPath(const FacadeT &facade,
                RandomIter packed_path_begin,
                RandomIter packed_path_end,
                const PhantomEndpoints &route_endpoints,
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
            [&facade, &unpacked_nodes, &unpacked_edges](const auto from, const auto to)
            {
                unpacked_nodes.push_back(to);
                unpacked_edges.push_back(facade.FindEdge(from, to));
            });
    }

    annotatePath(facade, route_endpoints, unpacked_nodes, unpacked_edges, unpacked_path);
}

template <typename Algorithm>
double getNetworkDistance(SearchEngineData<Algorithm> &engine_working_data,
                          const DataFacade<Algorithm> &facade,
                          typename SearchEngineData<Algorithm>::MapMatchingQueryHeap &forward_heap,
                          typename SearchEngineData<Algorithm>::MapMatchingQueryHeap &reverse_heap,
                          const PhantomNode &source_phantom,
                          const PhantomNode &target_phantom,
                          EdgeWeight weight_upper_bound = INVALID_EDGE_WEIGHT)
{
    forward_heap.Clear();
    reverse_heap.Clear();

    if (source_phantom.IsValidForwardSource())
    {
        forward_heap.Insert(source_phantom.forward_segment_id.id,
                            EdgeWeight{0} - source_phantom.GetForwardWeightPlusOffset(),
                            {source_phantom.forward_segment_id.id,
                             false,
                             EdgeDistance{0} - source_phantom.GetForwardDistance()});
    }

    if (source_phantom.IsValidReverseSource())
    {
        forward_heap.Insert(source_phantom.reverse_segment_id.id,
                            EdgeWeight{0} - source_phantom.GetReverseWeightPlusOffset(),
                            {source_phantom.reverse_segment_id.id,
                             false,
                             EdgeDistance{0} - source_phantom.GetReverseDistance()});
    }

    if (target_phantom.IsValidForwardTarget())
    {
        reverse_heap.Insert(
            target_phantom.forward_segment_id.id,
            target_phantom.GetForwardWeightPlusOffset(),
            {target_phantom.forward_segment_id.id, false, target_phantom.GetForwardDistance()});
    }

    if (target_phantom.IsValidReverseTarget())
    {
        reverse_heap.Insert(
            target_phantom.reverse_segment_id.id,
            target_phantom.GetReverseWeightPlusOffset(),
            {target_phantom.reverse_segment_id.id, false, target_phantom.GetReverseDistance()});
    }

    const PhantomEndpoints endpoints{source_phantom, target_phantom};

    auto distance = searchDistance(
        engine_working_data, facade, forward_heap, reverse_heap, {}, weight_upper_bound, endpoints);

    if (distance == INVALID_EDGE_DISTANCE)
    {
        return std::numeric_limits<double>::max();
    }
    return from_alias<double>(distance);
}

} // namespace osrm::engine::routing_algorithms::mld

#endif // OSRM_ENGINE_ROUTING_BASE_MLD_HPP
