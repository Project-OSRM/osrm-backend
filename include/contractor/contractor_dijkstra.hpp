#ifndef OSRM_CONTRACTOR_DIJKSTRA_HPP
#define OSRM_CONTRACTOR_DIJKSTRA_HPP

#include "contractor/contractor_graph.hpp"
#include "contractor/contractor_heap.hpp"
#include "util/typedefs.hpp"

#include <cstddef>

namespace osrm
{
namespace contractor
{

// allow access to the heap itself, add Dijkstra functionality on top
class ContractorDijkstra
{
  public:
    ContractorDijkstra(std::size_t heap_size);

    // search the graph up
    void Run(const unsigned number_of_targets,
             const int node_limit,
             const int weight_limit,
             const NodeID forbidden_node,
             const ContractorGraph &graph);

    // adaption of the heap interface
    void Clear();
    bool WasInserted(const NodeID node) const;
    void Insert(const NodeID node,
                const ContractorHeap::WeightType weight,
                const ContractorHeap::DataType &data);

    // cannot be const due to node-hash access in the binary heap :(
    ContractorHeap::WeightType GetKey(const NodeID node);

  private:
    void RelaxNode(const NodeID node,
                   const int node_weight,
                   const NodeID forbidden_node,
                   const ContractorGraph &graph);

    ContractorHeap heap;
};

} // namespace contractor
} // namespace osrm

#endif // OSRM_CONTRACTOR_DIJKSTRA_HPP
