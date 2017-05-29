#include "engine/routing_algorithms/routing_base_mld.hpp"
#include "engine/routing_algorithms/routing_init.hpp"

namespace osrm
{
namespace engine
{
namespace routing_algorithms
{
namespace mld
{

double getNetworkDistance(SearchEngineData<Algorithm> &engine_working_data,
                          const datafacade::ContiguousInternalMemoryDataFacade<Algorithm> &facade,
                          SearchEngineData<Algorithm>::QueryHeap &forward_heap,
                          SearchEngineData<Algorithm>::QueryHeap &reverse_heap,
                          const PhantomNode &source_phantom,
                          const PhantomNode &target_phantom,
                          EdgeWeight weight_upper_bound)
{
    forward_heap.Clear();
    reverse_heap.Clear();

    const PhantomNodes phantom_nodes{source_phantom, target_phantom};
    insertNodesInHeaps(facade, forward_heap, reverse_heap, phantom_nodes);

    EdgeWeight weight = INVALID_EDGE_WEIGHT;
    std::vector<NodeID> unpacked_nodes;
    std::vector<EdgeID> unpacked_edges;
    std::tie(weight, unpacked_nodes, unpacked_edges) = search(engine_working_data,
                                                              facade,
                                                              forward_heap,
                                                              reverse_heap,
                                                              DO_NOT_FORCE_LOOPS,
                                                              DO_NOT_FORCE_LOOPS,
                                                              weight_upper_bound,
                                                              phantom_nodes);

    if (weight == INVALID_EDGE_WEIGHT)
    {
        return std::numeric_limits<double>::max();
    }

    std::vector<PathData> unpacked_path;

    annotatePath(facade, phantom_nodes, unpacked_nodes, unpacked_edges, unpacked_path);

    return getPathDistance(facade, unpacked_path, source_phantom, target_phantom);
}

} // namespace mld
} // namespace routing_algorithms
} // namespace engine
} // namespace osrm
