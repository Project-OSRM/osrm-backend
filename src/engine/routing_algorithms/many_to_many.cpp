#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/routing_algorithms/routing_base_ch.hpp"

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

namespace
{
struct NodeBucket
{
    NodeID middle_node;
    unsigned column_index; // a column in the weight/duration matrix
    EdgeWeight weight;
    EdgeDuration duration;

    NodeBucket(NodeID middle_node, unsigned column_index, EdgeWeight weight, EdgeDuration duration)
        : middle_node(middle_node), column_index(column_index), weight(weight), duration(duration)
    {
    }

    // partial order comparison
    bool operator<(const NodeBucket &rhs) const { return middle_node < rhs.middle_node; }

    // functor for equal_range
    struct Compare
    {
        bool operator()(const NodeBucket &lhs, const NodeID &rhs) const
        {
            return lhs.middle_node < rhs;
        }

        bool operator()(const NodeID &lhs, const NodeBucket &rhs) const
        {
            return lhs < rhs.middle_node;
        }
    };
};

inline bool addLoopWeight(const DataFacade<ch::Algorithm> &facade,
                          const NodeID node,
                          EdgeWeight &weight,
                          EdgeDuration &duration)
{ // Special case for CH when contractor creates a loop edge node->node
    BOOST_ASSERT(weight < 0);

    const auto loop_weight = ch::getLoopWeight<false>(facade, node);
    if (loop_weight != INVALID_EDGE_WEIGHT)
    {
        const auto new_weight_with_loop = weight + loop_weight;
        if (new_weight_with_loop >= 0)
        {
            weight = new_weight_with_loop;
            duration += ch::getLoopWeight<true>(facade, node);
            return true;
        }
    }

    // No loop found or adjusted weight is negative
    return false;
}

template <bool DIRECTION>
void relaxOutgoingEdges(const DataFacade<ch::Algorithm> &facade,
                        const NodeID node,
                        const EdgeWeight weight,
                        const EdgeDuration duration,
                        typename SearchEngineData<ch::Algorithm>::ManyToManyQueryHeap &query_heap,
                        const PhantomNode &)
{
    if (ch::stallAtNode<DIRECTION>(facade, node, weight, query_heap))
    {
        return;
    }

    for (auto edge : facade.GetAdjacentEdgeRange(node))
    {
        const auto &data = facade.GetEdgeData(edge);
        if (DIRECTION == FORWARD_DIRECTION ? data.forward : data.backward)
        {
            const NodeID to = facade.GetTarget(edge);

            const auto edge_weight = data.weight;
            const auto edge_duration = data.duration;

            BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
            const auto to_weight = weight + edge_weight;
            const auto to_duration = duration + edge_duration;

            // New Node discovered -> Add to Heap + Node Info Storage
            if (!query_heap.WasInserted(to))
            {
                query_heap.Insert(to, to_weight, {node, to_duration});
            }
            // Found a shorter Path -> Update weight
            else if (std::tie(to_weight, to_duration) <
                     std::tie(query_heap.GetKey(to), query_heap.GetData(to).duration))
            {
                // new parent
                query_heap.GetData(to) = {node, to_duration};
                query_heap.DecreaseKey(to, to_weight);
            }
        }
    }
}

inline bool
addLoopWeight(const DataFacade<mld::Algorithm> &, const NodeID, EdgeWeight &, EdgeDuration &)
{ // MLD overlay does not introduce loop edges
    return false;
}

template <typename MultiLevelPartition>
inline LevelID getNodeQueryLevel(const MultiLevelPartition &partition,
                                 NodeID node,
                                 const PhantomNode &phantom_node)
{
    auto highest_diffrent_level = [&partition, node](const SegmentID &phantom_node) {
        if (phantom_node.enabled)
            return partition.GetHighestDifferentLevel(phantom_node.id, node);
        return INVALID_LEVEL_ID;
    };

    return std::min(highest_diffrent_level(phantom_node.forward_segment_id),
                    highest_diffrent_level(phantom_node.reverse_segment_id));
}

template <typename MultiLevelPartition>
inline LevelID getNodeQueryLevel(const MultiLevelPartition &partition,
                                 NodeID node,
                                 const std::vector<PhantomNode> &phantom_nodes,
                                 const std::size_t phantom_index,
                                 const std::vector<std::size_t> &phantom_indices)
{
    auto level = [&partition, node](const SegmentID &source, const SegmentID &target) {
        if (source.enabled && target.enabled)
            return partition.GetQueryLevel(source.id, target.id, node);
        return INVALID_LEVEL_ID;
    };

    const auto &source_phantom = phantom_nodes[phantom_index];

    auto result = INVALID_LEVEL_ID;
    for (const auto &index : phantom_indices)
    {
        const auto &target_phantom = phantom_nodes[index];
        auto min_level = std::min(
            std::min(level(source_phantom.forward_segment_id, target_phantom.forward_segment_id),
                     level(source_phantom.forward_segment_id, target_phantom.reverse_segment_id)),
            std::min(level(source_phantom.reverse_segment_id, target_phantom.forward_segment_id),
                     level(source_phantom.reverse_segment_id, target_phantom.reverse_segment_id)));
        result = std::min(result, min_level);
    }
    return result;
}

template <bool DIRECTION, typename... Args>
void relaxOutgoingEdges(const DataFacade<mld::Algorithm> &facade,
                        const NodeID node,
                        const EdgeWeight weight,
                        const EdgeDuration duration,
                        typename SearchEngineData<mld::Algorithm>::ManyToManyQueryHeap &query_heap,
                        Args... args)
{
    BOOST_ASSERT(!facade.ExcludeNode(node));

    const auto &partition = facade.GetMultiLevelPartition();
    const auto &cells = facade.GetCellStorage();
    const auto &metric = facade.GetCellMetric();

    const auto level = getNodeQueryLevel(partition, node, args...);

    const auto &node_data = query_heap.GetData(node);

    if (level >= 1 && !node_data.from_clique_arc)
    {
        const auto &cell = cells.GetCell(metric, level, partition.GetCell(level, node));
        if (DIRECTION == FORWARD_DIRECTION)
        { // Shortcuts in forward direction
            auto destination = cell.GetDestinationNodes().begin();
            auto shortcut_durations = cell.GetOutDuration(node);
            for (auto shortcut_weight : cell.GetOutWeight(node))
            {
                BOOST_ASSERT(destination != cell.GetDestinationNodes().end());
                BOOST_ASSERT(!shortcut_durations.empty());
                const NodeID to = *destination;

                if (shortcut_weight != INVALID_EDGE_WEIGHT && node != to)
                {
                    const auto to_weight = weight + shortcut_weight;
                    const auto to_duration = duration + shortcut_durations.front();
                    if (!query_heap.WasInserted(to))
                    {
                        query_heap.Insert(to, to_weight, {node, true, to_duration});
                    }
                    else if (std::tie(to_weight, to_duration) <
                             std::tie(query_heap.GetKey(to), query_heap.GetData(to).duration))
                    {
                        query_heap.GetData(to) = {node, true, to_duration};
                        query_heap.DecreaseKey(to, to_weight);
                    }
                }
                ++destination;
                shortcut_durations.advance_begin(1);
            }
            BOOST_ASSERT(shortcut_durations.empty());
        }
        else
        { // Shortcuts in backward direction
            auto source = cell.GetSourceNodes().begin();
            auto shortcut_durations = cell.GetInDuration(node);
            for (auto shortcut_weight : cell.GetInWeight(node))
            {
                BOOST_ASSERT(source != cell.GetSourceNodes().end());
                BOOST_ASSERT(!shortcut_durations.empty());
                const NodeID to = *source;

                if (shortcut_weight != INVALID_EDGE_WEIGHT && node != to)
                {
                    const auto to_weight = weight + shortcut_weight;
                    const auto to_duration = duration + shortcut_durations.front();
                    if (!query_heap.WasInserted(to))
                    {
                        query_heap.Insert(to, to_weight, {node, true, to_duration});
                    }
                    else if (std::tie(to_weight, to_duration) <
                             std::tie(query_heap.GetKey(to), query_heap.GetData(to).duration))
                    {
                        query_heap.GetData(to) = {node, true, to_duration};
                        query_heap.DecreaseKey(to, to_weight);
                    }
                }
                ++source;
                shortcut_durations.advance_begin(1);
            }
            BOOST_ASSERT(shortcut_durations.empty());
        }
    }

    for (const auto edge : facade.GetBorderEdgeRange(level, node))
    {
        const auto &data = facade.GetEdgeData(edge);
        if (DIRECTION == FORWARD_DIRECTION ? data.forward : data.backward)
        {
            const NodeID to = facade.GetTarget(edge);
            if (facade.ExcludeNode(to))
            {
                continue;
            }

            const auto edge_weight = data.weight;
            const auto edge_duration = data.duration;

            BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
            const auto to_weight = weight + edge_weight;
            const auto to_duration = duration + edge_duration;

            // New Node discovered -> Add to Heap + Node Info Storage
            if (!query_heap.WasInserted(to))
            {
                query_heap.Insert(to, to_weight, {node, false, to_duration});
            }
            // Found a shorter Path -> Update weight
            else if (std::tie(to_weight, to_duration) <
                     std::tie(query_heap.GetKey(to), query_heap.GetData(to).duration))
            {
                // new parent
                query_heap.GetData(to) = {node, false, to_duration};
                query_heap.DecreaseKey(to, to_weight);
            }
        }
    }
}

template <typename Algorithm>
void forwardRoutingStep(const DataFacade<Algorithm> &facade,
                        const unsigned row_idx,
                        const unsigned number_of_targets,
                        typename SearchEngineData<Algorithm>::ManyToManyQueryHeap &query_heap,
                        const std::vector<NodeBucket> &search_space_with_buckets,
                        std::vector<EdgeWeight> &weights_table,
                        std::vector<EdgeDuration> &durations_table,
                        const PhantomNode &phantom_node)
{
    const auto node = query_heap.DeleteMin();
    const auto source_weight = query_heap.GetKey(node);
    const auto source_duration = query_heap.GetData(node).duration;

    // check if each encountered node has an entry
    const auto &bucket_list = std::equal_range(search_space_with_buckets.begin(),
                                               search_space_with_buckets.end(),
                                               node,
                                               NodeBucket::Compare());
    for (const auto &current_bucket : boost::make_iterator_range(bucket_list))
    {
        // get target id from bucket entry
        const auto column_idx = current_bucket.column_index;
        const auto target_weight = current_bucket.weight;
        const auto target_duration = current_bucket.duration;

        auto &current_weight = weights_table[row_idx * number_of_targets + column_idx];
        auto &current_duration = durations_table[row_idx * number_of_targets + column_idx];

        // check if new weight is better
        auto new_weight = source_weight + target_weight;
        auto new_duration = source_duration + target_duration;

        if (new_weight < 0)
        {
            if (addLoopWeight(facade, node, new_weight, new_duration))
            {
                current_weight = std::min(current_weight, new_weight);
                current_duration = std::min(current_duration, new_duration);
            }
        }
        else if (std::tie(new_weight, new_duration) < std::tie(current_weight, current_duration))
        {
            current_weight = new_weight;
            current_duration = new_duration;
        }
    }

    relaxOutgoingEdges<FORWARD_DIRECTION>(
        facade, node, source_weight, source_duration, query_heap, phantom_node);
}

template <typename Algorithm>
void backwardRoutingStep(const DataFacade<Algorithm> &facade,
                         const unsigned column_idx,
                         typename SearchEngineData<Algorithm>::ManyToManyQueryHeap &query_heap,
                         std::vector<NodeBucket> &search_space_with_buckets,
                         const PhantomNode &phantom_node)
{
    const auto node = query_heap.DeleteMin();
    const auto target_weight = query_heap.GetKey(node);
    const auto target_duration = query_heap.GetData(node).duration;

    // store settled nodes in search space bucket
    search_space_with_buckets.emplace_back(node, column_idx, target_weight, target_duration);

    relaxOutgoingEdges<REVERSE_DIRECTION>(
        facade, node, target_weight, target_duration, query_heap, phantom_node);
}
}

