#ifndef OSRM_EDGE_BASED_GRAPH_READER_HPP
#define OSRM_EDGE_BASED_GRAPH_READER_HPP

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

struct EdgeBasedGraphEdgeData
{
    NodeID edge_id : 31;
    // Artificial edge used to fixup partitioning, see #3205.
    // These artificial edges have invalid weight / duration.
    std::uint32_t is_boundary_arc : 1;
    EdgeWeight weight;
    EdgeWeight duration : 30;
    std::uint32_t forward : 1;
    std::uint32_t backward : 1;
};

struct EdgeBasedGraph : util::DynamicGraph<EdgeBasedGraphEdgeData>
{
    using Base = util::DynamicGraph<EdgeBasedGraphEdgeData>;
    using Base::Base;
};

struct EdgeBasedGraphEdge : EdgeBasedGraph::InputEdge
{
    using Base = EdgeBasedGraph::InputEdge;
    using Base::Base;
};

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

    std::unique_ptr<EdgeBasedGraph> BuildEdgeBasedGraph()
    {
        // FIXME: The following is a rough adaption from:
        // - adaptToContractorInput
        // - GraphContractor::GraphContractor
        // and should really be abstracted over.

        auto directed = SplitBidirectionalEdges(edges);
        auto tidied = PrepareEdgesForUsageInGraph(directed);

        return std::make_unique<EdgeBasedGraph>(num_nodes, tidied);
    }

  private:
    // Bidirectional (s,t) to (s,t) and (t,s)
    static std::vector<extractor::EdgeBasedEdge>
    SplitBidirectionalEdges(std::vector<extractor::EdgeBasedEdge> edges)
    {
        std::vector<extractor::EdgeBasedEdge> directed;
        directed.reserve(edges.size() * 2);

        for (const auto &edge : edges)
        {
            directed.emplace_back(edge.source,
                                  edge.target,
                                  edge.edge_id,
                                  std::max(edge.weight, 1),
                                  edge.duration,
                                  edge.forward,
                                  edge.backward);

            directed.emplace_back(edge.target,
                                  edge.source,
                                  edge.edge_id,
                                  std::max(edge.weight, 1),
                                  edge.duration,
                                  edge.backward,
                                  edge.forward);
        }

        std::swap(directed, edges);

        return directed;
    }

    static std::vector<EdgeBasedGraphEdge>
    PrepareEdgesForUsageInGraph(std::vector<extractor::EdgeBasedEdge> edges)
    {
        std::sort(begin(edges), end(edges));

        std::vector<EdgeBasedGraphEdge> graph_edges;
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

            EdgeBasedGraphEdge forward_edge;
            EdgeBasedGraphEdge reverse_edge;
            forward_edge.source = reverse_edge.source = source;
            forward_edge.target = reverse_edge.target = target;
            forward_edge.data.edge_id = reverse_edge.data.edge_id = edges[i].edge_id;
            forward_edge.data.is_boundary_arc = reverse_edge.data.is_boundary_arc = false;
            forward_edge.data.weight = reverse_edge.data.weight = INVALID_EDGE_WEIGHT;
            forward_edge.data.duration = reverse_edge.data.duration = MAXIMAL_EDGE_DURATION;
            forward_edge.data.forward = reverse_edge.data.backward = true;
            forward_edge.data.backward = reverse_edge.data.forward = false;


            // remove parallel edges
            while (i < edges.size() && edges[i].source == source && edges[i].target == target)
            {
                if (edges[i].forward)
                {
                    forward_edge.data.weight = std::min(edges[i].weight, forward_edge.data.weight);
                    forward_edge.data.duration =
                        std::min(edges[i].duration, forward_edge.data.duration);
                }
                if (edges[i].backward)
                {
                    reverse_edge.data.weight = std::min(edges[i].weight, reverse_edge.data.weight);
                    reverse_edge.data.duration =
                        std::min(edges[i].duration, reverse_edge.data.duration);
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

    std::vector<extractor::EdgeBasedEdge> edges;
    std::size_t num_nodes;
};

inline std::unique_ptr<EdgeBasedGraph> LoadEdgeBasedGraph(const std::string &path)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;
    storage::io::FileReader reader(path, fingerprint);

    EdgeBasedGraphReader builder{reader};

    return builder.BuildEdgeBasedGraph();
}

} // ns partition
} // ns osrm

#endif
