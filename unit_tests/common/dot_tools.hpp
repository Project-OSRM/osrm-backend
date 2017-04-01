#ifndef UNIT_TESTS_DOT_TOOLS_HPP
#define UNIT_TESTS_DOT_TOOLS_HPP

#include "partition/multi_level_graph.hpp"

#include <ostream>

static inline std::string weight_to_string(const EdgeWeight weight)
{
    return weight == INVALID_EDGE_WEIGHT ? "∞" : std::to_string(weight);
}

template <typename EdgeData, osrm::storage::Ownership Ownership>
std::ostream &dotGraph(std::ostream &os,
                       const osrm::partition::MultiLevelPartition &mlp,
                       const osrm::partition::MultiLevelGraph<EdgeData, Ownership> &graph)
{
    os << "digraph MLG {\n";
    for (auto cell : osrm::util::irange<NodeID>(0, mlp.GetNumberOfCells(1)))
    {
        os << "  subgraph cluster" << cell << " {\n  style=filled; color=lightgray; label=\"cell "
           << cell << "\";\n";
        for (auto node : osrm::util::irange<NodeID>(0, graph.GetNumberOfNodes()))
        {
            if (mlp.GetCell(1, node) == cell)
            {
                for (auto edge : graph.GetInternalEdgeRange(1, node))
                {
                    const auto &data = graph.GetEdgeData(edge);
                    if (data.forward)
                    {
                        const auto to = graph.GetTarget(edge);
                        const auto reverse = graph.FindEdge(to, node);
                        const auto has_reverse =
                            reverse != SPECIAL_EDGEID && graph.GetEdgeData(reverse).forward;
                        if (!(has_reverse && to > node))
                        {
                            std::string weight = weight_to_string(data.weight);
                            if (has_reverse && data.weight != graph.GetEdgeData(reverse).weight)
                                weight = ',' + weight_to_string(graph.GetEdgeData(reverse).weight);
                            os << "    " << node << " -> " << to << " "
                               << "[dir=" << (!has_reverse ? "forward" : "none")
                               << ", label=" << weight << "];\n";
                        }
                    }
                }
            }
        }
        os << "  }\n";
    }

    for (auto node : osrm::util::irange<NodeID>(0, graph.GetNumberOfNodes()))
    {
        for (auto edge : graph.GetBorderEdgeRange(1, node))
        {
            const auto &data = graph.GetEdgeData(edge);
            if (data.forward)
            {
                const auto to = graph.GetTarget(edge);
                const auto reverse = graph.FindEdge(to, node);
                const auto has_reverse =
                    reverse != SPECIAL_EDGEID && graph.GetEdgeData(reverse).forward;
                if (!(has_reverse && to > node))
                {
                    std::string weight = weight_to_string(data.weight);
                    if (has_reverse && data.weight != graph.GetEdgeData(reverse).weight)
                        weight = ',' + weight_to_string(graph.GetEdgeData(reverse).weight);
                    os << "  " << node << " -> " << to << " "
                       << "[dir=" << (!has_reverse ? "forward" : "none") << ", label=" << weight
                       << "];\n";
                }
            }
        }
    }
    return os << "}\n";
}

template <typename EdgeData, osrm::storage::Ownership Ownership>
std::ostream &dotGraph(std::ostream &os, const osrm::util::StaticGraph<EdgeData, Ownership> &graph)
{
    os << "digraph StaticGraph {\n";
    for (auto node : osrm::util::irange<NodeID>(0, graph.GetNumberOfNodes()))
    {
        for (auto edge : graph.GetAdjacentEdgeRange(node))
        {
            const auto &data = graph.GetEdgeData(edge);
            os << "  " << node << " -> " << graph.GetTarget(edge)
               << " [label=" << weight_to_string(data.weight) << "];\n";
        }
    }
    return os << "}\n";
}

#endif // UNIT_TESTS_DOT_TOOLS_HPP
