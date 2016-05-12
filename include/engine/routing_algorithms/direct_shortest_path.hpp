#ifndef DIRECT_SHORTEST_PATH_HPP
#define DIRECT_SHORTEST_PATH_HPP

#include <boost/assert.hpp>
#include <iterator>

#include "engine/routing_algorithms/routing_base.hpp"
#include "engine/search_engine_data.hpp"
#include "util/integer_range.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

/// This is a striped down version of the general shortest path algorithm.
/// The general algorithm always computes two queries for each leg. This is only
/// necessary in case of vias, where the directions of the start node is constrainted
/// by the previous route.
/// This variation is only an optimazation for graphs with slow queries, for example
/// not fully contracted graphs.
template <class DataFacadeT>
class DirectShortestPathRouting final
    : public BasicRoutingInterface<DataFacadeT, DirectShortestPathRouting<DataFacadeT>>
{
    using super = BasicRoutingInterface<DataFacadeT, DirectShortestPathRouting<DataFacadeT>>;
    using QueryHeap = SearchEngineData::QueryHeap;
    SearchEngineData &engine_working_data;

  public:
    DirectShortestPathRouting(SearchEngineData &engine_working_data)
        : engine_working_data(engine_working_data)
    {
    }

    ~DirectShortestPathRouting() {}

    void operator()(const DataFacadeT &facade,
                    const std::vector<PhantomNodes> &phantom_nodes_vector,
                    InternalRouteResult &raw_route_data) const
    {
        // Get weight to next pair of target nodes.
        BOOST_ASSERT_MSG(1 == phantom_nodes_vector.size(),
                         "Direct Shortest Path Query only accepts a single source and target pair. "
                         "Multiple ones have been specified.");
        const auto &phantom_node_pair = phantom_nodes_vector.front();
        const auto &source_phantom = phantom_node_pair.source_phantom;
        const auto &target_phantom = phantom_node_pair.target_phantom;

        engine_working_data.InitializeOrClearFirstThreadLocalStorage(facade.GetNumberOfNodes());
        QueryHeap &forward_heap = *(engine_working_data.forward_heap_1);
        QueryHeap &reverse_heap = *(engine_working_data.reverse_heap_1);
        forward_heap.Clear();
        reverse_heap.Clear();

        BOOST_ASSERT(source_phantom.IsValid());
        BOOST_ASSERT(target_phantom.IsValid());

        if (source_phantom.forward_segment_id.enabled)
        {
            forward_heap.Insert(source_phantom.forward_segment_id.id,
                                -source_phantom.GetForwardWeightPlusOffset(),
                                source_phantom.forward_segment_id.id);
        }
        if (source_phantom.reverse_segment_id.enabled)
        {
            forward_heap.Insert(source_phantom.reverse_segment_id.id,
                                -source_phantom.GetReverseWeightPlusOffset(),
                                source_phantom.reverse_segment_id.id);
        }

        if (target_phantom.forward_segment_id.enabled)
        {
            reverse_heap.Insert(target_phantom.forward_segment_id.id,
                                target_phantom.GetForwardWeightPlusOffset(),
                                target_phantom.forward_segment_id.id);
        }

        if (target_phantom.reverse_segment_id.enabled)
        {
            reverse_heap.Insert(target_phantom.reverse_segment_id.id,
                                target_phantom.GetReverseWeightPlusOffset(),
                                target_phantom.reverse_segment_id.id);
        }

        int weight = INVALID_EDGE_WEIGHT;
        std::vector<NodeID> packed_leg;

        const bool constexpr DO_NOT_FORCE_LOOPS =
            false; // prevents forcing of loops, since offsets are set correctly

        if (facade.GetCoreSize() > 0)
        {
            engine_working_data.InitializeOrClearSecondThreadLocalStorage(
                facade.GetNumberOfNodes());
            QueryHeap &forward_core_heap = *(engine_working_data.forward_heap_2);
            QueryHeap &reverse_core_heap = *(engine_working_data.reverse_heap_2);
            forward_core_heap.Clear();
            reverse_core_heap.Clear();

            super::SearchWithCore(facade,
                                  forward_heap,
                                  reverse_heap,
                                  forward_core_heap,
                                  reverse_core_heap,
                                  weight,
                                  packed_leg,
                                  DO_NOT_FORCE_LOOPS,
                                  DO_NOT_FORCE_LOOPS);
        }
        else
        {
            super::Search(facade,
                          forward_heap,
                          reverse_heap,
                          weight,
                          packed_leg,
                          DO_NOT_FORCE_LOOPS,
                          DO_NOT_FORCE_LOOPS);
        }

        // No path found for both target nodes?
        if (INVALID_EDGE_WEIGHT == weight)
        {
            raw_route_data.shortest_path_length = INVALID_EDGE_WEIGHT;
            raw_route_data.alternative_path_length = INVALID_EDGE_WEIGHT;
            return;
        }

        BOOST_ASSERT_MSG(!packed_leg.empty(), "packed path empty");

        raw_route_data.shortest_path_length = weight;
        raw_route_data.unpacked_path_segments.resize(1);
        raw_route_data.source_traversed_in_reverse.push_back(
            (packed_leg.front() != phantom_node_pair.source_phantom.forward_segment_id.id));
        raw_route_data.target_traversed_in_reverse.push_back(
            (packed_leg.back() != phantom_node_pair.target_phantom.forward_segment_id.id));

        super::UnpackPath(facade,
                          packed_leg.begin(),
                          packed_leg.end(),
                          phantom_node_pair,
                          raw_route_data.unpacked_path_segments.front());
    }
};
}
}
}

#endif /* DIRECT_SHORTEST_PATH_HPP */
