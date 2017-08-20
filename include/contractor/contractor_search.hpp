#ifndef OSRM_CONTRACTOR_SEARCH_HPP
#define OSRM_CONTRACTOR_SEARCH_HPP

#include "contractor/contractor_graph.hpp"
#include "contractor/contractor_heap.hpp"

#include "util/typedefs.hpp"

#include <cstddef>

namespace osrm
{
namespace contractor
{

void search(ContractorHeap &heap,
            const ContractorGraph &graph,
            const unsigned number_of_targets,
            const int node_limit,
            const EdgeWeight weight_limit,
            const NodeID forbidden_node);

} // namespace contractor
} // namespace osrm

#endif // OSRM_CONTRACTOR_DIJKSTRA_HPP
