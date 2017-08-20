#ifndef OSRM_CONTRACTOR_GRAPH_CONTRACTOR_HPP
#define OSRM_CONTRACTOR_GRAPH_CONTRACTOR_HPP

#include "contractor/contractor_graph.hpp"

#include "util/filtered_graph.hpp"

#include <tuple>
#include <vector>

namespace osrm
{
namespace contractor
{

std::vector<bool> contractGraph(ContractorGraph &graph,
                                std::vector<bool> node_is_uncontracted,
                                std::vector<bool> node_is_contractable,
                                std::vector<EdgeWeight> node_weights,
                                double core_factor = 1.0);

// Overload for contracting all nodes
inline auto contractGraph(ContractorGraph &graph,
                          std::vector<EdgeWeight> node_weights,
                          double core_factor = 1.0)
{
    return contractGraph(graph, {}, {}, std::move(node_weights), core_factor);
}

// Overload no contracted nodes
inline auto contractGraph(ContractorGraph &graph,
                          std::vector<bool> node_is_contractable,
                          std::vector<EdgeWeight> node_weights,
                          double core_factor = 1.0)
{
    return contractGraph(
        graph, {}, std::move(node_is_contractable), std::move(node_weights), core_factor);
}

} // namespace contractor
} // namespace osrm

#endif // OSRM_CONTRACTOR_GRAPH_CONTRACTOR_HPP