template <typename Algorithm>
std::vector<EdgeDuration> manyToManySearch(SearchEngineData<Algorithm> &engine_working_data,
                                           const DataFacade<Algorithm> &facade,
                                           const std::vector<PhantomNode> &phantom_nodes,
                                           std::vector<std::size_t> source_indices,
                                           std::vector<std::size_t> target_indices)
{
    if (source_indices.empty())
    {
        source_indices.resize(phantom_nodes.size());
        std::iota(source_indices.begin(), source_indices.end(), 0);
    }
    if (target_indices.empty())
    {
        target_indices.resize(phantom_nodes.size());
        std::iota(target_indices.begin(), target_indices.end(), 0);
    }

    const auto number_of_sources = source_indices.size();
    const auto number_of_targets = target_indices.size();
    const auto number_of_entries = number_of_sources * number_of_targets;

    std::vector<EdgeWeight> weights_table(number_of_entries, INVALID_EDGE_WEIGHT);
    std::vector<EdgeDuration> durations_table(number_of_entries, MAXIMAL_EDGE_DURATION);

    std::mutex lock;
    std::vector<NodeBucket> search_space_with_buckets;

    // Backward search for target phantoms
    tbb::parallel_for(
        tbb::blocked_range<std::size_t>{0, target_indices.size()},
        [&](const tbb::blocked_range<std::size_t> &chunk) {
            for (auto column_idx = chunk.begin(), end = chunk.end(); column_idx != end;
                 ++column_idx)
            {
                const auto index = target_indices[column_idx];
                const auto &phantom = phantom_nodes[index];

                engine_working_data.InitializeOrClearManyToManyThreadLocalStorage(
                    facade.GetNumberOfNodes());
                auto &query_heap = *(engine_working_data.many_to_many_heap);
                insertTargetInHeap(query_heap, phantom);

                // explore search space
                std::vector<NodeBucket> local_buckets;
                while (!query_heap.Empty())
                {
                    backwardRoutingStep(facade, column_idx, query_heap, local_buckets, phantom);
                }

                { // Insert local buckets into the global search space
                    std::lock_guard<std::mutex> guard{lock};
                    search_space_with_buckets.insert(std::end(search_space_with_buckets),
                                                     std::begin(local_buckets),
                                                     std::end(local_buckets));
                }
            }
        });

    tbb::parallel_sort(search_space_with_buckets.begin(), search_space_with_buckets.end());

    // For each source do forward search
    tbb::parallel_for(tbb::blocked_range<std::size_t>{0, source_indices.size()},
                      [&](const tbb::blocked_range<std::size_t> &chunk) {
                          for (auto row_idx = chunk.begin(), end = chunk.end(); row_idx != end;
                               ++row_idx)
                          {
                              const auto index = source_indices[row_idx];
                              const auto &phantom = phantom_nodes[index];

                              // clear heap and insert source nodes
                              engine_working_data.InitializeOrClearManyToManyThreadLocalStorage(
                                  facade.GetNumberOfNodes());
                              auto &query_heap = *(engine_working_data.many_to_many_heap);
                              insertSourceInHeap(query_heap, phantom);

                              // explore search space
                              while (!query_heap.Empty())
                              {
                                  forwardRoutingStep(facade,
                                                     row_idx,
                                                     number_of_targets,
                                                     query_heap,
                                                     search_space_with_buckets,
                                                     weights_table,
                                                     durations_table,
                                                     phantom);
                              }
                          }
                      });

    return durations_table;
}

