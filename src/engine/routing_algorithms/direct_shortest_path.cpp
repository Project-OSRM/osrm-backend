#include "engine/routing_algorithms/direct_shortest_path.hpp"

#include "engine/routing_algorithms/routing_base.hpp"
#include "engine/routing_algorithms/routing_base_ch.hpp"
#include "engine/routing_algorithms/routing_base_mld.hpp"

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

template <typename AlgorithmT>
InternalRouteResult
extractRoute(const datafacade::ContiguousInternalMemoryDataFacade<AlgorithmT> &facade,
             const EdgeWeight weight,
             const NodeID source_node,
             const NodeID target_node,
             const std::vector<EdgeID> &edges,
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

    raw_route_data.shortest_path_length = weight;
    raw_route_data.unpacked_path_segments.resize(1);
    raw_route_data.source_traversed_in_reverse.push_back(
        (source_node != nodes.source_phantom.forward_segment_id.id));
    raw_route_data.target_traversed_in_reverse.push_back(
        (target_node != nodes.target_phantom.forward_segment_id.id));

    annotatePath(facade,
                 source_node,
                 target_node,
                 edges,
                 nodes,
                 raw_route_data.unpacked_path_segments.front());

    return raw_route_data;
}

namespace detail
{
/// This is a striped down version of the general shortest path algorithm.
/// The general algorithm always computes two queries for each leg. This is only
/// necessary in case of vias, where the directions of the start node is constrainted
/// by the previous route.
/// This variation is only an optimazation for graphs with slow queries, for example
/// not fully contracted graphs.
template <typename Algorithm>
InternalRouteResult directShortestPathSearchImpl(
    SearchEngineData<Algorithm> &engine_working_data,
    const datafacade::ContiguousInternalMemoryDataFacade<Algorithm> &facade,
    const PhantomNodes &phantom_nodes)
{
    engine_working_data.InitializeOrClearFirstThreadLocalStorage(facade.GetNumberOfNodes());
    auto &forward_heap = *engine_working_data.forward_heap_1;
    auto &reverse_heap = *engine_working_data.reverse_heap_1;
    forward_heap.Clear();
    reverse_heap.Clear();

    EdgeWeight weight = INVALID_EDGE_WEIGHT;
    std::vector<NodeID> packed_leg;
    insertNodesInHeaps(forward_heap, reverse_heap, phantom_nodes);

    search(engine_working_data,
           facade,
           forward_heap,
           reverse_heap,
           weight,
           packed_leg,
           DO_NOT_FORCE_LOOPS,
           DO_NOT_FORCE_LOOPS,
           phantom_nodes);

    std::vector<EdgeID> unpacked_edges;
    auto source_node = SPECIAL_NODEID, target_node = SPECIAL_NODEID;
    if (!packed_leg.empty())
    {
        source_node = packed_leg.front();
        target_node = packed_leg.back();
        unpacked_edges.reserve(packed_leg.size());
        ch::unpackPath(
            facade,
            packed_leg.begin(),
            packed_leg.end(),
            [&facade, &unpacked_edges](std::pair<NodeID, NodeID> & /* edge */,
                                       const auto &edge_id) { unpacked_edges.push_back(edge_id); });
    }

    return extractRoute(facade, weight, source_node, target_node, unpacked_edges, phantom_nodes);
}

} // namespace ch

InternalRouteResult directShortestPathSearch(
    SearchEngineData<corech::Algorithm> &engine_working_data,
    const datafacade::ContiguousInternalMemoryDataFacade<corech::Algorithm> &facade,
    const PhantomNodes &phantom_nodes)
{
    return detail::directShortestPathSearchImpl(engine_working_data, facade, phantom_nodes);
}

InternalRouteResult directShortestPathSearch(
    SearchEngineData<ch::Algorithm> &engine_working_data,
    const datafacade::ContiguousInternalMemoryDataFacade<ch::Algorithm> &facade,
    const PhantomNodes &phantom_nodes)
{
    return detail::directShortestPathSearchImpl(engine_working_data, facade, phantom_nodes);
}

InternalRouteResult directShortestPathSearch(
    SearchEngineData<mld::Algorithm> &engine_working_data,
    const datafacade::ContiguousInternalMemoryDataFacade<mld::Algorithm> &facade,
    const PhantomNodes &phantom_nodes)
{
    engine_working_data.InitializeOrClearFirstThreadLocalStorage(facade.GetNumberOfNodes());
    auto &forward_heap = *engine_working_data.forward_heap_1;
    auto &reverse_heap = *engine_working_data.reverse_heap_1;
    insertNodesInHeaps(forward_heap, reverse_heap, phantom_nodes);

    // TODO: when structured bindings will be allowed change to
    // auto [weight, source_node, target_node, unpacked_edges] = ...
    EdgeWeight weight;
    NodeID source_node, target_node;
    std::vector<EdgeID> unpacked_edges;
    std::tie(weight, source_node, target_node, unpacked_edges) = mld::search(engine_working_data,
                                                                             facade,
                                                                             forward_heap,
                                                                             reverse_heap,
                                                                             DO_NOT_FORCE_LOOPS,
                                                                             DO_NOT_FORCE_LOOPS,
                                                                             INVALID_EDGE_WEIGHT,
                                                                             phantom_nodes);

    return extractRoute(facade, weight, source_node, target_node, unpacked_edges, phantom_nodes);
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
