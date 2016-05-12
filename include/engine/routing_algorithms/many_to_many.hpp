#ifndef MANY_TO_MANY_ROUTING_HPP
#define MANY_TO_MANY_ROUTING_HPP

#include "engine/routing_algorithms/routing_base.hpp"
#include "engine/search_engine_data.hpp"
#include "util/typedefs.hpp"

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

template <class DataFacadeT>
class ManyToManyRouting final
    : public BasicRoutingInterface<DataFacadeT, ManyToManyRouting<DataFacadeT>>
{
    using super = BasicRoutingInterface<DataFacadeT, ManyToManyRouting<DataFacadeT>>;
    using QueryHeap = SearchEngineData::QueryHeap;
    SearchEngineData &engine_working_data;

    struct NodeBucket
    {
        unsigned target_id; // essentially a row in the weight matrix
        EdgeWeight weight;
        NodeBucket(const unsigned target_id, const EdgeWeight weight)
            : target_id(target_id), weight(weight)
        {
        }
    };

    // FIXME This should be replaced by an std::unordered_multimap, though this needs benchmarking
    using SearchSpaceWithBuckets = std::unordered_map<NodeID, std::vector<NodeBucket>>;

  public:
    ManyToManyRouting(SearchEngineData &engine_working_data)
        : engine_working_data(engine_working_data)
    {
    }

    std::vector<EdgeWeight> operator()(const DataFacadeT &facade,
                                       const std::vector<PhantomNode> &phantom_nodes,
                                       const std::vector<std::size_t> &source_indices,
                                       const std::vector<std::size_t> &target_indices) const
    {
        const auto number_of_sources =
            source_indices.empty() ? phantom_nodes.size() : source_indices.size();
        const auto number_of_targets =
            target_indices.empty() ? phantom_nodes.size() : target_indices.size();
        const auto number_of_entries = number_of_sources * number_of_targets;
        std::vector<EdgeWeight> result_table(number_of_entries,
                                             std::numeric_limits<EdgeWeight>::max());

        engine_working_data.InitializeOrClearFirstThreadLocalStorage(facade.GetNumberOfNodes());

        QueryHeap &query_heap = *(engine_working_data.forward_heap_1);

        SearchSpaceWithBuckets search_space_with_buckets;

        unsigned column_idx = 0;
        const auto search_target_phantom = [&](const PhantomNode &phantom) {
            query_heap.Clear();
            // insert target(s) at weight 0

            if (phantom.forward_segment_id.enabled)
            {
                query_heap.Insert(phantom.forward_segment_id.id,
                                  phantom.GetForwardWeightPlusOffset(),
                                  phantom.forward_segment_id.id);
            }
            if (phantom.reverse_segment_id.enabled)
            {
                query_heap.Insert(phantom.reverse_segment_id.id,
                                  phantom.GetReverseWeightPlusOffset(),
                                  phantom.reverse_segment_id.id);
            }

            // explore search space
            while (!query_heap.Empty())
            {
                BackwardRoutingStep(facade, column_idx, query_heap, search_space_with_buckets);
            }
            ++column_idx;
        };

        // for each source do forward search
        unsigned row_idx = 0;
        const auto search_source_phantom = [&](const PhantomNode &phantom) {
            query_heap.Clear();
            // insert target(s) at weight 0

            if (phantom.forward_segment_id.enabled)
            {
                query_heap.Insert(phantom.forward_segment_id.id,
                                  -phantom.GetForwardWeightPlusOffset(),
                                  phantom.forward_segment_id.id);
            }
            if (phantom.reverse_segment_id.enabled)
            {
                query_heap.Insert(phantom.reverse_segment_id.id,
                                  -phantom.GetReverseWeightPlusOffset(),
                                  phantom.reverse_segment_id.id);
            }

            // explore search space
            while (!query_heap.Empty())
            {
                ForwardRoutingStep(facade,
                                   row_idx,
                                   number_of_targets,
                                   query_heap,
                                   search_space_with_buckets,
                                   result_table);
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

        return result_table;
    }

    void ForwardRoutingStep(const DataFacadeT &facade,
                            const unsigned row_idx,
                            const unsigned number_of_targets,
                            QueryHeap &query_heap,
                            const SearchSpaceWithBuckets &search_space_with_buckets,
                            std::vector<EdgeWeight> &result_table) const
    {
        const NodeID node = query_heap.DeleteMin();
        const int source_weight = query_heap.GetKey(node);

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
                const int target_weight = current_bucket.weight;
                auto &current_weight = result_table[row_idx * number_of_targets + column_idx];
                // check if new weight is better
                const EdgeWeight new_weight = source_weight + target_weight;
                if (new_weight < 0)
                {
                    const EdgeWeight loop_weight = super::GetLoopWeight(facade, node);
                    const int new_weight_with_loop = new_weight + loop_weight;
                    if (loop_weight != INVALID_EDGE_WEIGHT && new_weight_with_loop >= 0)
                    {
                        current_weight = std::min(current_weight, new_weight_with_loop);
                    }
                }
                else if (new_weight < current_weight)
                {
                    result_table[row_idx * number_of_targets + column_idx] = new_weight;
                }
            }
        }
        if (StallAtNode<true>(facade, node, source_weight, query_heap))
        {
            return;
        }
        RelaxOutgoingEdges<true>(facade, node, source_weight, query_heap);
    }

    void BackwardRoutingStep(const DataFacadeT &facade,
                             const unsigned column_idx,
                             QueryHeap &query_heap,
                             SearchSpaceWithBuckets &search_space_with_buckets) const
    {
        const NodeID node = query_heap.DeleteMin();
        const int target_weight = query_heap.GetKey(node);

        // store settled nodes in search space bucket
        search_space_with_buckets[node].emplace_back(column_idx, target_weight);

        if (StallAtNode<false>(facade, node, target_weight, query_heap))
        {
            return;
        }

        RelaxOutgoingEdges<false>(facade, node, target_weight, query_heap);
    }

    template <bool forward_direction>
    inline void RelaxOutgoingEdges(const DataFacadeT &facade,
                                   const NodeID node,
                                   const EdgeWeight weight,
                                   QueryHeap &query_heap) const
    {
        for (auto edge : facade.GetAdjacentEdgeRange(node))
        {
            const auto &data = facade.GetEdgeData(edge);
            const bool direction_flag = (forward_direction ? data.forward : data.backward);
            if (direction_flag)
            {
                const NodeID to = facade.GetTarget(edge);
                const int edge_weight = data.weight;

                BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
                const int to_weight = weight + edge_weight;

                // New Node discovered -> Add to Heap + Node Info Storage
                if (!query_heap.WasInserted(to))
                {
                    query_heap.Insert(to, to_weight, node);
                }
                // Found a shorter Path -> Update weight
                else if (to_weight < query_heap.GetKey(to))
                {
                    // new parent
                    query_heap.GetData(to).parent = node;
                    query_heap.DecreaseKey(to, to_weight);
                }
            }
        }
    }

    // Stalling
    template <bool forward_direction>
    inline bool StallAtNode(const DataFacadeT &facade,
                            const NodeID node,
                            const EdgeWeight weight,
                            QueryHeap &query_heap) const
    {
        for (auto edge : facade.GetAdjacentEdgeRange(node))
        {
            const auto &data = facade.GetEdgeData(edge);
            const bool reverse_flag = ((!forward_direction) ? data.forward : data.backward);
            if (reverse_flag)
            {
                const NodeID to = facade.GetTarget(edge);
                const int edge_weight = data.weight;
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
};
}
}
}

#endif
