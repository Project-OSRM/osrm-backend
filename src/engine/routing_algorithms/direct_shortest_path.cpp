#include "engine/routing_algorithms/direct_shortest_path.hpp"

#include "engine/routing_algorithms/routing_base.hpp"

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
InternalRouteResult directShortestPathSearch(
    SearchEngineData &engine_working_data,
    const datafacade::ContiguousInternalMemoryDataFacade<algorithm::CH> &facade,
    const std::vector<PhantomNodes> &phantom_nodes_vector)
{
    InternalRouteResult raw_route_data;
    // Get weight to next pair of target nodes.
    BOOST_ASSERT_MSG(1 == phantom_nodes_vector.size(),
                     "Direct Shortest Path Query only accepts a single source and target pair. "
                     "Multiple ones have been specified.");
    const auto &phantom_node_pair = phantom_nodes_vector.front();
    const auto &source_phantom = phantom_node_pair.source_phantom;
    const auto &target_phantom = phantom_node_pair.target_phantom;

    engine_working_data.InitializeOrClearFirstThreadLocalStorage(facade.GetNumberOfNodes());
    auto &forward_heap = *(engine_working_data.forward_heap_1);
    auto &reverse_heap = *(engine_working_data.reverse_heap_1);
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
        engine_working_data.InitializeOrClearSecondThreadLocalStorage(facade.GetNumberOfNodes());
        auto &forward_core_heap = *(engine_working_data.forward_heap_2);
        auto &reverse_core_heap = *(engine_working_data.reverse_heap_2);
        forward_core_heap.Clear();
        reverse_core_heap.Clear();

        searchWithCore(facade,
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
        search(facade,
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
        return raw_route_data;
    }

    BOOST_ASSERT_MSG(!packed_leg.empty(), "packed path empty");

    raw_route_data.shortest_path_length = weight;
    raw_route_data.unpacked_path_segments.resize(1);
    raw_route_data.source_traversed_in_reverse.push_back(
        (packed_leg.front() != phantom_node_pair.source_phantom.forward_segment_id.id));
    raw_route_data.target_traversed_in_reverse.push_back(
        (packed_leg.back() != phantom_node_pair.target_phantom.forward_segment_id.id));

    unpackPath(facade,
               packed_leg.begin(),
               packed_leg.end(),
               phantom_node_pair,
               raw_route_data.unpacked_path_segments.front());

    return raw_route_data;
}
} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
