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

/// This is a stripped down version of the general shortest path algorithm.
/// The general algorithm always computes two queries for each leg. This is only
/// necessary in case of vias, where the directions of the start node is constrained
/// by the previous route.
/// This variation is only an optimization for graphs with slow queries, for example
/// not fully contracted graphs.
template <>
InternalRouteResult directShortestPathSearch(SearchEngineData<ch::Algorithm> &engine_working_data,
                                             const DataFacade<ch::Algorithm> &facade,
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

    return extractRoute(facade, weight, phantom_nodes, unpacked_nodes, unpacked_edges);
}

template <>
InternalRouteResult directShortestPathSearch(SearchEngineData<mld::Algorithm> &engine_working_data,
                                             const DataFacade<mld::Algorithm> &facade,
                                             const PhantomNodes &phantom_nodes)
{
    engine_working_data.InitializeOrClearFirstThreadLocalStorage(facade.GetNumberOfNodes(),
                                                                 facade.GetMaxBorderNodeID() + 1);
    auto &forward_heap = *engine_working_data.forward_heap_1;
    auto &reverse_heap = *engine_working_data.reverse_heap_1;
    insertNodesInHeaps(forward_heap, reverse_heap, phantom_nodes);

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
