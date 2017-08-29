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
            else if (to_weight < query_heap.GetKey(to))
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

template <bool DIRECTION>
void relaxOutgoingEdges(const DataFacade<mld::Algorithm> &facade,
                        const NodeID node,
                        const EdgeWeight weight,
                        const EdgeDuration duration,
                        typename SearchEngineData<mld::Algorithm>::ManyToManyQueryHeap &query_heap,
                        const PhantomNode &phantom_node)
{
    BOOST_ASSERT(!facade.ExcludeNode(node));

    const auto &partition = facade.GetMultiLevelPartition();
    const auto &cells = facade.GetCellStorage();
    const auto &metric = facade.GetCellMetric();

    auto highest_diffrent_level = [&partition, node](const SegmentID &phantom_node) {
        if (phantom_node.enabled)
            return partition.GetHighestDifferentLevel(phantom_node.id, node);
        return INVALID_LEVEL_ID;
    };
    const auto level = std::min(highest_diffrent_level(phantom_node.forward_segment_id),
                                highest_diffrent_level(phantom_node.reverse_segment_id));

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
                    else if (to_weight < query_heap.GetKey(to))
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
                    else if (to_weight < query_heap.GetKey(to))
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
            else if (to_weight < query_heap.GetKey(to))
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
        else if (new_weight < current_weight)
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
std::vector<EdgeWeight> manyToManySearch(SearchEngineData<Algorithm> &engine_working_data,
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
        [&](const auto &chunk) {
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
                      [&](const auto &chunk) {
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

template std::vector<EdgeWeight>
manyToManySearch(SearchEngineData<ch::Algorithm> &engine_working_data,
                 const DataFacade<ch::Algorithm> &facade,
                 const std::vector<PhantomNode> &phantom_nodes,
                 std::vector<std::size_t> source_indices,
                 std::vector<std::size_t> target_indices);

template std::vector<EdgeWeight>
manyToManySearch(SearchEngineData<mld::Algorithm> &engine_working_data,
                 const DataFacade<mld::Algorithm> &facade,
                 const std::vector<PhantomNode> &phantom_nodes,
                 std::vector<std::size_t> source_indices,
                 std::vector<std::size_t> target_indices);

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
