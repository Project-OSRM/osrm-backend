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
        unsigned target_id; // essentially a row in the distance matrix
        EdgeWeight distance;
        NodeBucket(const unsigned target_id, const EdgeWeight distance)
            : target_id(target_id), distance(distance)
        {
        }
    };
    using SearchSpaceWithBuckets = std::unordered_map<NodeID, std::vector<NodeBucket>>;

  public:
    ManyToManyRouting(DataFacadeT *facade, SearchEngineData &engine_working_data)
        : super(facade), engine_working_data(engine_working_data)
    {
    }

    ~ManyToManyRouting() {}

    std::shared_ptr<std::vector<EdgeWeight>>
    operator()(const std::vector<PhantomNode> &phantom_sources_array,
               const std::vector<PhantomNode> &phantom_targets_array) const
    {
        const auto number_of_sources = phantom_sources_array.size();
        const auto number_of_targets = phantom_targets_array.size();
        std::shared_ptr<std::vector<EdgeWeight>> result_table =
            std::make_shared<std::vector<EdgeWeight>>(number_of_targets * number_of_sources,
                                                      std::numeric_limits<EdgeWeight>::max());

        engine_working_data.InitializeOrClearFirstThreadLocalStorage(
            super::facade->GetNumberOfNodes());

        QueryHeap &query_heap = *(engine_working_data.forward_heap_1);

        SearchSpaceWithBuckets search_space_with_buckets;

        unsigned target_id = 0;
        for (const auto &phantom : phantom_targets_array)
        {
            query_heap.Clear();
            // insert target(s) at distance 0

            if (SPECIAL_NODEID != phantom.forward_node_id)
            {
                query_heap.Insert(phantom.forward_node_id, phantom.GetForwardWeightPlusOffset(),
                                  phantom.forward_node_id);
            }
            if (SPECIAL_NODEID != phantom.reverse_node_id)
            {
                query_heap.Insert(phantom.reverse_node_id, phantom.GetReverseWeightPlusOffset(),
                                  phantom.reverse_node_id);
            }

            // explore search space
            while (!query_heap.Empty())
            {
                BackwardRoutingStep(target_id, query_heap, search_space_with_buckets);
            }
            ++target_id;
        }

        // for each source do forward search
        unsigned source_id = 0;
        for (const auto &phantom : phantom_sources_array)
        {
            query_heap.Clear();
            // insert target(s) at distance 0

            if (SPECIAL_NODEID != phantom.forward_node_id)
            {
                query_heap.Insert(phantom.forward_node_id, -phantom.GetForwardWeightPlusOffset(),
                                  phantom.forward_node_id);
            }
            if (SPECIAL_NODEID != phantom.reverse_node_id)
            {
                query_heap.Insert(phantom.reverse_node_id, -phantom.GetReverseWeightPlusOffset(),
                                  phantom.reverse_node_id);
            }

            // explore search space
            while (!query_heap.Empty())
            {
                ForwardRoutingStep(source_id, number_of_targets, query_heap,
                                   search_space_with_buckets, result_table);
            }

            ++source_id;
        }
        // BOOST_ASSERT(source_id == target_id);
        return result_table;
    }

    void ForwardRoutingStep(const unsigned source_id,
                            const unsigned number_of_targets,
                            QueryHeap &query_heap,
                            const SearchSpaceWithBuckets &search_space_with_buckets,
                            std::shared_ptr<std::vector<EdgeWeight>> result_table) const
    {
        const NodeID node = query_heap.DeleteMin();
        const int source_distance = query_heap.GetKey(node);

        // check if each encountered node has an entry
        const auto bucket_iterator = search_space_with_buckets.find(node);
        // iterate bucket if there exists one
        if (bucket_iterator != search_space_with_buckets.end())
        {
            const std::vector<NodeBucket> &bucket_list = bucket_iterator->second;
            for (const NodeBucket &current_bucket : bucket_list)
            {
                // get target id from bucket entry
                const unsigned target_id = current_bucket.target_id;
                const int target_distance = current_bucket.distance;
                const EdgeWeight current_distance =
                    (*result_table)[source_id * number_of_targets + target_id];
                // check if new distance is better
                const EdgeWeight new_distance = source_distance + target_distance;
                if (new_distance >= 0 && new_distance < current_distance)
                {
                    (*result_table)[source_id * number_of_targets + target_id] =
                        (source_distance + target_distance);
                }
            }
        }
        if (StallAtNode<true>(node, source_distance, query_heap))
        {
            return;
        }
        RelaxOutgoingEdges<true>(node, source_distance, query_heap);
    }

    void BackwardRoutingStep(const unsigned target_id,
                             QueryHeap &query_heap,
                             SearchSpaceWithBuckets &search_space_with_buckets) const
    {
        const NodeID node = query_heap.DeleteMin();
        const int target_distance = query_heap.GetKey(node);

        // store settled nodes in search space bucket
        search_space_with_buckets[node].emplace_back(target_id, target_distance);

        if (StallAtNode<false>(node, target_distance, query_heap))
        {
            return;
        }

        RelaxOutgoingEdges<false>(node, target_distance, query_heap);
    }

    template <bool forward_direction>
    inline void
    RelaxOutgoingEdges(const NodeID node, const EdgeWeight distance, QueryHeap &query_heap) const
    {
        for (auto edge : super::facade->GetAdjacentEdgeRange(node))
        {
            const auto &data = super::facade->GetEdgeData(edge);
            const bool direction_flag = (forward_direction ? data.forward : data.backward);
            if (direction_flag)
            {
                const NodeID to = super::facade->GetTarget(edge);
                const int edge_weight = data.distance;

                BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
                const int to_distance = distance + edge_weight;

                // New Node discovered -> Add to Heap + Node Info Storage
                if (!query_heap.WasInserted(to))
                {
                    query_heap.Insert(to, to_distance, node);
                }
                // Found a shorter Path -> Update distance
                else if (to_distance < query_heap.GetKey(to))
                {
                    // new parent
                    query_heap.GetData(to).parent = node;
                    query_heap.DecreaseKey(to, to_distance);
                }
            }
        }
    }

    // Stalling
    template <bool forward_direction>
    inline bool
    StallAtNode(const NodeID node, const EdgeWeight distance, QueryHeap &query_heap) const
    {
        for (auto edge : super::facade->GetAdjacentEdgeRange(node))
        {
            const auto &data = super::facade->GetEdgeData(edge);
            const bool reverse_flag = ((!forward_direction) ? data.forward : data.backward);
            if (reverse_flag)
            {
                const NodeID to = super::facade->GetTarget(edge);
                const int edge_weight = data.distance;
                BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
                if (query_heap.WasInserted(to))
                {
                    if (query_heap.GetKey(to) + edge_weight < distance)
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
