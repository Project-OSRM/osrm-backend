#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/routing_algorithms/routing_base_ch.hpp"

#include <boost/assert.hpp>

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

using ManyToManyQueryHeap = SearchEngineData::ManyToManyQueryHeap;

namespace
{
struct NodeBucket
{
    unsigned target_id; // essentially a row in the weight matrix
    EdgeWeight weight;
    RoutingPayload payload;
    NodeBucket(const unsigned target_id, const EdgeWeight weight, const RoutingPayload &payload)
        : target_id(target_id), weight(weight), payload(payload)
    {
    }
};

// FIXME This should be replaced by an std::unordered_multimap, though this needs benchmarking
using SearchSpaceWithBuckets = std::unordered_map<NodeID, std::vector<NodeBucket>>;

template <bool DIRECTION>
void relaxOutgoingEdges(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
                        const NodeID node,
                        const EdgeWeight weight,
                        const RoutingPayload &payload,
                        ManyToManyQueryHeap &query_heap)
{
    for (auto edge : facade.GetAdjacentEdgeRange(node))
    {
        const auto &data = facade.GetEdgeData(edge);
        if (DIRECTION == FORWARD_DIRECTION ? data.forward : data.backward)
        {
            const NodeID to = facade.GetTarget(edge);
            const EdgeWeight edge_weight = data.weight;
            const RoutingPayload &edge_payload = data.payload;

            BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
            const EdgeWeight to_weight = weight + edge_weight;
            const RoutingPayload to_payload = payload + edge_payload;

            // New Node discovered -> Add to Heap + Node Info Storage
            if (!query_heap.WasInserted(to))
            {
                query_heap.Insert(to, to_weight, {node, to_payload});
            }
            // Found a shorter Path -> Update weight
            else if (to_weight < query_heap.GetKey(to))
            {
                // new parent
                query_heap.GetData(to) = {node, to_payload};
                query_heap.DecreaseKey(to, to_weight);
            }
        }
    }
}

void forwardRoutingStep(const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
                        const unsigned row_idx,
                        const unsigned number_of_targets,
                        ManyToManyQueryHeap &query_heap,
                        const SearchSpaceWithBuckets &search_space_with_buckets,
                        std::vector<EdgeWeight> &weights_table,
                        std::vector<RoutingPayload> &payload_table)
{
    const NodeID node = query_heap.DeleteMin();
    const EdgeWeight source_weight = query_heap.GetKey(node);
    const RoutingPayload source_payload = query_heap.GetData(node).payload;

    // check if each encountered node has an entry
    const auto bucket_iterator = search_space_with_buckets.find(node);
    // iterate bucket if there exists one
    if (bucket_iterator != search_space_with_buckets.end())
    {
        const std::vector<NodeBucket> &bucket_list = bucket_iterator->second;
        for (const NodeBucket &current_bucket : bucket_list)
        {
            // get target id from bucket entry
            const unsigned column_idx = current_bucket.target_id;
            const EdgeWeight target_weight = current_bucket.weight;
            const RoutingPayload &target_payload = current_bucket.payload;

            auto &current_weight = weights_table[row_idx * number_of_targets + column_idx];
            auto &current_payload = payload_table[row_idx * number_of_targets + column_idx];

            // check if new weight is better
            const EdgeWeight new_weight = source_weight + target_weight;
            if (new_weight < 0)
            {
                const EdgeWeight loop_weight = ch::getLoopWeight(facade, node);
                const EdgeWeight new_weight_with_loop = new_weight + loop_weight;
                if (loop_weight != INVALID_EDGE_WEIGHT && new_weight_with_loop >= 0)
                {
                    if (new_weight_with_loop < current_weight)
                    {
                        current_weight = new_weight_with_loop;
                        RoutingPayload payload = RoutingPayload(ch::getLoopPayload(facade, node));
                        current_payload = source_payload + target_payload + payload;
                    }
                }
            }
            else if (new_weight < current_weight)
            {
                current_weight = new_weight;
                current_payload = source_payload + target_payload;
            }
        }
    }
    if (ch::stallAtNode<FORWARD_DIRECTION>(facade, node, source_weight, query_heap))
    {
        return;
    }

    relaxOutgoingEdges<FORWARD_DIRECTION>(facade, node, source_weight, source_payload, query_heap);
}

void backwardRoutingStep(
    const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
    const unsigned column_idx,
    ManyToManyQueryHeap &query_heap,
    SearchSpaceWithBuckets &search_space_with_buckets)
{
    const NodeID node = query_heap.DeleteMin();
    const EdgeWeight target_weight = query_heap.GetKey(node);
    const RoutingPayload target_payload = query_heap.GetData(node).payload;

    // store settled nodes in search space bucket
    search_space_with_buckets[node].emplace_back(column_idx, target_weight, target_payload);

    if (ch::stallAtNode<REVERSE_DIRECTION>(facade, node, target_weight, query_heap))
    {
        return;
    }

    relaxOutgoingEdges<REVERSE_DIRECTION>(facade, node, target_weight, target_payload, query_heap);
}
}

