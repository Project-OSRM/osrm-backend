#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/routing_algorithms/routing_base_mld.hpp"

#include <boost/assert.hpp>
#include <boost/range/iterator_range_core.hpp>

#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

namespace mld
{

using PackedEdge = std::tuple</*from*/ NodeID, /*to*/ NodeID, /*from_clique_arc*/ bool>;
using PackedPath = std::vector<PackedEdge>;

template <typename MultiLevelPartition>
inline LevelID getNodeQueryLevel(const MultiLevelPartition &partition,
                                 const NodeID node,
                                 const PhantomNode &phantom_node,
                                 const LevelID maximal_level)
{
    const auto node_level = getNodeQueryLevel(partition, node, phantom_node);

    if (node_level >= maximal_level)
        return INVALID_LEVEL_ID;

    return node_level;
}

template <bool DIRECTION, typename... Args>
void relaxOutgoingEdges(const DataFacade<mld::Algorithm> &facade,
                        const NodeID node,
                        const EdgeWeight weight,
                        const EdgeDuration duration,
                        const EdgeDistance distance,
                        typename SearchEngineData<mld::Algorithm>::ManyToManyQueryHeap &query_heap,
                        Args... args)
{
    BOOST_ASSERT(!facade.ExcludeNode(node));

    const auto &partition = facade.GetMultiLevelPartition();

    const auto level = getNodeQueryLevel(partition, node, args...);

    // Break outgoing edges relaxation if node at the restricted level
    if (level == INVALID_LEVEL_ID)
        return;

    const auto &cells = facade.GetCellStorage();
    const auto &metric = facade.GetCellMetric();
    const auto &node_data = query_heap.GetData(node);

    if (level >= 1 && !node_data.from_clique_arc)
    {
        const auto &cell = cells.GetCell(metric, level, partition.GetCell(level, node));
        if (DIRECTION == FORWARD_DIRECTION)
        { // Shortcuts in forward direction
            auto destination = cell.GetDestinationNodes().begin();
            auto shortcut_durations = cell.GetOutDuration(node);
            auto shortcut_distances = cell.GetOutDistance(node);
            for (auto shortcut_weight : cell.GetOutWeight(node))
            {
                BOOST_ASSERT(destination != cell.GetDestinationNodes().end());
                BOOST_ASSERT(!shortcut_durations.empty());
                BOOST_ASSERT(!shortcut_distances.empty());
                const NodeID to = *destination;

                if (shortcut_weight != INVALID_EDGE_WEIGHT && node != to)
                {
                    const auto to_weight = weight + shortcut_weight;
                    const auto to_duration = duration + shortcut_durations.front();
                    const auto to_distance = distance + shortcut_distances.front();
                    if (!query_heap.WasInserted(to))
                    {
                        query_heap.Insert(to, to_weight, {node, true, to_duration, to_distance});
                    }
                    else if (std::tie(to_weight, to_duration, to_distance, node) <
                             std::tie(query_heap.GetKey(to),
                                      query_heap.GetData(to).duration,
                                      query_heap.GetData(to).distance,
                                      query_heap.GetData(to).parent))
                    {
                        query_heap.GetData(to) = {node, true, to_duration, to_distance};
                        query_heap.DecreaseKey(to, to_weight);
                    }
                }
                ++destination;
                shortcut_durations.advance_begin(1);
                shortcut_distances.advance_begin(1);
            }
            BOOST_ASSERT(shortcut_durations.empty());
            BOOST_ASSERT(shortcut_distances.empty());
        }
        else
        { // Shortcuts in backward direction
            auto source = cell.GetSourceNodes().begin();
            auto shortcut_durations = cell.GetInDuration(node);
            auto shortcut_distances = cell.GetInDistance(node);
            for (auto shortcut_weight : cell.GetInWeight(node))
            {
                BOOST_ASSERT(source != cell.GetSourceNodes().end());
                BOOST_ASSERT(!shortcut_durations.empty());
                BOOST_ASSERT(!shortcut_distances.empty());
                const NodeID to = *source;

                if (shortcut_weight != INVALID_EDGE_WEIGHT && node != to)
                {
                    const auto to_weight = weight + shortcut_weight;
                    const auto to_duration = duration + shortcut_durations.front();
                    const auto to_distance = distance + shortcut_distances.front();
                    if (!query_heap.WasInserted(to))
                    {
                        query_heap.Insert(to, to_weight, {node, true, to_duration, to_distance});
                    }
                    else if (std::tie(to_weight, to_duration, node) <
                             std::tie(query_heap.GetKey(to),
                                      query_heap.GetData(to).duration,
                                      query_heap.GetData(to).distance,
                                      query_heap.GetData(to).parent))
                    {
                        query_heap.GetData(to) = {node, true, to_duration, to_distance};
                        query_heap.DecreaseKey(to, to_weight);
                    }
                }
                ++source;
                shortcut_durations.advance_begin(1);
                shortcut_distances.advance_begin(1);
            }
            BOOST_ASSERT(shortcut_durations.empty());
            BOOST_ASSERT(shortcut_distances.empty());
        }
    }

    for (const auto edge : facade.GetBorderEdgeRange(level, node))
    {
        const auto &data = facade.GetEdgeData(edge);
        if ((DIRECTION == FORWARD_DIRECTION) ? facade.IsForwardEdge(edge)
                                             : facade.IsBackwardEdge(edge))
        {
            const NodeID to = facade.GetTarget(edge);
            if (facade.ExcludeNode(to))
            {
                continue;
            }

            const auto turn_id = data.turn_id;
            const auto node_id = DIRECTION == FORWARD_DIRECTION ? node : facade.GetTarget(edge);
            const auto node_weight = facade.GetNodeWeight(node_id);
            const auto node_duration = facade.GetNodeDuration(node_id);
            const auto node_distance = facade.GetNodeDistance(node_id);
            const auto turn_weight = node_weight + facade.GetWeightPenaltyForEdgeID(turn_id);
            const auto turn_duration = node_duration + facade.GetDurationPenaltyForEdgeID(turn_id);

            BOOST_ASSERT_MSG(node_weight + turn_weight > 0, "edge weight is invalid");
            const auto to_weight = weight + turn_weight;
            const auto to_duration = duration + turn_duration;

            // New Node discovered -> Add to Heap + Node Info Storage
            if (!query_heap.WasInserted(to))
            {
                query_heap.Insert(
                    to,
                    to_weight,
                    {node, false, to_duration, static_cast<EdgeDistance>(node_distance)});
            }
            // Found a shorter Path -> Update weight and set new parent
            else if (std::tie(to_weight, to_duration, node_distance, node) <
                     std::tie(query_heap.GetKey(to),
                              query_heap.GetData(to).duration,
                              query_heap.GetData(to).distance,
                              query_heap.GetData(to).parent))
            {
                query_heap.GetData(to) = {node, false, to_duration, node_distance};
                query_heap.DecreaseKey(to, to_weight);
            }
        }
    }
}

//
// Unidirectional multi-layer Dijkstra search for 1-to-N and N-to-1 matrices
//
template <bool DIRECTION>
std::pair<std::vector<EdgeDuration>, std::vector<EdgeDistance>>
oneToManySearch(SearchEngineData<Algorithm> &engine_working_data,
                const DataFacade<Algorithm> &facade,
                const std::vector<PhantomNode> &phantom_nodes,
                std::size_t phantom_index,
                const std::vector<std::size_t> &phantom_indices,
                const bool /* calculate_distance */)
{
    std::vector<EdgeWeight> weights(phantom_indices.size(), INVALID_EDGE_WEIGHT);
    std::vector<EdgeDuration> durations(phantom_indices.size(), MAXIMAL_EDGE_DURATION);
    std::vector<EdgeDistance> distances_table(phantom_indices.size(), MAXIMAL_EDGE_DISTANCE);
    std::vector<NodeID> middle_nodes_table(phantom_indices.size(), SPECIAL_NODEID);

    // Collect destination (source) nodes into a map
    std::unordered_multimap<NodeID, std::tuple<std::size_t, EdgeWeight, EdgeDuration, EdgeDistance>>
        target_nodes_index;
    target_nodes_index.reserve(phantom_indices.size());
    for (std::size_t index = 0; index < phantom_indices.size(); ++index)
    {
        const auto &phantom_index = phantom_indices[index];
        const auto &phantom_node = phantom_nodes[phantom_index];

        if (DIRECTION == FORWARD_DIRECTION)
        {
            if (phantom_node.IsValidForwardTarget())
                target_nodes_index.insert(
                    {phantom_node.forward_segment_id.id,
                     std::make_tuple(index,
                                     phantom_node.GetForwardWeightPlusOffset(),
                                     phantom_node.GetForwardDuration(),
                                     phantom_node.GetForwardDistance())});
            if (phantom_node.IsValidReverseTarget())
                target_nodes_index.insert(
                    {phantom_node.reverse_segment_id.id,
                     std::make_tuple(index,
                                     phantom_node.GetReverseWeightPlusOffset(),
                                     phantom_node.GetReverseDuration(),
                                     phantom_node.GetReverseDistance())});
        }
        else if (DIRECTION == REVERSE_DIRECTION)
        {
            if (phantom_node.IsValidForwardSource())
                target_nodes_index.insert(
                    {phantom_node.forward_segment_id.id,
                     std::make_tuple(index,
                                     -phantom_node.GetForwardWeightPlusOffset(),
                                     -phantom_node.GetForwardDuration(),
                                     -phantom_node.GetForwardDistance())});
            if (phantom_node.IsValidReverseSource())
                target_nodes_index.insert(
                    {phantom_node.reverse_segment_id.id,
                     std::make_tuple(index,
                                     -phantom_node.GetReverseWeightPlusOffset(),
                                     -phantom_node.GetReverseDuration(),
                                     -phantom_node.GetReverseDistance())});
        }
    }

    // Initialize query heap
    engine_working_data.InitializeOrClearManyToManyThreadLocalStorage(
        facade.GetNumberOfNodes(), facade.GetMaxBorderNodeID() + 1);
    auto &query_heap = *(engine_working_data.many_to_many_heap);

    // Check if node is in the destinations list and update weights/durations
    auto update_values =
        [&](NodeID node, EdgeWeight weight, EdgeDuration duration, EdgeDistance distance) {
            auto candidates = target_nodes_index.equal_range(node);
            for (auto it = candidates.first; it != candidates.second;)
            {
                std::size_t index;
                EdgeWeight target_weight;
                EdgeDuration target_duration;
                EdgeDuration target_distance;
                std::tie(index, target_weight, target_duration, target_distance) = it->second;

                const auto path_weight = weight + target_weight;
                if (path_weight >= 0)
                {
                    const auto path_duration = duration + target_duration;
                    const auto path_distance = distance + target_distance;

                    if (std::tie(path_weight, path_duration) <
                        std::tie(weights[index], durations[index]))
                    {
                        weights[index] = path_weight;
                        durations[index] = path_duration;
                        distances_table[index] = path_distance;
                        middle_nodes_table[index] = node;
                    }

                    // Remove node from destinations list
                    it = target_nodes_index.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        };

    auto insert_node = [&](NodeID node,
                           EdgeWeight initial_weight,
                           EdgeDuration initial_duration,
                           EdgeDistance initial_distance) {
        // Update single node paths
        update_values(node, initial_weight, initial_duration, initial_distance);

        query_heap.Insert(node, initial_weight, {node, initial_duration, initial_distance});

        // Place adjacent nodes into heap
        for (auto edge : facade.GetAdjacentEdgeRange(node))
        {
            const auto &data = facade.GetEdgeData(edge);
            const auto to = facade.GetTarget(edge);

            if (facade.ExcludeNode(to))
            {
                continue;
            }

            if ((DIRECTION == FORWARD_DIRECTION ? facade.IsForwardEdge(edge)
                                                : facade.IsBackwardEdge(edge)) &&
                !query_heap.WasInserted(to))
            {
                const auto turn_id = data.turn_id;
                const auto node_id = DIRECTION == FORWARD_DIRECTION ? node : to;
                const auto edge_weight = initial_weight + facade.GetNodeWeight(node_id) +
                                         facade.GetWeightPenaltyForEdgeID(turn_id);
                const auto edge_duration = initial_duration + facade.GetNodeDuration(node_id) +
                                           facade.GetDurationPenaltyForEdgeID(turn_id);
                const auto edge_distance = initial_distance;

                query_heap.Insert(to, edge_weight, {node, edge_duration, edge_distance});
            }
        }
    };

    { // Place source (destination) adjacent nodes into the heap
        const auto &phantom_node = phantom_nodes[phantom_index];

        if (DIRECTION == FORWARD_DIRECTION)
        {
            if (phantom_node.IsValidForwardSource())
            {
                insert_node(phantom_node.forward_segment_id.id,
                            -phantom_node.GetForwardWeightPlusOffset(),
                            -phantom_node.GetForwardDuration(),
                            -phantom_node.GetForwardDistance());
            }

            if (phantom_node.IsValidReverseSource())
            {
                insert_node(phantom_node.reverse_segment_id.id,
                            -phantom_node.GetReverseWeightPlusOffset(),
                            -phantom_node.GetReverseDuration(),
                            -phantom_node.GetReverseDistance());
            }
        }
        else if (DIRECTION == REVERSE_DIRECTION)
        {
            if (phantom_node.IsValidForwardTarget())
            {
                insert_node(phantom_node.forward_segment_id.id,
                            phantom_node.GetForwardWeightPlusOffset(),
                            phantom_node.GetForwardDuration(),
                            phantom_node.GetForwardDistance());
            }

            if (phantom_node.IsValidReverseTarget())
            {
                insert_node(phantom_node.reverse_segment_id.id,
                            phantom_node.GetReverseWeightPlusOffset(),
                            phantom_node.GetReverseDuration(),
                            phantom_node.GetReverseDistance());
            }
        }
    }

    while (!query_heap.Empty() && !target_nodes_index.empty())
    {
        // Extract node from the heap
        const auto node = query_heap.DeleteMin();
        const auto weight = query_heap.GetKey(node);
        const auto duration = query_heap.GetData(node).duration;
        const auto distance = query_heap.GetData(node).distance;

        // Update values
        update_values(node, weight, duration, distance);

        // Relax outgoing edges
        relaxOutgoingEdges<DIRECTION>(facade,
                                      node,
                                      weight,
                                      duration,
                                      distance,
                                      query_heap,
                                      phantom_nodes,
                                      phantom_index,
                                      phantom_indices);
    }

    // TODO: re-enable this if we need to fallback
    // if (calculate_distance)
    if (false)
    {
        // Initialize unpacking heaps
        engine_working_data.InitializeOrClearFirstThreadLocalStorage(
            facade.GetNumberOfNodes(), facade.GetMaxBorderNodeID() + 1);

        distances_table.resize(phantom_indices.size(), INVALID_EDGE_DISTANCE);

        for (unsigned location = 0; location < phantom_indices.size(); ++location)
        {
            // Get the "middle" node that is the last node of a path
            const NodeID middle_node_id = middle_nodes_table[location];
            if (middle_node_id == SPECIAL_NODEID) // takes care of one-ways
            {
                continue;
            }

            // Retrieve the packed path from the heap
            PackedPath packed_path = mld::retrievePackedPathFromSingleManyToManyHeap<DIRECTION>(
                query_heap, middle_node_id);

            // ... and reverse it to have packed edges in the correct order,
            if (DIRECTION == FORWARD_DIRECTION)
            {
                std::reverse(packed_path.begin(), packed_path.end());
            }

            // ... unpack path
            auto &forward_heap = *engine_working_data.forward_heap_1;
            auto &reverse_heap = *engine_working_data.reverse_heap_1;
            EdgeWeight weight = INVALID_EDGE_WEIGHT;
            std::vector<NodeID> unpacked_nodes;
            std::vector<EdgeID> unpacked_edges;

            std::tie(weight, unpacked_nodes, unpacked_edges) =
                unpackPathAndCalculateDistance(engine_working_data,
                                               facade,
                                               forward_heap,
                                               reverse_heap,
                                               DO_NOT_FORCE_LOOPS,
                                               DO_NOT_FORCE_LOOPS,
                                               INVALID_EDGE_WEIGHT,
                                               packed_path,
                                               middle_node_id,
                                               phantom_nodes,
                                               phantom_index,
                                               phantom_indices);

            // Accumulate the path length without the last node
            auto annotation = 0.0;

            BOOST_ASSERT(!unpacked_nodes.empty());
            for (auto node = unpacked_nodes.begin(), last_node = std::prev(unpacked_nodes.end());
                 node != last_node;
                 ++node)
            {
                annotation += computeEdgeDistance(facade, *node);
            }

            // ... and add negative source and positive target offsets
            // ⚠ for REVERSE_DIRECTION original source and target phantom nodes are swapped
            // Get source and target phantom nodes
            // * 1-to-N: source is a single index, target is the corresponding from the indices list
            // * N-to-1: source is the corresponding from the indices list, target is a single index
            auto source_phantom_index = phantom_index;
            auto target_phantom_index = phantom_indices[location];
            if (DIRECTION == REVERSE_DIRECTION)
            {
                std::swap(source_phantom_index, target_phantom_index);
            }
            const auto &source_phantom = phantom_nodes[source_phantom_index];
            const auto &target_phantom = phantom_nodes[target_phantom_index];
            const NodeID source_node = unpacked_nodes.front();
            const NodeID target_node = unpacked_nodes.back();

            EdgeDistance source_offset = 0., target_offset = 0.;
            if (source_phantom.IsValidForwardSource() &&
                source_phantom.forward_segment_id.id == source_node)
            {
                //       ............       <-- calculateEGBAnnotation returns distance from 0
                //       to 3
                //       -->s               <-- subtract offset to start at source
                //          .........       <-- want this distance as result
                // entry 0---1---2---3---   <-- 3 is exit node
                source_offset = source_phantom.GetForwardDistance();
            }
            else if (source_phantom.IsValidReverseSource() &&
                     source_phantom.reverse_segment_id.id == source_node)
            {
                //       ............    <-- calculateEGBAnnotation returns distance from 0 to 3
                //          s<-------    <-- subtract offset to start at source
                //       ...             <-- want this distance
                // entry 0---1---2---3   <-- 3 is exit node
                source_offset = source_phantom.GetReverseDistance();
            }
            if (target_phantom.IsValidForwardTarget() &&
                target_phantom.forward_segment_id.id == target_node)
            {
                //       ............       <-- calculateEGBAnnotation returns distance from 0
                //       to 3
                //                   ++>t   <-- add offset to get to target
                //       ................   <-- want this distance as result
                // entry 0---1---2---3---   <-- 3 is exit node
                target_offset = target_phantom.GetForwardDistance();
            }
            else if (target_phantom.IsValidReverseTarget() &&
                     target_phantom.reverse_segment_id.id == target_node)
            {
                //       ............       <-- calculateEGBAnnotation returns distance from 0
                //       to 3
                //                   <++t   <-- add offset to get from target
                //       ................   <-- want this distance as result
                // entry 0---1---2---3---   <-- 3 is exit node
                target_offset = target_phantom.GetReverseDistance();
            }

            distances_table[location] = -source_offset + annotation + target_offset;
        }
    }

    return std::make_pair(durations, distances_table);
}

//
// Bidirectional multi-layer Dijkstra search for M-to-N matrices
//
template <bool DIRECTION>
void forwardRoutingStep(const DataFacade<Algorithm> &facade,
                        const unsigned row_idx,
                        const unsigned number_of_sources,
                        const unsigned number_of_targets,
                        typename SearchEngineData<Algorithm>::ManyToManyQueryHeap &query_heap,
                        const std::vector<NodeBucket> &search_space_with_buckets,
                        std::vector<EdgeWeight> &weights_table,
                        std::vector<EdgeDuration> &durations_table,
                        std::vector<EdgeDistance> &distances_table,
                        std::vector<NodeID> &middle_nodes_table,
                        const PhantomNode &phantom_node)
{
    const auto node = query_heap.DeleteMin();
    const auto source_weight = query_heap.GetKey(node);
    const auto source_duration = query_heap.GetData(node).duration;
    const auto source_distance = query_heap.GetData(node).distance;

    // Check if each encountered node has an entry
    const auto &bucket_list = std::equal_range(search_space_with_buckets.begin(),
                                               search_space_with_buckets.end(),
                                               node,
                                               NodeBucket::Compare());
    for (const auto &current_bucket : boost::make_iterator_range(bucket_list))
    {
        // Get target id from bucket entry
        const auto column_idx = current_bucket.column_index;
        const auto target_weight = current_bucket.weight;
        const auto target_duration = current_bucket.duration;
        const auto target_distance = current_bucket.distance;

        // Get the value location in the results tables:
        //  * row-major direct (row_idx, column_idx) index for forward direction
        //  * row-major transposed (column_idx, row_idx) for reversed direction
        const auto location = DIRECTION == FORWARD_DIRECTION
                                  ? row_idx * number_of_targets + column_idx
                                  : row_idx + column_idx * number_of_sources;
        auto &current_weight = weights_table[location];
        auto &current_duration = durations_table[location];
        auto &current_distance = distances_table[location];

        // Check if new weight is better
        auto new_weight = source_weight + target_weight;
        auto new_duration = source_duration + target_duration;
        auto new_distance = source_distance + target_distance;

        if (new_weight >= 0 &&
            std::tie(new_weight, new_duration, new_distance) <
                std::tie(current_weight, current_duration, current_distance))
        {
            current_weight = new_weight;
            current_duration = new_duration;
            current_distance = new_distance;
            middle_nodes_table[location] = node;
        }
    }

    relaxOutgoingEdges<DIRECTION>(
        facade, node, source_weight, source_duration, source_distance, query_heap, phantom_node);
}

template <bool DIRECTION>
void backwardRoutingStep(const DataFacade<Algorithm> &facade,
                         const unsigned column_idx,
                         typename SearchEngineData<Algorithm>::ManyToManyQueryHeap &query_heap,
                         std::vector<NodeBucket> &search_space_with_buckets,
                         const PhantomNode &phantom_node)
{
    const auto node = query_heap.DeleteMin();
    const auto target_weight = query_heap.GetKey(node);
    const auto target_duration = query_heap.GetData(node).duration;
    const auto target_distance = query_heap.GetData(node).distance;
    const auto parent = query_heap.GetData(node).parent;
    const auto from_clique_arc = query_heap.GetData(node).from_clique_arc;

    // Store settled nodes in search space bucket
    search_space_with_buckets.emplace_back(
        node, parent, from_clique_arc, column_idx, target_weight, target_duration, target_distance);

    const auto &partition = facade.GetMultiLevelPartition();
    const auto maximal_level = partition.GetNumberOfLevels() - 1;

    relaxOutgoingEdges<!DIRECTION>(facade,
                                   node,
                                   target_weight,
                                   target_duration,
                                   target_distance,
                                   query_heap,
                                   phantom_node,
                                   maximal_level);
}

template <bool DIRECTION>
void retrievePackedPathFromSearchSpace(NodeID middle_node_id,
                                       const unsigned column_idx,
                                       const std::vector<NodeBucket> &search_space_with_buckets,
                                       PackedPath &path)
{
    auto bucket_list = std::equal_range(search_space_with_buckets.begin(),
                                        search_space_with_buckets.end(),
                                        middle_node_id,
                                        NodeBucket::ColumnCompare(column_idx));

    BOOST_ASSERT_MSG(std::distance(bucket_list.first, bucket_list.second) == 1,
                     "The pointers are not pointing to the same element.");

    NodeID current_node_id = middle_node_id;

    while (bucket_list.first->parent_node != current_node_id &&
           bucket_list.first != search_space_with_buckets.end())
    {
        const auto parent_node_id = bucket_list.first->parent_node;

        const auto from = DIRECTION == FORWARD_DIRECTION ? current_node_id : parent_node_id;
        const auto to = DIRECTION == FORWARD_DIRECTION ? parent_node_id : current_node_id;
        path.emplace_back(std::make_tuple(from, to, bucket_list.first->from_clique_arc));

        current_node_id = parent_node_id;
        bucket_list = std::equal_range(search_space_with_buckets.begin(),
                                       search_space_with_buckets.end(),
                                       current_node_id,
                                       NodeBucket::ColumnCompare(column_idx));

        BOOST_ASSERT_MSG(std::distance(bucket_list.first, bucket_list.second) == 1,
                         "The pointers are not pointing to the same element.");
    }
}

template <bool DIRECTION>
void calculateDistances(typename SearchEngineData<mld::Algorithm>::ManyToManyQueryHeap &query_heap,
                        const DataFacade<mld::Algorithm> &facade,
                        const std::vector<PhantomNode> &phantom_nodes,
                        const std::vector<std::size_t> &target_indices,
                        const unsigned row_idx,
                        const std::size_t source_index,
                        const unsigned number_of_sources,
                        const unsigned number_of_targets,
                        const std::vector<NodeBucket> &search_space_with_buckets,
                        std::vector<EdgeDistance> &distances_table,
                        const std::vector<NodeID> &middle_nodes_table,
                        SearchEngineData<mld::Algorithm> &engine_working_data)
{
    engine_working_data.InitializeOrClearFirstThreadLocalStorage(facade.GetNumberOfNodes(),
                                                                 facade.GetMaxBorderNodeID() + 1);

    for (unsigned column_idx = 0; column_idx < number_of_targets; ++column_idx)
    {
        // Step 1: Get source and target phantom nodes that were used in the bucketed search
        auto source_phantom_index = source_index;
        auto target_phantom_index = target_indices[column_idx];
        const auto &source_phantom = phantom_nodes[source_phantom_index];
        const auto &target_phantom = phantom_nodes[target_phantom_index];

        const auto location = DIRECTION == FORWARD_DIRECTION
                                  ? row_idx * number_of_targets + column_idx
                                  : row_idx + column_idx * number_of_sources;

        if (source_phantom_index == target_phantom_index)
        {
            distances_table[location] = 0.0;
            continue;
        }

        NodeID middle_node_id = middle_nodes_table[location];

        if (middle_node_id == SPECIAL_NODEID) // takes care of one-ways
        {
            distances_table[location] = INVALID_EDGE_DISTANCE;
            continue;
        }

        // Step 2: Find path from source to middle node
        PackedPath packed_path =
            mld::retrievePackedPathFromSingleManyToManyHeap<DIRECTION>(query_heap, middle_node_id);

        if (DIRECTION == FORWARD_DIRECTION)
        {
            std::reverse(packed_path.begin(), packed_path.end());
        }

        auto &forward_heap = *engine_working_data.forward_heap_1;
        auto &reverse_heap = *engine_working_data.reverse_heap_1;
        EdgeWeight weight = INVALID_EDGE_WEIGHT;
        std::vector<NodeID> unpacked_nodes_from_source;
        std::vector<EdgeID> unpacked_edges;
        std::tie(weight, unpacked_nodes_from_source, unpacked_edges) =
            unpackPathAndCalculateDistance(engine_working_data,
                                           facade,
                                           forward_heap,
                                           reverse_heap,
                                           DO_NOT_FORCE_LOOPS,
                                           DO_NOT_FORCE_LOOPS,
                                           INVALID_EDGE_WEIGHT,
                                           packed_path,
                                           middle_node_id,
                                           source_phantom);

        // Step 3: Find path from middle to target node
        packed_path.clear();
        retrievePackedPathFromSearchSpace<DIRECTION>(
            middle_node_id, column_idx, search_space_with_buckets, packed_path);

        if (DIRECTION == REVERSE_DIRECTION)
        {
            std::reverse(packed_path.begin(), packed_path.end());
        }

        std::vector<NodeID> unpacked_nodes_to_target;
        std::tie(weight, unpacked_nodes_to_target, unpacked_edges) =
            unpackPathAndCalculateDistance(engine_working_data,
                                           facade,
                                           forward_heap,
                                           reverse_heap,
                                           DO_NOT_FORCE_LOOPS,
                                           DO_NOT_FORCE_LOOPS,
                                           INVALID_EDGE_WEIGHT,
                                           packed_path,
                                           middle_node_id,
                                           target_phantom);

        if (DIRECTION == REVERSE_DIRECTION)
        {
            std::swap(unpacked_nodes_to_target, unpacked_nodes_from_source);
        }

        // Step 4: Compute annotation value along the path nodes without the target node
        auto annotation = 0.0;

        for (auto node = unpacked_nodes_from_source.begin(),
                  last_node = std::prev(unpacked_nodes_from_source.end());
             node != last_node;
             ++node)
        {
            annotation += computeEdgeDistance(facade, *node);
        }

        for (auto node = unpacked_nodes_to_target.begin(),
                  last_node = std::prev(unpacked_nodes_to_target.end());
             node != last_node;
             ++node)
        {
            annotation += computeEdgeDistance(facade, *node);
        }

        // Step 5: Get phantom node offsets and compute the annotation value
        EdgeDistance source_offset = 0., target_offset = 0.;
        {
            // ⚠ for REVERSE_DIRECTION original source and target phantom nodes are swapped
            if (DIRECTION == REVERSE_DIRECTION)
            {
                std::swap(source_phantom_index, target_phantom_index);
            }
            const auto &source_phantom = phantom_nodes[source_phantom_index];
            const auto &target_phantom = phantom_nodes[target_phantom_index];

            NodeID source_node = unpacked_nodes_from_source.front();
            NodeID target_node = unpacked_nodes_to_target.back();

            if (source_phantom.IsValidForwardSource() &&
                source_phantom.forward_segment_id.id == source_node)
            {
                //       ............       <-- calculateEGBAnnotation returns distance from 0
                //       to 3
                //       -->s               <-- subtract offset to start at source
                //          .........       <-- want this distance as result
                // entry 0---1---2---3---   <-- 3 is exit node
                source_offset = source_phantom.GetForwardDistance();
            }
            else if (source_phantom.IsValidReverseSource() &&
                     source_phantom.reverse_segment_id.id == source_node)
            {
                //       ............    <-- calculateEGBAnnotation returns distance from 0 to 3
                //          s<-------    <-- subtract offset to start at source
                //       ...             <-- want this distance
                // entry 0---1---2---3   <-- 3 is exit node
                source_offset = source_phantom.GetReverseDistance();
            }

            if (target_phantom.IsValidForwardTarget() &&
                target_phantom.forward_segment_id.id == target_node)
            {
                //       ............       <-- calculateEGBAnnotation returns distance from 0
                //       to 3
                //                   ++>t   <-- add offset to get to target
                //       ................   <-- want this distance as result
                // entry 0---1---2---3---   <-- 3 is exit node
                target_offset = target_phantom.GetForwardDistance();
            }
            else if (target_phantom.IsValidReverseTarget() &&
                     target_phantom.reverse_segment_id.id == target_node)
            {
                //       ............       <-- calculateEGBAnnotation returns distance from 0
                //       to 3
                //                   <++t   <-- add offset to get from target
                //       ................   <-- want this distance as result
                // entry 0---1---2---3---   <-- 3 is exit node
                target_offset = target_phantom.GetReverseDistance();
            }
        }

        distances_table[location] = -source_offset + annotation + target_offset;
    }
}

template <bool DIRECTION>
std::pair<std::vector<EdgeDuration>, std::vector<EdgeDistance>>
manyToManySearch(SearchEngineData<Algorithm> &engine_working_data,
                 const DataFacade<Algorithm> &facade,
                 const std::vector<PhantomNode> &phantom_nodes,
                 const std::vector<std::size_t> &source_indices,
                 const std::vector<std::size_t> &target_indices,
                 const bool calculate_distance)
{
    const auto number_of_sources = source_indices.size();
    const auto number_of_targets = target_indices.size();
    const auto number_of_entries = number_of_sources * number_of_targets;

    std::vector<EdgeWeight> weights_table(number_of_entries, INVALID_EDGE_WEIGHT);
    std::vector<EdgeDuration> durations_table(number_of_entries, MAXIMAL_EDGE_DURATION);
    std::vector<EdgeDistance> distances_table(number_of_entries, INVALID_EDGE_DISTANCE);
    std::vector<NodeID> middle_nodes_table(number_of_entries, SPECIAL_NODEID);

    std::vector<NodeBucket> search_space_with_buckets;

    // Populate buckets with paths from all accessible nodes to destinations via backward searches
    for (std::uint32_t column_idx = 0; column_idx < target_indices.size(); ++column_idx)
    {
        const auto index = target_indices[column_idx];
        const auto &target_phantom = phantom_nodes[index];

        engine_working_data.InitializeOrClearManyToManyThreadLocalStorage(
            facade.GetNumberOfNodes(), facade.GetMaxBorderNodeID() + 1);
        auto &query_heap = *(engine_working_data.many_to_many_heap);

        if (DIRECTION == FORWARD_DIRECTION)
            insertTargetInHeap(query_heap, target_phantom);
        else
            insertSourceInHeap(query_heap, target_phantom);

        // explore search space
        while (!query_heap.Empty())
        {
            backwardRoutingStep<DIRECTION>(
                facade, column_idx, query_heap, search_space_with_buckets, target_phantom);
        }
    }

    // Order lookup buckets
    std::sort(search_space_with_buckets.begin(), search_space_with_buckets.end());

    // Find shortest paths from sources to all accessible nodes
    for (std::uint32_t row_idx = 0; row_idx < source_indices.size(); ++row_idx)
    {
        const auto source_index = source_indices[row_idx];
        const auto &source_phantom = phantom_nodes[source_index];

        // Clear heap and insert source nodes
        engine_working_data.InitializeOrClearManyToManyThreadLocalStorage(
            facade.GetNumberOfNodes(), facade.GetMaxBorderNodeID() + 1);

        auto &query_heap = *(engine_working_data.many_to_many_heap);

        if (DIRECTION == FORWARD_DIRECTION)
            insertSourceInHeap(query_heap, source_phantom);
        else
            insertTargetInHeap(query_heap, source_phantom);

        // Explore search space
        while (!query_heap.Empty())
        {
            forwardRoutingStep<DIRECTION>(facade,
                                          row_idx,
                                          number_of_sources,
                                          number_of_targets,
                                          query_heap,
                                          search_space_with_buckets,
                                          weights_table,
                                          durations_table,
                                          distances_table,
                                          middle_nodes_table,
                                          source_phantom);
        }

        if (calculate_distance)
        {
            distances_table.resize(number_of_entries, INVALID_EDGE_DISTANCE);
            calculateDistances<DIRECTION>(query_heap,
                                          facade,
                                          phantom_nodes,
                                          target_indices, // source_indices
                                          row_idx,
                                          source_index,
                                          number_of_sources,
                                          number_of_targets,
                                          search_space_with_buckets,
                                          distances_table,
                                          middle_nodes_table,
                                          engine_working_data);
        }
    }

    return std::make_pair(durations_table, distances_table);
}

} // namespace mld

// Dispatcher function for one-to-many and many-to-one tasks that can be handled by MLD differently:
//
// * one-to-many (many-to-one) tasks use a unidirectional forward (backward) Dijkstra search
//   with the candidate node level `min(GetQueryLevel(phantom_node, node, phantom_nodes)`
//   for all destination (source) phantom nodes
//
// * many-to-many search tasks use a bidirectional Dijkstra search
//   with the candidate node level `min(GetHighestDifferentLevel(phantom_node, node))`
//   Due to pruned backward search space it is always better to compute the durations matrix
//   when number of sources is less than targets. If number of targets is less than sources
//   then search is performed on a reversed graph with phantom nodes with flipped roles and
//   returning a transposed matrix.
template <>
std::pair<std::vector<EdgeDuration>, std::vector<EdgeDistance>>
manyToManySearch(SearchEngineData<mld::Algorithm> &engine_working_data,
                 const DataFacade<mld::Algorithm> &facade,
                 const std::vector<PhantomNode> &phantom_nodes,
                 const std::vector<std::size_t> &source_indices,
                 const std::vector<std::size_t> &target_indices,
                 const bool calculate_distance,
                 const bool calculate_duration)
{
    (void)calculate_duration; // flag stub to use for calculating distances in matrix in mld in the
                              // future

    if (source_indices.size() == 1)
    { // TODO: check if target_indices.size() == 1 and do a bi-directional search
        return mld::oneToManySearch<FORWARD_DIRECTION>(engine_working_data,
                                                       facade,
                                                       phantom_nodes,
                                                       source_indices.front(),
                                                       target_indices,
                                                       calculate_distance);
    }

    if (target_indices.size() == 1)
    {
        return mld::oneToManySearch<REVERSE_DIRECTION>(engine_working_data,
                                                       facade,
                                                       phantom_nodes,
                                                       target_indices.front(),
                                                       source_indices,
                                                       calculate_distance);
    }

    if (target_indices.size() < source_indices.size())
    {
        return mld::manyToManySearch<REVERSE_DIRECTION>(engine_working_data,
                                                        facade,
                                                        phantom_nodes,
                                                        target_indices,
                                                        source_indices,
                                                        calculate_distance);
    }

    return mld::manyToManySearch<FORWARD_DIRECTION>(engine_working_data,
                                                    facade,
                                                    phantom_nodes,
                                                    source_indices,
                                                    target_indices,
                                                    calculate_distance);
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
