#ifndef MANY_TO_MANY_ROUTING_HPP
#define MANY_TO_MANY_ROUTING_HPP

#include "engine/datafacade/datafacade_base.hpp"
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

class ManyToManyRouting final : public BasicRoutingInterface
{
    using super = BasicRoutingInterface;
    using QueryHeap = SearchEngineData::ManyToManyQueryHeap;
    SearchEngineData &engine_working_data;

    struct NodeBucket
    {
        unsigned target_id; // essentially a row in the weight matrix
        EdgeWeight weight;
        EdgeWeight duration;
        NodeBucket(const unsigned target_id, const EdgeWeight weight, const EdgeWeight duration)
            : target_id(target_id), weight(weight), duration(duration)
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

    std::vector<EdgeWeight>
    operator()(const std::shared_ptr<const datafacade::BaseDataFacade> facade,
               const std::vector<PhantomNode> &phantom_nodes,
               const std::vector<std::size_t> &source_indices,
               const std::vector<std::size_t> &target_indices,
               const EdgeWeight max_weight = std::numeric_limits<EdgeWeight>::max()) const;

    void ForwardRoutingStep(const std::shared_ptr<const datafacade::BaseDataFacade> facade,
                            const unsigned row_idx,
                            const unsigned number_of_targets,
                            QueryHeap &query_heap,
                            const SearchSpaceWithBuckets &search_space_with_buckets,
                            std::vector<EdgeWeight> &weights_table,
                            std::vector<EdgeWeight> &durations_table,
                            const EdgeWeight max_weight) const;

    void BackwardRoutingStep(const std::shared_ptr<const datafacade::BaseDataFacade> facade,
                             const unsigned column_idx,
                             QueryHeap &query_heap,
                             SearchSpaceWithBuckets &search_space_with_buckets,
                             const EdgeWeight max_weight) const;

    template <bool forward_direction>
    inline void RelaxOutgoingEdges(const std::shared_ptr<const datafacade::BaseDataFacade> facade,
                                   const NodeID node,
                                   const EdgeWeight weight,
                                   const EdgeWeight duration,
                                   QueryHeap &query_heap) const
    {
        for (auto edge : facade->GetAdjacentEdgeRange(node))
        {
            const auto &data = facade->GetEdgeData(edge);
            const bool direction_flag = (forward_direction ? data.forward : data.backward);
            if (direction_flag)
            {
                const NodeID to = facade->GetTarget(edge);
                const EdgeWeight edge_weight = data.weight;
                const EdgeWeight edge_duration = data.duration;

                BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
                const EdgeWeight to_weight = weight + edge_weight;
                const EdgeWeight to_duration = duration + edge_duration;

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

    // Stalling
    template <bool forward_direction>
    inline bool StallAtNode(const std::shared_ptr<const datafacade::BaseDataFacade> facade,
                            const NodeID node,
                            const EdgeWeight weight,
                            QueryHeap &query_heap) const
    {
        for (auto edge : facade->GetAdjacentEdgeRange(node))
        {
            const auto &data = facade->GetEdgeData(edge);
            const bool reverse_flag = ((!forward_direction) ? data.forward : data.backward);
            if (reverse_flag)
            {
                const NodeID to = facade->GetTarget(edge);
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
};

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm

#endif
