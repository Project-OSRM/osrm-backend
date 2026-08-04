#include "contractor/contractor_search.hpp"

#include "contractor/contractor_graph.hpp"
#include "contractor/contractor_heap.hpp"
#include "util/typedefs.hpp"

namespace osrm::contractor
{

namespace
{
// Returns true if the search must stop because the heap's fixed-capacity storage is
// running out of room. Callers must abort the search immediately in that case: further
// insertions risk filling the hash table completely, which turns its linear probing into
// an infinite loop.
bool relaxNode(ContractorHeap &heap,
               const ContractorGraph &graph,
               const std::vector<bool> &contractible,
               const NodeID node,
               const EdgeWeight node_weight,
               const NodeID forbidden_node)
{
    for (auto edge : graph.GetAdjacentEdgeRange(node))
    {
        const auto &data = graph.GetEdgeData(edge);
        if (!data.forward)
        {
            continue;
        }
        const NodeID to = graph.GetTarget(edge);
        BOOST_ASSERT(to != SPECIAL_NODEID);
        if (forbidden_node == to)
        {
            continue;
        }
        const EdgeWeight to_weight = node_weight + data.weight;

        const auto toHeapNode = heap.GetHeapNodeIfWasInserted(to);
        // New Node discovered -> Add to Heap + Node Info Storage
        if (!toHeapNode)
        {
            if (!contractible[to])
            {
                continue;
            }
            if (heap.Occupancy() >= RELAXED_NODE_LIMIT)
            {
                return true; // stop search
            }
            heap.Insert(to, to_weight, false);
        }
        // Found a shorter Path -> Update weight
        else if (to_weight < toHeapNode->weight)
        {
            toHeapNode->weight = to_weight;
            heap.DecreaseKey(*toHeapNode);
        }
    }
    return false;
}
} // namespace

void search(ContractorHeap &heap,
            const ContractorGraph &graph,
            const NodeID start,
            const std::vector<bool> &contractible,
            const unsigned number_of_targets,
            const int node_limit,
            const EdgeWeight weight_limit,
            const NodeID forbidden_node)
{
    int nodes = 0;
    unsigned number_of_targets_found = 0;
    if (relaxNode(heap, graph, contractible, start, EdgeWeight{0}, forbidden_node))
    {
        return;
    }
    while (!heap.Empty())
    {
        const NodeID node = heap.DeleteMin();
        BOOST_ASSERT(node != SPECIAL_NODEID);
        const auto node_weight = heap.GetKey(node);
        if (++nodes > node_limit)
        {
            return;
        }
        if (node_weight > weight_limit)
        {
            return;
        }

        // Target settled?
        if (heap.GetData(node))
        {
            ++number_of_targets_found;
            if (number_of_targets_found >= number_of_targets)
            {
                return;
            }
        }

        if (relaxNode(heap, graph, contractible, node, node_weight, forbidden_node))
        {
            return;
        }
    }
}
} // namespace osrm::contractor
