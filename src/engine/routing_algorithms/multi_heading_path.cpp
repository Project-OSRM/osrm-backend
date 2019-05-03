#include "engine/routing_algorithms/multi_heading_path.hpp"
#include "engine/routing_algorithms/routing_base.hpp"
#include "engine/routing_algorithms/routing_base_ch.hpp"
#include "engine/routing_algorithms/routing_base_mld.hpp"
#include "util/static_assert.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <iterator>
#include <memory>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/function_output_iterator.hpp>

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{

template <>
InternalManyRoutesResult
multiHeadingDirectShortestPathsSearch(SearchEngineData<ch::Algorithm> &engine_working_data,
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

    std::vector<InternalRouteResult> routes;
    routes.reserve(6);

    InternalRouteResult route =
        extractRoute(facade, weight, phantom_nodes, unpacked_nodes, unpacked_edges);
    routes.push_back(route);
    BOOST_ASSERT(routes.size() >= 1);

    return InternalManyRoutesResult{std::move(routes)};
}

template <>
InternalManyRoutesResult
multiHeadingDirectShortestPathsSearch(SearchEngineData<mld::Algorithm> &engine_working_data,
                                      const DataFacade<mld::Algorithm> &facade,
                                      const PhantomNodes &phantom_nodes)
{
    engine_working_data.InitializeOrClearFirstThreadLocalStorage(facade.GetNumberOfNodes(),
                                                                 facade.GetMaxBorderNodeID() + 1);

    std::vector<InternalRouteResult> routes;
    routes.reserve(6);

    //                insertNodesInHeaps(forward_heap, reverse_heap, phantom_nodes);

    const auto &source = phantom_nodes.source_phantom;
    const auto &target = phantom_nodes.target_phantom;

    auto &forward_heap = *engine_working_data.forward_heap_1;
    auto &reverse_heap = *engine_working_data.reverse_heap_1;
    if (source.IsValidForwardSource() && target.IsValidForwardTarget())
    {

        forward_heap.Clear();
        reverse_heap.Clear();
        forward_heap.Insert(source.forward_segment_id.id,
                            -source.GetForwardWeightPlusOffset(),
                            source.forward_segment_id.id);
        reverse_heap.Insert(target.forward_segment_id.id,
                            target.GetForwardWeightPlusOffset(),
                            target.forward_segment_id.id);
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
        InternalRouteResult route =
            extractRoute(facade, weight, phantom_nodes, unpacked_nodes, unpacked_edges);

        routes.push_back(route);
    }

    if (source.IsValidReverseSource() && target.IsValidForwardTarget())
    {
        forward_heap.Clear();
        reverse_heap.Clear();
        forward_heap.Insert(source.reverse_segment_id.id,
                            -source.GetReverseWeightPlusOffset(),
                            source.reverse_segment_id.id);
        reverse_heap.Insert(target.forward_segment_id.id,
                            target.GetForwardWeightPlusOffset(),
                            target.forward_segment_id.id);
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
        InternalRouteResult route =
            extractRoute(facade, weight, phantom_nodes, unpacked_nodes, unpacked_edges);

        routes.push_back(route);
    }

    if (source.IsValidForwardSource() && target.IsValidReverseTarget())
    {
        forward_heap.Clear();
        reverse_heap.Clear();
        forward_heap.Insert(source.forward_segment_id.id,
                            -source.GetForwardWeightPlusOffset(),
                            source.forward_segment_id.id);
        reverse_heap.Insert(target.reverse_segment_id.id,
                            target.GetReverseWeightPlusOffset(),
                            target.reverse_segment_id.id);
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
        InternalRouteResult route =
            extractRoute(facade, weight, phantom_nodes, unpacked_nodes, unpacked_edges);

        routes.push_back(route);
    }
    if (source.IsValidReverseSource() && target.IsValidReverseTarget())
    {
        forward_heap.Clear();
        reverse_heap.Clear();
        forward_heap.Insert(source.reverse_segment_id.id,
                            -source.GetReverseWeightPlusOffset(),
                            source.reverse_segment_id.id);
        reverse_heap.Insert(target.reverse_segment_id.id,
                            target.GetReverseWeightPlusOffset(),
                            target.reverse_segment_id.id);
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
        InternalRouteResult route =
            extractRoute(facade, weight, phantom_nodes, unpacked_nodes, unpacked_edges);

        routes.push_back(route);
    }

    BOOST_ASSERT(routes.size() >= 1);

    return InternalManyRoutesResult{std::move(routes)};
}

} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
