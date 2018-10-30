#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/routing_algorithms/routing_base_ch.hpp"

#include <boost/assert.hpp>
#include <boost/range/iterator_range_core.hpp>

#include <limits>
#include <memory>
#include <vector>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

namespace ch
{

inline bool addLoopWeight(const DataFacade<ch::Algorithm> &facade,
                          const NodeID node,
                          EdgeWeight &weight,
                          EdgeDuration &duration,
                          EdgeDistance &distance)
{ // Special case for CH when contractor creates a loop edge node->node
    BOOST_ASSERT(weight < 0);

    const auto loop_weight = ch::getLoopWeight<false>(facade, node);
    if (std::get<0>(loop_weight) != INVALID_EDGE_WEIGHT)
    {
        const auto new_weight_with_loop = weight + std::get<0>(loop_weight);
        if (new_weight_with_loop >= 0)
        {
            weight = new_weight_with_loop;
            auto result = ch::getLoopWeight<true>(facade, node);
            duration += std::get<0>(result);
            distance += std::get<1>(result);
            return true;
        }
    }

    // No loop found or adjusted weight is negative
    return false;
}

template <bool DIRECTION>
void relaxOutgoingEdges(const DataFacade<Algorithm> &facade,
                        const NodeID node,
                        const EdgeWeight weight,
                        const EdgeDuration duration,
                        const EdgeDistance distance,
                        typename SearchEngineData<Algorithm>::ManyToManyQueryHeap &query_heap,
                        const PhantomNode &)
{
    if (stallAtNode<DIRECTION>(facade, node, weight, query_heap))
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
            const auto edge_distance = data.distance;

            BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
            const auto to_weight = weight + edge_weight;
            const auto to_duration = duration + edge_duration;
            const auto to_distance = distance + edge_distance;

            // New Node discovered -> Add to Heap + Node Info Storage
            if (!query_heap.WasInserted(to))
            {
                query_heap.Insert(to, to_weight, {node, to_duration, to_distance});
            }
            // Found a shorter Path -> Update weight and set new parent
            else if (std::tie(to_weight, to_duration) <
                     std::tie(query_heap.GetKey(to), query_heap.GetData(to).duration))
            {
                query_heap.GetData(to) = {node, to_duration, to_distance};
                query_heap.DecreaseKey(to, to_weight);
            }
        }
    }
}

void forwardRoutingStep(const DataFacade<Algorithm> &facade,
                        const std::size_t row_index,
                        const std::size_t number_of_targets,
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
        const auto column_index = current_bucket.column_index;
        const auto target_weight = current_bucket.weight;
        const auto target_duration = current_bucket.duration;
        const auto target_distance = current_bucket.distance;

        auto &current_weight = weights_table[row_index * number_of_targets + column_index];

        EdgeDistance nulldistance = 0;

        auto &current_duration = durations_table[row_index * number_of_targets + column_index];
        auto &current_distance =
            distances_table.empty() ? nulldistance
                                    : distances_table[row_index * number_of_targets + column_index];

        // Check if new weight is better
        auto new_weight = source_weight + target_weight;
        auto new_duration = source_duration + target_duration;
        auto new_distance = source_distance + target_distance;

        if (new_weight < 0)
        {
            if (addLoopWeight(facade, node, new_weight, new_duration, new_distance))
            {
                current_weight = std::min(current_weight, new_weight);
                current_duration = std::min(current_duration, new_duration);
                current_distance = std::min(current_distance, new_distance);
                middle_nodes_table[row_index * number_of_targets + column_index] = node;
            }
        }
        else if (std::tie(new_weight, new_duration) < std::tie(current_weight, current_duration))
        {
            current_weight = new_weight;
            current_duration = new_duration;
            current_distance = new_distance;
            middle_nodes_table[row_index * number_of_targets + column_index] = node;
        }
    }

    relaxOutgoingEdges<FORWARD_DIRECTION>(
        facade, node, source_weight, source_duration, source_distance, query_heap, phantom_node);
}

void backwardRoutingStep(const DataFacade<Algorithm> &facade,
                         const unsigned column_index,
                         typename SearchEngineData<Algorithm>::ManyToManyQueryHeap &query_heap,
                         std::vector<NodeBucket> &search_space_with_buckets,
                         const PhantomNode &phantom_node)
{
    const auto node = query_heap.DeleteMin();
    const auto target_weight = query_heap.GetKey(node);
    const auto target_duration = query_heap.GetData(node).duration;
    const auto target_distance = query_heap.GetData(node).distance;
    const auto parent = query_heap.GetData(node).parent;

    // Store settled nodes in search space bucket
    search_space_with_buckets.emplace_back(
        node, parent, column_index, target_weight, target_duration, target_distance);

    relaxOutgoingEdges<REVERSE_DIRECTION>(
        facade, node, target_weight, target_duration, target_distance, query_heap, phantom_node);
}

} // namespace ch

template <>
std::pair<std::vector<EdgeDuration>, std::vector<EdgeDistance>>
manyToManySearch(SearchEngineData<ch::Algorithm> &engine_working_data,
                 const DataFacade<ch::Algorithm> &facade,
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
    std::vector<EdgeDistance> distances_table(calculate_distance ? number_of_entries : 0,
                                              MAXIMAL_EDGE_DISTANCE);
    std::vector<NodeID> middle_nodes_table(number_of_entries, SPECIAL_NODEID);

    std::vector<NodeBucket> search_space_with_buckets;

    // Populate buckets with paths from all accessible nodes to destinations via backward searches
    for (std::uint32_t column_index = 0; column_index < target_indices.size(); ++column_index)
    {
        const auto index = target_indices[column_index];
        const auto &phantom = phantom_nodes[index];

        engine_working_data.InitializeOrClearManyToManyThreadLocalStorage(
            facade.GetNumberOfNodes());
        auto &query_heap = *(engine_working_data.many_to_many_heap);
        insertTargetInHeap(query_heap, phantom);

        // Explore search space
        while (!query_heap.Empty())
        {
            backwardRoutingStep(
                facade, column_index, query_heap, search_space_with_buckets, phantom);
        }
    }

    // Order lookup buckets
    std::sort(search_space_with_buckets.begin(), search_space_with_buckets.end());

    // Find shortest paths from sources to all accessible nodes
    for (std::uint32_t row_index = 0; row_index < source_indices.size(); ++row_index)
    {
        const auto source_index = source_indices[row_index];
        const auto &source_phantom = phantom_nodes[source_index];

        // Clear heap and insert source nodes
        engine_working_data.InitializeOrClearManyToManyThreadLocalStorage(
            facade.GetNumberOfNodes());
        auto &query_heap = *(engine_working_data.many_to_many_heap);
        insertSourceInHeap(query_heap, source_phantom);

        // Explore search space
        while (!query_heap.Empty())
        {
            forwardRoutingStep(facade,
                               row_index,
                               number_of_targets,
                               query_heap,
                               search_space_with_buckets,
                               weights_table,
                               durations_table,
                               distances_table,
                               middle_nodes_table,
                               source_phantom);
        }
    }

    return std::make_pair(durations_table, distances_table);
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
