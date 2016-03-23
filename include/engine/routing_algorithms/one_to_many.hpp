//
// Created by robin on 3/16/16.
//

#ifndef OSRM_ONE_TO_MANY_HPP
#define OSRM_ONE_TO_MANY_HPP

#include "engine/routing_algorithms/routing_base.hpp"
#include "engine/search_engine_data.hpp"
#include "util/typedefs.hpp"
#include "util/simple_logger.hpp"

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
class OneToManyRouting final
    : public BasicRoutingInterface<DataFacadeT, OneToManyRouting<DataFacadeT>>
{
    using super = BasicRoutingInterface<DataFacadeT, OneToManyRouting<DataFacadeT>>;
    using QueryHeap = SearchEngineData::QueryHeap;
    using PredecessorMap = std::unordered_map<NodeID, NodeID>;
    using DistanceMap = std::unordered_map<NodeID, EdgeWeight>;
    SearchEngineData &engine_working_data;

  public:
    OneToManyRouting(DataFacadeT *facade, SearchEngineData &engine_working_data)
        : super(facade), engine_working_data(engine_working_data)
    {
    }

    ~OneToManyRouting() {}

    void operator()(PhantomNode &phantomSource,
                    const int distance,
                    PredecessorMap &predecessorMap,
                    DistanceMap &distanceMap) const
    {

        engine_working_data.InitializeOrClearFirstThreadLocalStorage(
            super::facade->GetNumberOfNodes());

        QueryHeap &query_heap = *(engine_working_data.forward_heap_1);
        QueryHeap &backward_heap = *(engine_working_data.reverse_heap_1);

        query_heap.Clear();
        // insert target(s) at distance 0
        if (SPECIAL_NODEID != phantomSource.forward_node_id)
        {
            query_heap.Insert(phantomSource.forward_node_id,
                              -phantomSource.GetForwardWeightPlusOffset(),
                              phantomSource.forward_node_id);
        }
        if (SPECIAL_NODEID != phantomSource.reverse_node_id)
        {
            query_heap.Insert(phantomSource.reverse_node_id,
                              -phantomSource.GetReverseWeightPlusOffset(),
                              phantomSource.reverse_node_id);
        }

        util::SimpleLogger().Write() << phantomSource;
        // explore search space
        while (!query_heap.Empty())
        {
<<<<<<< HEAD
<<<<<<< HEAD
//            ForwardRoutingStep(query_heap, distance, predecessorMap, distanceMap);
            ForwardRoutingStep(query_heap, backward_heap, )
=======
            ForwardRoutingStep(query_heap, distance, predecessorMap, distanceMap);
>>>>>>> first implementation of routing algo with pretty json-print
=======
//            ForwardRoutingStep(query_heap, distance, predecessorMap, distanceMap);
            ForwardRoutingStep(query_heap, backward_heap, )
>>>>>>> added testing
        }
    }

    void ForwardRoutingStep(QueryHeap &query_heap,
                            const int distance,
                            PredecessorMap &predecessorMap,
                            DistanceMap &distanceMap) const
    {
        const NodeID node = query_heap.DeleteMin();
        const int source_distance = query_heap.GetKey(node);
        for (auto edge : super::facade->GetAdjacentEdgeRange(node))
        {
            const auto &data = super::facade->GetEdgeData(edge);
            const NodeID to = super::facade->GetTarget(edge);
            const int edge_weight = data.distance;
            BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");

            if (query_heap.WasInserted(to))
            {
                if (query_heap.GetKey(to) + edge_weight < source_distance)
                {
                    continue;
                }
            }
            const int to_distance = source_distance + edge_weight;

            if (to_distance > distance)
            {
                continue;
            }

            // New Node discovered -> Add to Heap + Node Info Storage
            if (!query_heap.WasInserted(to))
            {
                query_heap.Insert(to, to_distance, node);
                predecessorMap[to] = node;
                distanceMap[to] = to_distance;
            }
            // Found a shorter Path -> Update distance
            else if (to_distance < query_heap.GetKey(to))
            {
                // new parent
                query_heap.GetData(to).parent = node;
                query_heap.DecreaseKey(to, to_distance);

                predecessorMap[to] = node;
                distanceMap[to] = to_distance;
            }
        }
    }
};
}
}
}
#endif // OSRM_ONE_TO_MANY_HPP
