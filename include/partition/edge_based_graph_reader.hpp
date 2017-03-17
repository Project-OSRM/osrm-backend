#ifndef OSRM_EDGE_BASED_GRAPH_READER_HPP
#define OSRM_EDGE_BASED_GRAPH_READER_HPP

#include "partition/edge_based_graph.hpp"

#include "extractor/edge_based_edge.hpp"
#include "storage/io.hpp"
#include "util/coordinate.hpp"
#include "util/dynamic_graph.hpp"
#include "util/typedefs.hpp"

#include <cstdint>

#include <algorithm>
#include <iterator>
#include <memory>
#include <vector>

namespace osrm
{
namespace partition
{

// Bidirectional (s,t) to (s,t) and (t,s)
std::vector<extractor::EdgeBasedEdge>
splitBidirectionalEdges(const std::vector<extractor::EdgeBasedEdge> &edges)
{
    std::vector<extractor::EdgeBasedEdge> directed;
    directed.reserve(edges.size() * 2);

    for (const auto &edge : edges)
    {
        if (edge.data.weight == INVALID_EDGE_WEIGHT)
            continue;

        directed.emplace_back(edge.source,
                              edge.target,
                              edge.data.turn_id,
                              std::max(edge.data.weight, 1),
                              edge.data.duration,
                              edge.data.forward,
                              edge.data.backward);

        directed.emplace_back(edge.target,
                              edge.source,
                              edge.data.turn_id,
                              std::max(edge.data.weight, 1),
                              edge.data.duration,
                              edge.data.backward,
                              edge.data.forward);
    }

    return directed;
}

template <typename OutputEdgeT>
std::vector<OutputEdgeT> prepareEdgesForUsageInGraph(std::vector<extractor::EdgeBasedEdge> edges)
{
    std::sort(begin(edges), end(edges));

    std::vector<OutputEdgeT> graph_edges;
    graph_edges.reserve(edges.size());

    for (NodeID i = 0; i < edges.size();)
    {
        const NodeID source = edges[i].source;
        const NodeID target = edges[i].target;

        // remove eigenloops
        if (source == target)
        {
            ++i;
            continue;
        }

        OutputEdgeT forward_edge;
        OutputEdgeT reverse_edge;
        forward_edge.source = reverse_edge.source = source;
        forward_edge.target = reverse_edge.target = target;
        forward_edge.data.turn_id = reverse_edge.data.turn_id = edges[i].data.turn_id;
        forward_edge.data.weight = reverse_edge.data.weight = INVALID_EDGE_WEIGHT;
        forward_edge.data.duration = reverse_edge.data.duration = MAXIMAL_EDGE_DURATION_INT_30;
        forward_edge.data.forward = reverse_edge.data.backward = true;
        forward_edge.data.backward = reverse_edge.data.forward = false;

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
                graph_edges.push_back(forward_edge);
            }
        }
        else
        { // insert seperate edges
            if (((int)forward_edge.data.weight) != INVALID_EDGE_WEIGHT)
            {
                graph_edges.push_back(forward_edge);
            }
            if ((int)reverse_edge.data.weight != INVALID_EDGE_WEIGHT)
            {
                graph_edges.push_back(reverse_edge);
            }
        }
    }

    return graph_edges;
}

struct EdgeBasedGraphReader
{
    EdgeBasedGraphReader(storage::io::FileReader &reader)
    {
        // Reads:  | Fingerprint | #e | max_eid | edges |
        // - uint64: number of edges
        // - EdgeID: max edge id
        // - extractor::EdgeBasedEdge edges
        //
        // Gets written in Extractor::WriteEdgeBasedGraph

        const auto num_edges = reader.ReadElementCount64();
        const auto max_edge_id = reader.ReadOne<EdgeID>();

        num_nodes = max_edge_id + 1;

        edges.resize(num_edges);
        reader.ReadInto(edges);
    }

    // FIXME: wrapped in unique_ptr since dynamic_graph is not move-able

    std::unique_ptr<DynamicEdgeBasedGraph> BuildEdgeBasedGraph()
    {
        // FIXME: The following is a rough adaption from:
        // - adaptToContractorInput
        // - GraphContractor::GraphContractor
        // and should really be abstracted over.
        // FIXME: edges passed as a const reference, can be changed pass-by-value if can be moved

        auto directed = splitBidirectionalEdges(edges);
        auto tidied = prepareEdgesForUsageInGraph<DynamicEdgeBasedGraphEdge>(std::move(directed));

        return std::make_unique<DynamicEdgeBasedGraph>(num_nodes, std::move(tidied));
    }

  private:
    std::vector<extractor::EdgeBasedEdge> edges;
    std::size_t num_nodes;
};

inline std::unique_ptr<DynamicEdgeBasedGraph> LoadEdgeBasedGraph(const std::string &path)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader(path, fingerprint);

    EdgeBasedGraphReader builder{reader};

    return builder.BuildEdgeBasedGraph();
}

} // ns partition
} // ns osrm

#endif