template std::vector<EdgeDuration>
manyToManySearch(SearchEngineData<ch::Algorithm> &engine_working_data,
                 const DataFacade<ch::Algorithm> &facade,
                 const std::vector<PhantomNode> &phantom_nodes,
                 std::vector<std::size_t> source_indices,
                 std::vector<std::size_t> target_indices);

template std::vector<EdgeDuration>
manyToManySearch(SearchEngineData<mld::Algorithm> &engine_working_data,
                 const DataFacade<mld::Algorithm> &facade,
                 const std::vector<PhantomNode> &phantom_nodes,
                 std::vector<std::size_t> source_indices,
                 std::vector<std::size_t> target_indices);

namespace mld
{

// Unidirectional multi-layer Dijkstra search for 1-to-N and N-to-1 matrices
template <bool DIRECTION>
std::vector<EdgeDuration> oneToManySearch(SearchEngineData<Algorithm> &engine_working_data,
                                          const DataFacade<Algorithm> &facade,
                                          const std::vector<PhantomNode> &phantom_nodes,
                                          std::size_t phantom_index,
                                          std::vector<std::size_t> phantom_indices)
{
    if (phantom_indices.empty())
    {
        phantom_indices.resize(phantom_nodes.size());
        std::iota(phantom_indices.begin(), phantom_indices.end(), 0);
    }

    std::vector<EdgeWeight> weights(phantom_indices.size(), INVALID_EDGE_WEIGHT);
    std::vector<EdgeDuration> durations(phantom_indices.size(), MAXIMAL_EDGE_DURATION);

    // Collect destination (source) nodes into a map
    std::unordered_multimap<NodeID, std::tuple<std::size_t, EdgeWeight, EdgeDuration>>
        target_nodes_index;
    target_nodes_index.reserve(phantom_indices.size());
    for (std::size_t index = 0; index < phantom_indices.size(); ++index)
    {
        const auto &phantom_index = phantom_indices[index];
        const auto &phantom_node = phantom_nodes[phantom_index];

        if (DIRECTION == FORWARD_DIRECTION)
        {
            if (phantom_node.IsValidForwardTarget())
                target_nodes_index.insert({phantom_node.forward_segment_id.id,
                                           {index,
                                            phantom_node.GetForwardWeightPlusOffset(),
                                            phantom_node.GetForwardDuration()}});
            if (phantom_node.IsValidReverseTarget())
                target_nodes_index.insert({phantom_node.reverse_segment_id.id,
                                           {index,
                                            phantom_node.GetReverseWeightPlusOffset(),
                                            phantom_node.GetReverseDuration()}});
        }
        else if (DIRECTION == REVERSE_DIRECTION)
        {
            if (phantom_node.IsValidForwardSource())
                target_nodes_index.insert({phantom_node.forward_segment_id.id,
                                           {index,
                                            -phantom_node.GetForwardWeightPlusOffset(),
                                            -phantom_node.GetForwardDuration()}});
            if (phantom_node.IsValidReverseSource())
                target_nodes_index.insert({phantom_node.reverse_segment_id.id,
                                           {index,
                                            -phantom_node.GetReverseWeightPlusOffset(),
                                            -phantom_node.GetReverseDuration()}});
        }
    }

    // Initialize query heap
    engine_working_data.InitializeOrClearManyToManyThreadLocalStorage(facade.GetNumberOfNodes());
    auto &query_heap = *(engine_working_data.many_to_many_heap);

    // Check if node is in the destinations list and update weights/durations
    auto update_values = [&](NodeID node, EdgeWeight weight, EdgeDuration duration) {
        auto candidates = target_nodes_index.equal_range(node);
        for (auto it = candidates.first; it != candidates.second;)
        {
            std::size_t index;
            EdgeWeight target_weight;
            EdgeDuration target_duration;
            std::tie(index, target_weight, target_duration) = it->second;

            const auto path_weight = weight + target_weight;
            if (path_weight >= 0)
            {
                const auto path_duration = duration + target_duration;

                if (std::tie(path_weight, path_duration) <
                    std::tie(weights[index], durations[index]))
                {
                    weights[index] = path_weight;
                    durations[index] = path_duration;
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

    // Check a single path result and insert adjacent nodes into heap
    auto insert_node = [&](NodeID node, EdgeWeight initial_weight, EdgeDuration initial_duration) {

        // Update single node paths
        update_values(node, initial_weight, initial_duration);

        // Place adjacent nodes into heap
        for (auto edge : facade.GetAdjacentEdgeRange(node))
        {
            const auto &data = facade.GetEdgeData(edge);
            if (DIRECTION == FORWARD_DIRECTION ? data.forward : data.backward)
            {
                query_heap.Insert(facade.GetTarget(edge),
                                  data.weight + initial_weight,
                                  {node, data.duration + initial_duration});
            }
        }
    };

    { // Place source (destination) adjacent nodes into the heap
        const auto &phantom_node = phantom_nodes[phantom_index];

        if (DIRECTION == FORWARD_DIRECTION)
        {

            if (phantom_node.IsValidForwardSource())
                insert_node(phantom_node.forward_segment_id.id,
                            -phantom_node.GetForwardWeightPlusOffset(),
                            -phantom_node.GetForwardDuration());

            if (phantom_node.IsValidReverseSource())
                insert_node(phantom_node.reverse_segment_id.id,
                            -phantom_node.GetReverseWeightPlusOffset(),
                            -phantom_node.GetReverseDuration());
        }
        else if (DIRECTION == REVERSE_DIRECTION)
        {
            if (phantom_node.IsValidForwardTarget())
                insert_node(phantom_node.forward_segment_id.id,
                            phantom_node.GetForwardWeightPlusOffset(),
                            phantom_node.GetForwardDuration());

            if (phantom_node.IsValidReverseTarget())
                insert_node(phantom_node.reverse_segment_id.id,
                            phantom_node.GetReverseWeightPlusOffset(),
                            phantom_node.GetReverseDuration());
        }
    }

    while (!query_heap.Empty() && !target_nodes_index.empty())
    {
        // Extract node from the heap
        const auto node = query_heap.DeleteMin();
        const auto weight = query_heap.GetKey(node);
        const auto duration = query_heap.GetData(node).duration;

        // Update values
        update_values(node, weight, duration);

        // Relax outgoing edges
        relaxOutgoingEdges<DIRECTION>(facade,
                                      node,
                                      weight,
                                      duration,
                                      query_heap,
                                      phantom_nodes,
                                      phantom_index,
                                      phantom_indices);
    }

    return durations;
}

template std::vector<EdgeDuration>
oneToManySearch<FORWARD_DIRECTION>(SearchEngineData<Algorithm> &engine_working_data,
                                   const DataFacade<Algorithm> &facade,
                                   const std::vector<PhantomNode> &phantom_nodes,
                                   std::size_t phantom_index,
                                   std::vector<std::size_t> phantom_indices);

template std::vector<EdgeDuration>
oneToManySearch<REVERSE_DIRECTION>(SearchEngineData<Algorithm> &engine_working_data,
                                   const DataFacade<Algorithm> &facade,
                                   const std::vector<PhantomNode> &phantom_nodes,
                                   std::size_t phantom_index,
                                   std::vector<std::size_t> phantom_indices);
} // mld

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
