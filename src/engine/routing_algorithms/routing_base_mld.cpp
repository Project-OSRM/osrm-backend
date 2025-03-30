#include "engine/routing_algorithms/routing_base_mld.hpp"

namespace osrm::engine::routing_algorithms::mld
{
double getNetworkDistance(SearchEngineData<mld::Algorithm> &engine_working_data,
                          const DataFacade<mld::Algorithm> &facade,
                          typename SearchEngineData<mld::Algorithm>::QueryHeap &forward_heap,
                          typename SearchEngineData<mld::Algorithm>::QueryHeap &reverse_heap,
                          const PhantomNode &source_phantom,
                          const PhantomNode &target_phantom,
                          EdgeWeight weight_upper_bound)
{
    forward_heap.Clear();
    reverse_heap.Clear();

    const PhantomEndpoints endpoints{source_phantom, target_phantom};
    insertNodesInHeaps(forward_heap, reverse_heap, endpoints);

    auto [weight, unpacked_nodes, unpacked_edges] = search(
        engine_working_data, facade, forward_heap, reverse_heap, {}, weight_upper_bound, endpoints);

    if (weight == INVALID_EDGE_WEIGHT)
    {
        return std::numeric_limits<double>::max();
    }

    BOOST_ASSERT(unpacked_nodes.size() >= 1);

    EdgeDistance distance = {0.0};

    if (source_phantom.forward_segment_id.id == unpacked_nodes.front())
    {
        BOOST_ASSERT(source_phantom.forward_segment_id.enabled);
        distance = EdgeDistance{0} - source_phantom.GetForwardDistance();
    }
    else if (source_phantom.reverse_segment_id.id == unpacked_nodes.front())
    {
        BOOST_ASSERT(source_phantom.reverse_segment_id.enabled);
        distance = EdgeDistance{0} - source_phantom.GetReverseDistance();
    }

    for (size_t index = 0; index < unpacked_nodes.size() - 1; ++index)
    {
        distance += facade.GetNodeDistance(unpacked_nodes[index]);
    }

    if (target_phantom.forward_segment_id.id == unpacked_nodes.back())
    {
        BOOST_ASSERT(target_phantom.forward_segment_id.enabled);
        distance += target_phantom.GetForwardDistance();
    }
    else if (target_phantom.reverse_segment_id.id == unpacked_nodes.back())
    {
        BOOST_ASSERT(target_phantom.reverse_segment_id.enabled);
        distance += target_phantom.GetReverseDistance();
    }

    return from_alias<double>(distance);
}
} // namespace osrm::engine::routing_algorithms::mld
