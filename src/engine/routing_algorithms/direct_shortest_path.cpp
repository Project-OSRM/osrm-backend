#include "engine/routing_algorithms/direct_shortest_path.hpp"
#include "engine/routing_algorithms/routing_base.hpp"
#include "engine/routing_algorithms/routing_base_ch.hpp"
#include "engine/routing_algorithms/routing_base_mld.hpp"

namespace osrm::engine::routing_algorithms
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
                                             const PhantomEndpointCandidates &endpoint_candidates)
{
    engine_working_data.InitializeOrClearFirstThreadLocalStorage(facade.GetNumberOfNodes());
    auto &forward_heap = *engine_working_data.forward_heap_1;
    auto &reverse_heap = *engine_working_data.reverse_heap_1;
    forward_heap.Clear();
    reverse_heap.Clear();

    EdgeWeight weight = INVALID_EDGE_WEIGHT;
    std::vector<NodeID> packed_leg;
    insertNodesInHeaps(forward_heap, reverse_heap, endpoint_candidates);

    search(engine_working_data,
           facade,
           forward_heap,
           reverse_heap,
           weight,
           packed_leg,
           {},
           endpoint_candidates);

    std::vector<NodeID> unpacked_nodes;
    std::vector<EdgeID> unpacked_edges;

    if (!packed_leg.empty())
    {
        unpacked_nodes.reserve(packed_leg.size());
        unpacked_edges.reserve(packed_leg.size());
        unpacked_nodes.push_back(packed_leg.front());
        ch::unpackPath(
            facade,
            packed_leg.begin(),
            packed_leg.end(),
            [&unpacked_nodes, &unpacked_edges](std::pair<NodeID, NodeID> &edge, const auto &edge_id)
            {
                BOOST_ASSERT(edge.first == unpacked_nodes.back());
                unpacked_nodes.push_back(edge.second);
                unpacked_edges.push_back(edge_id);
            });
    }

    return extractRoute(facade, weight, endpoint_candidates, unpacked_nodes, unpacked_edges);
}

template <>
InternalRouteResult directShortestPathSearch(SearchEngineData<mld::Algorithm> &engine_working_data,
                                             const DataFacade<mld::Algorithm> &facade,
                                             const PhantomEndpointCandidates &endpoint_candidates)
{
    engine_working_data.InitializeOrClearFirstThreadLocalStorage(facade.GetNumberOfNodes(),
                                                                 facade.GetMaxBorderNodeID() + 1);
    auto &forward_heap = *engine_working_data.forward_heap_1;
    auto &reverse_heap = *engine_working_data.reverse_heap_1;
    insertNodesInHeaps(forward_heap, reverse_heap, endpoint_candidates);

    auto unpacked_path = mld::search(engine_working_data,
                                     facade,
                                     forward_heap,
                                     reverse_heap,
                                     {},
                                     INVALID_EDGE_WEIGHT,
                                     endpoint_candidates);

    return extractRoute(facade,
                        unpacked_path.weight,
                        endpoint_candidates,
                        unpacked_path.nodes,
                        unpacked_path.edges);
}

} // namespace osrm::engine::routing_algorithms
