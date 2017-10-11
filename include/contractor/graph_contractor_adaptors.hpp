#ifndef OSRM_CONTRACTOR_GRAPH_CONTRACTION_ADAPTORS_HPP_
#define OSRM_CONTRACTOR_GRAPH_CONTRACTION_ADAPTORS_HPP_

#include "contractor/contractor_graph.hpp"

#include "util/log.hpp"
#include "util/percent.hpp"

#include <tbb/parallel_sort.h>

#include <vector>

namespace osrm
{
namespace contractor
{

// Make sure to move in the input edge list!
template <typename InputEdgeContainer>
ContractorGraph toContractorGraph(NodeID number_of_nodes, InputEdgeContainer input_edge_list)
{
    std::vector<ContractorEdge> edges;
    edges.reserve(input_edge_list.size() * 2);

    for (const auto &input_edge : input_edge_list)
    {
        if (input_edge.data.weight == INVALID_EDGE_WEIGHT)
            continue;

#ifndef NDEBUG
        const unsigned int constexpr DAY_IN_DECI_SECONDS = 24 * 60 * 60 * 10;
        if (static_cast<unsigned int>(std::max(input_edge.data.weight, 1)) > DAY_IN_DECI_SECONDS)
        {
            util::Log(logWARNING) << "Edge weight large -> "
                                  << static_cast<unsigned int>(std::max(input_edge.data.weight, 1))
                                  << " : " << static_cast<unsigned int>(input_edge.source) << " -> "
                                  << static_cast<unsigned int>(input_edge.target);
        }
#endif
        edges.emplace_back(input_edge.source,
                           input_edge.target,
                           std::max(input_edge.data.weight, 1),
                           input_edge.data.duration,
                           1,
                           input_edge.data.turn_id,
                           false,
                           input_edge.data.forward ? true : false,
                           input_edge.data.backward ? true : false);

        edges.emplace_back(input_edge.target,
                           input_edge.source,
                           std::max(input_edge.data.weight, 1),
                           input_edge.data.duration,
                           1,
                           input_edge.data.turn_id,
                           false,
                           input_edge.data.backward ? true : false,
                           input_edge.data.forward ? true : false);
    };
    tbb::parallel_sort(edges.begin(), edges.end());

    NodeID edge = 0;
    for (NodeID i = 0; i < edges.size();)
    {
        const NodeID source = edges[i].source;
        const NodeID target = edges[i].target;
        const NodeID id = edges[i].data.id;
        // remove eigenloops
        if (source == target)
        {
            ++i;
            continue;
        }
        ContractorEdge forward_edge;
        ContractorEdge reverse_edge;
        forward_edge.source = reverse_edge.source = source;
        forward_edge.target = reverse_edge.target = target;
        forward_edge.data.forward = reverse_edge.data.backward = true;
        forward_edge.data.backward = reverse_edge.data.forward = false;
        forward_edge.data.shortcut = reverse_edge.data.shortcut = false;
        forward_edge.data.id = reverse_edge.data.id = id;
        forward_edge.data.originalEdges = reverse_edge.data.originalEdges = 1;
        forward_edge.data.weight = reverse_edge.data.weight = INVALID_EDGE_WEIGHT;
        forward_edge.data.duration = reverse_edge.data.duration = MAXIMAL_EDGE_DURATION;
        // remove parallel edges
        while (i < edges.size() && edges[i].source == source && edges[i].target == target)
        {
            if (edges[i].data.forward)
            {
                forward_edge.data.weight = std::min(edges[i].data.weight, forward_edge.data.weight);
                forward_edge.data.duration =
                    std::min(edges[i].data.duration, forward_edge.data.duration);
            }
            if (edges[i].data.backward)
            {
                reverse_edge.data.weight = std::min(edges[i].data.weight, reverse_edge.data.weight);
                reverse_edge.data.duration =
                    std::min(edges[i].data.duration, reverse_edge.data.duration);
            }
            ++i;
        }
        // merge edges (s,t) and (t,s) into bidirectional edge
        if (forward_edge.data.weight == reverse_edge.data.weight)
        {
            if ((int)forward_edge.data.weight != INVALID_EDGE_WEIGHT)
            {
                forward_edge.data.backward = true;
                edges[edge++] = forward_edge;
            }
        }
        else
        { // insert seperate edges
            if (((int)forward_edge.data.weight) != INVALID_EDGE_WEIGHT)
            {
                edges[edge++] = forward_edge;
            }
            if ((int)reverse_edge.data.weight != INVALID_EDGE_WEIGHT)
            {
                edges[edge++] = reverse_edge;
            }
        }
    }
    util::Log() << "merged " << edges.size() - edge << " edges out of " << edges.size();
    edges.resize(edge);

    return ContractorGraph{number_of_nodes, edges};
}

template <class Edge, typename GraphT> inline std::vector<Edge> toEdges(GraphT graph)
{
    std::vector<Edge> edges;
    edges.reserve(graph.GetNumberOfEdges());

    util::UnbufferedLog log;
    log << "Getting edges of minimized graph ";
    util::Percent p(log, graph.GetNumberOfNodes());
    const NodeID number_of_nodes = graph.GetNumberOfNodes();
    if (graph.GetNumberOfNodes())
    {
        Edge new_edge;
        for (const auto node : util::irange(0u, number_of_nodes))
        {
            p.PrintStatus(node);
            for (auto edge : graph.GetAdjacentEdgeRange(node))
            {
                const NodeID target = graph.GetTarget(edge);
                const ContractorGraph::EdgeData &data = graph.GetEdgeData(edge);
                new_edge.source = node;
                new_edge.target = target;
                BOOST_ASSERT_MSG(SPECIAL_NODEID != new_edge.target, "Target id invalid");
                new_edge.data.weight = data.weight;
                new_edge.data.duration = data.duration;
                new_edge.data.shortcut = data.shortcut;
                new_edge.data.turn_id = data.id;
                BOOST_ASSERT_MSG(new_edge.data.turn_id != INT_MAX, // 2^31
                                 "edge id invalid");
                new_edge.data.forward = data.forward;
                new_edge.data.backward = data.backward;
                edges.push_back(new_edge);
            }
        }
    }

    // sort and remove duplicates
    tbb::parallel_sort(edges.begin(), edges.end());
    auto new_end = std::unique(edges.begin(), edges.end());
    edges.resize(new_end - edges.begin());
    edges.shrink_to_fit();

    return edges;
}

} // namespace contractor
} // namespace osrm

#endif // OSRM_CONTRACTOR_GRAPH_CONTRACTION_ADAPTORS_HPP_
