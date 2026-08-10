#ifndef OSRM_CONTRACTOR_GRAPH_CONTRACTOR_HPP
#define OSRM_CONTRACTOR_GRAPH_CONTRACTOR_HPP

#include "contractor/contractor_graph.hpp"
#include "contractor/query_graph.hpp"

#include <cstddef>
#include <limits>
#include <vector>

namespace osrm::contractor
{

using GraphAndFilter = std::tuple<QueryGraph, std::vector<std::vector<bool>>>;

GraphAndFilter contractFullGraph(ContractorGraph contractor_graph);

GraphAndFilter contractExcludableGraph(ContractorGraph contractor_graph_,
                                       const std::vector<std::vector<bool>> &filters);

// Number of edge slots an edge index can address. The edge list cannot grow beyond it.
constexpr std::size_t EDGE_LIST_LIMIT = std::numeric_limits<ContractorGraph::EdgeIterator>::max();

// Number of slots contraction leaves free when it compacts. Compaction cannot run inside the
// parallel sections, so the trigger has to sit far enough below the limit to absorb one whole
// iteration's growth.
constexpr std::size_t EDGE_LIST_COMPACTION_SLACK = 300000000;

// Number of edge slots at which the edge list is compacted. Lowering it trades contraction time
// for a smaller peak edge list.
constexpr std::size_t DEFAULT_EDGE_LIST_COMPACTION_THRESHOLD =
    EDGE_LIST_LIMIT - EDGE_LIST_COMPACTION_SLACK;

std::vector<bool>
contractGraph(ContractorGraph &graph,
              std::vector<bool> node_is_uncontracted,
              std::vector<bool> node_is_contractible,
              double core_factor = 1.0,
              std::size_t edge_list_compaction_threshold = DEFAULT_EDGE_LIST_COMPACTION_THRESHOLD);

// Overload for contracting all nodes
inline auto contractGraph(ContractorGraph &graph, double core_factor = 1.0)
{ return contractGraph(graph, {}, {}, core_factor); }

// Overload no contracted nodes
inline auto contractGraph(ContractorGraph &graph,
                          std::vector<bool> node_is_contractible,
                          double core_factor = 1.0)
{ return contractGraph(graph, {}, std::move(node_is_contractible), core_factor); }

} // namespace osrm::contractor

#endif // OSRM_CONTRACTOR_GRAPH_CONTRACTOR_HPP
