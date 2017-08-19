#ifndef OSRM_CONTRACTOR_GRAPH_CONTRACTOR_HPP
#define OSRM_CONTRACTOR_GRAPH_CONTRACTOR_HPP

#include "contractor/contractor_graph.hpp"

#include <tuple>
#include <vector>

namespace osrm
{
namespace contractor
{

using LevelAndCore = std::tuple<std::vector<float>, std::vector<bool>>;

LevelAndCore contractGraph(ContractorGraph &graph,
                           std::vector<float> cached_node_levels,
                           std::vector<EdgeWeight> node_weights,
                           double core_factor = 1.0);

// Overload for contracting withcout cache
inline LevelAndCore contractGraph(ContractorGraph &graph,
                           std::vector<EdgeWeight> node_weights,
                           double core_factor = 1.0)
{
    return contractGraph(graph, {}, std::move(node_weights), core_factor);
}


} // namespace contractor
} // namespace osrm

#endif // OSRM_CONTRACTOR_GRAPH_CONTRACTOR_HPP
