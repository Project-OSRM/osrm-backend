#ifndef OSRM_CONTRACTOR_CONTRACT_EXCLUDABLE_GRAPH_HPP
#define OSRM_CONTRACTOR_CONTRACT_EXCLUDABLE_GRAPH_HPP

#include "contractor/contracted_edge_container.hpp"
#include "contractor/contractor_graph.hpp"
#include "contractor/graph_contractor.hpp"
#include "contractor/graph_contractor_adaptors.hpp"
#include "contractor/query_graph.hpp"

namespace osrm
{
namespace contractor
{

using GraphFilterAndCore = std::tuple<QueryGraph, std::vector<std::vector<bool>>>;

inline auto contractExcludableGraph(ContractorGraph contractor_graph_,
                                    std::vector<EdgeWeight> node_weights,
                                    const std::vector<std::vector<bool>> &filters)
{
    auto num_nodes = contractor_graph_.GetNumberOfNodes();
    ContractedEdgeContainer edge_container;
    ContractorGraph shared_core_graph;
    std::vector<bool> is_shared_core;
    {
        ContractorGraph contractor_graph = std::move(contractor_graph_);
        std::vector<bool> always_allowed(num_nodes, true);
        for (const auto &filter : filters)
        {
            for (const auto node : util::irange<NodeID>(0, num_nodes))
            {
                always_allowed[node] = always_allowed[node] && filter[node];
            }
        }

        // By not contracting all contractable nodes we avoid creating
        // a very dense core. This increases the overall graph sizes a little bit
        // but increases the final CH quality and contraction speed.
        constexpr float BASE_CORE = 0.9;
        is_shared_core =
            contractGraph(contractor_graph, std::move(always_allowed), node_weights, BASE_CORE);

        // Add all non-core edges to container
        {
            auto non_core_edges = toEdges<QueryEdge>(contractor_graph);
            auto new_end =
                std::remove_if(non_core_edges.begin(), non_core_edges.end(), [&](const auto &edge) {
                    return is_shared_core[edge.source] && is_shared_core[edge.target];
                });
            non_core_edges.resize(new_end - non_core_edges.begin());
            edge_container.Insert(std::move(non_core_edges));
        }

        // Extract core graph for further contraction
        shared_core_graph = contractor_graph.Filter(
            [&is_shared_core](const NodeID node) { return is_shared_core[node]; });
    }

    for (const auto &filter : filters)
    {
        auto filtered_core_graph =
            shared_core_graph.Filter([&filter](const NodeID node) { return filter[node]; });

        edge_container.Merge(toEdges<QueryEdge>(std::move(filtered_core_graph)));
    }

    return GraphFilterAndCore{QueryGraph{num_nodes, std::move(edge_container.edges)},
                              edge_container.MakeEdgeFilters()};
}
}
}

#endif
