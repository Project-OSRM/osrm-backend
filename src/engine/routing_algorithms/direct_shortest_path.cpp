#include "engine/routing_algorithms/direct_shortest_path.hpp"

#include "engine/routing_algorithms/routing_base.hpp"

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

namespace
{
void insertInHeaps(SearchEngineData::QueryHeap &forward_heap,
                   SearchEngineData::QueryHeap &reverse_heap,
                   const PhantomNodes &nodes)
{
    const auto &source_phantom = nodes.source_phantom;
    const auto &target_phantom = nodes.target_phantom;
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
}

template <typename AlgorithmT>
InternalRouteResult
extractRoute(const datafacade::ContiguousInternalMemoryDataFacade<AlgorithmT> &facade,
             const EdgeWeight weight,
             const std::vector<NodeID> &packed_leg,
             const PhantomNodes &nodes)
{
    InternalRouteResult raw_route_data;
    raw_route_data.segment_end_coordinates = {nodes};
    // No path found for both target nodes?
    if (INVALID_EDGE_WEIGHT == weight)
    {
        raw_route_data.shortest_path_length = INVALID_EDGE_WEIGHT;
        raw_route_data.alternative_path_length = INVALID_EDGE_WEIGHT;
        return raw_route_data;
    }

    BOOST_ASSERT_MSG(!packed_leg.empty(), "packed path empty");

    raw_route_data.shortest_path_length = weight;
    raw_route_data.unpacked_path_segments.resize(1);
    raw_route_data.source_traversed_in_reverse.push_back(
        (packed_leg.front() != nodes.source_phantom.forward_segment_id.id));
    raw_route_data.target_traversed_in_reverse.push_back(
        (packed_leg.back() != nodes.target_phantom.forward_segment_id.id));

    unpackPath(facade,
               packed_leg.begin(),
               packed_leg.end(),
               nodes,
               raw_route_data.unpacked_path_segments.front());

    return raw_route_data;
}
}

/// This is a striped down version of the general shortest path algorithm.
/// The general algorithm always computes two queries for each leg. This is only
/// necessary in case of vias, where the directions of the start node is constrainted
/// by the previous route.
/// This variation is only an optimazation for graphs with slow queries, for example
/// not fully contracted graphs.
template <typename AlgorithmT>
InternalRouteResult directShortestPathSearchImpl(
    SearchEngineData &engine_working_data,
    const datafacade::ContiguousInternalMemoryDataFacade<AlgorithmT> &facade,
    const PhantomNodes &phantom_nodes)
{
    engine_working_data.InitializeOrClearFirstThreadLocalStorage(facade.GetNumberOfNodes());
    engine_working_data.InitializeOrClearSecondThreadLocalStorage(facade.GetNumberOfNodes());
    auto &forward_heap = *(engine_working_data.forward_heap_1);
    auto &reverse_heap = *(engine_working_data.reverse_heap_1);
    auto &forward_core_heap = *(engine_working_data.forward_heap_2);
    auto &reverse_core_heap = *(engine_working_data.reverse_heap_2);
    forward_heap.Clear();
    reverse_heap.Clear();
    forward_core_heap.Clear();
    reverse_core_heap.Clear();

    int weight = INVALID_EDGE_WEIGHT;
    std::vector<NodeID> packed_leg;
    insertInHeaps(forward_heap, reverse_heap, phantom_nodes);

    search(facade,
           forward_heap,
           reverse_heap,
           forward_core_heap,
           reverse_core_heap,
           weight,
           packed_leg,
           DO_NOT_FORCE_LOOPS,
           DO_NOT_FORCE_LOOPS);

    return extractRoute(facade, weight, packed_leg, phantom_nodes);
}

InternalRouteResult directShortestPathSearch(
    SearchEngineData &engine_working_data,
    const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CoreCH> &facade,
    const PhantomNodes &phantom_nodes)
{
    return directShortestPathSearchImpl(engine_working_data, facade, phantom_nodes);
}

InternalRouteResult directShortestPathSearch(
    SearchEngineData &engine_working_data,
    const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
    const PhantomNodes &phantom_nodes)
{
    return directShortestPathSearchImpl(engine_working_data, facade, phantom_nodes);
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