std::vector<RoutingPayload>
manyToManySearch(SearchEngineData &engine_working_data,
                 const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
                 const std::vector<PhantomNode> &phantom_nodes,
                 const std::vector<std::size_t> &source_indices,
                 const std::vector<std::size_t> &target_indices)
{
    const auto number_of_sources =
        source_indices.empty() ? phantom_nodes.size() : source_indices.size();
    const auto number_of_targets =
        target_indices.empty() ? phantom_nodes.size() : target_indices.size();
    const auto number_of_entries = number_of_sources * number_of_targets;

    std::vector<EdgeWeight> weights_table(number_of_entries, INVALID_EDGE_WEIGHT);
    std::vector<RoutingPayload> payload_table(number_of_entries, INVALID_RPAYLOAD);

    engine_working_data.InitializeOrClearManyToManyThreadLocalStorage(facade.GetNumberOfNodes());

    auto &query_heap = *(engine_working_data.many_to_many_heap);

    SearchSpaceWithBuckets search_space_with_buckets;

    unsigned column_idx = 0;
    const auto search_target_phantom = [&](const PhantomNode &phantom) {
        // clear heap and insert target nodes
        query_heap.Clear();
        insertNodesInHeap<REVERSE_DIRECTION>(query_heap, phantom);

        // explore search space
        while (!query_heap.Empty())
        {
            backwardRoutingStep(facade, column_idx, query_heap, search_space_with_buckets);
        }
        ++column_idx;
    };

    // for each source do forward search
    unsigned row_idx = 0;
    const auto search_source_phantom = [&](const PhantomNode &phantom) {
        // clear heap and insert source nodes
        query_heap.Clear();
        insertNodesInHeap<FORWARD_DIRECTION>(query_heap, phantom);

        // explore search space
        while (!query_heap.Empty())
        {
            forwardRoutingStep(facade,
                               row_idx,
                               number_of_targets,
                               query_heap,
                               search_space_with_buckets,
                               weights_table,
                               payload_table);
        }
        ++row_idx;
    };

    if (target_indices.empty())
    {
        for (const auto &phantom : phantom_nodes)
        {
            search_target_phantom(phantom);
        }
    }
    else
    {
        for (const auto index : target_indices)
        {
            const auto &phantom = phantom_nodes[index];
            search_target_phantom(phantom);
        }
    }

    if (source_indices.empty())
    {
        for (const auto &phantom : phantom_nodes)
        {
            search_source_phantom(phantom);
        }
    }
    else
    {
        for (const auto index : source_indices)
        {
            const auto &phantom = phantom_nodes[index];
            search_source_phantom(phantom);
        }
    }

    return payload_table;
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
