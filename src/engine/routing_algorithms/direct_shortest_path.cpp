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
             const PhantomNodes &phantom_nodes,
             const std::vector<NodeID> &unpacked_nodes,
             const std::vector<EdgeID> &unpacked_edges)
{
    InternalRouteResult raw_route_data;
    raw_route_data.segment_end_coordinates = {phantom_nodes};

    // No path found for both target nodes?
    if (INVALID_EDGE_WEIGHT == weight)
    {
        return raw_route_data;
    }

    raw_route_data.shortest_path_weight = weight;
    raw_route_data.unpacked_path_segments.resize(1);
    raw_route_data.source_traversed_in_reverse.push_back(
        (unpacked_nodes.front() != phantom_nodes.source_phantom.forward_segment_id.id));
    raw_route_data.target_traversed_in_reverse.push_back(
        (unpacked_nodes.back() != phantom_nodes.target_phantom.forward_segment_id.id));

    annotatePath(facade,
                 phantom_nodes,
                 unpacked_nodes,
                 unpacked_edges,
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
    auto single_edge_path = insertNodesInHeaps(facade, forward_heap, reverse_heap, phantom_nodes);

    search(engine_working_data,
           facade,
           forward_heap,
           reverse_heap,
           weight,
           packed_leg,
           DO_NOT_FORCE_LOOPS,
           DO_NOT_FORCE_LOOPS,
           phantom_nodes,
           single_edge_path.second);

    std::vector<NodeID> unpacked_nodes;
    std::vector<EdgeID> unpacked_edges;

    if (!packed_leg.empty())
    {
        unpacked_nodes.reserve(packed_leg.size());
        unpacked_edges.reserve(packed_leg.size());
        unpacked_nodes.push_back(packed_leg.front());
        ch::unpackPath(facade,
                       packed_leg.begin(),
                       packed_leg.end(),
                       [&unpacked_nodes, &unpacked_edges](std::pair<NodeID, NodeID> &edge,
                                                          const auto &edge_id) {
                           BOOST_ASSERT(edge.first == unpacked_nodes.back());
                           unpacked_nodes.push_back(edge.second);
                           unpacked_edges.push_back(edge_id);
                       });
    }
    else if (single_edge_path.second != INVALID_EDGE_WEIGHT)
    {
        weight = single_edge_path.second;
        unpacked_nodes.push_back(single_edge_path.first);
    }

    return extractRoute(facade, weight, phantom_nodes, unpacked_nodes, unpacked_edges);
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
    insertNodesInHeaps(facade, forward_heap, reverse_heap, phantom_nodes);

    // TODO: when structured bindings will be allowed change to
    // auto [weight, source_node, target_node, unpacked_edges] = ...
    EdgeWeight weight = INVALID_EDGE_WEIGHT;
    std::vector<NodeID> unpacked_nodes;
    std::vector<EdgeID> unpacked_edges;
    std::tie(weight, unpacked_nodes, unpacked_edges) = mld::search(engine_working_data,
                                                                   facade,
                                                                   forward_heap,
                                                                   reverse_heap,
                                                                   DO_NOT_FORCE_LOOPS,
                                                                   DO_NOT_FORCE_LOOPS,
                                                                   INVALID_EDGE_WEIGHT,
                                                                   phantom_nodes);

    return extractRoute(facade, weight, phantom_nodes, unpacked_nodes, unpacked_edges);
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
