#include "contractor/contractor_dijkstra.hpp"

namespace osrm
{
namespace contractor
{

ContractorDijkstra::ContractorDijkstra(const std::size_t heap_size) : heap(heap_size) {}

void ContractorDijkstra::Run(const unsigned number_of_targets,
                             const int node_limit,
                             const EdgeWeight weight_limit,
                             const NodeID forbidden_node,
                             const ContractorGraph &graph)
{
    int nodes = 0;
    unsigned number_of_targets_found = 0;
    while (!heap.Empty())
    {
        const NodeID node = heap.DeleteMin();
        const auto node_weight = heap.GetKey(node);
        if (++nodes > node_limit)
        {
            return;
        }
        if (node_weight > weight_limit)
        {
            return;
        }

        // Destination settled?
        if (heap.GetData(node).target)
        {
            ++number_of_targets_found;
            if (number_of_targets_found >= number_of_targets)
            {
                return;
            }
        }

        RelaxNode(node, node_weight, forbidden_node, graph);
    }
}

void ContractorDijkstra::RelaxNode(const NodeID node,
                                   const EdgeWeight node_weight,
                                   const NodeID forbidden_node,
                                   const ContractorGraph &graph)
{
    const short current_hop = heap.GetData(node).hop + 1;
    for (auto edge : graph.GetAdjacentEdgeRange(node))
    {
        const ContractorEdgeData &data = graph.GetEdgeData(edge);
        if (!data.forward)
        {
            continue;
        }
        const NodeID to = graph.GetTarget(edge);
        if (forbidden_node == to)
        {
            continue;
        }
        const EdgeWeight to_weight = node_weight + data.weight;

        // New Node discovered -> Add to Heap + Node Info Storage
        if (!heap.WasInserted(to))
        {
            heap.Insert(to, to_weight, ContractorHeapData{current_hop, false});
        }
        // Found a shorter Path -> Update weight
        else if (to_weight < GetKey(to))
        {
            heap.DecreaseKey(to, to_weight);
            heap.GetData(to).hop = current_hop;
        }
    }
}

void ContractorDijkstra::Clear() { heap.Clear(); }

bool ContractorDijkstra::WasInserted(const NodeID node) const { return heap.WasInserted(node); }

void ContractorDijkstra::Insert(const NodeID node,
                                const ContractorHeap::WeightType weight,
                                const ContractorHeap::DataType &data)
{
    heap.Insert(node, weight, data);
}

ContractorHeap::WeightType ContractorDijkstra::GetKey(const NodeID node)
{
    return heap.GetKey(node);
}

} // namespace contractor
} // namespace osrm
