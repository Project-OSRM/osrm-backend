#ifndef OSRM_PARTITIONER_DINIC_MAX_FLOW_HPP_
#define OSRM_PARTITIONER_DINIC_MAX_FLOW_HPP_

#include "partitioner/bisection_graph_view.hpp"

#include <cstdint>
#include <functional>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

namespace osrm::partitioner
{

class DinicMaxFlow
{
  public:
    // maximal number of hops in the graph from source to sink
    using Level = std::uint32_t;

    using MinCut = struct
    {
        std::size_t num_nodes_source;
        std::size_t num_edges;
        std::vector<bool> flags;
    };

    // input parameter storing the set o
    using SourceSinkNodes = std::unordered_set<NodeID>;

    MinCut operator()(const BisectionGraphView &view,
                      const SourceSinkNodes &source_nodes,
                      const SourceSinkNodes &sink_nodes) const;

    // validates the inpiut parameters to the flow algorithm (e.g. not intersecting)
    bool Validate(const BisectionGraphView &view,
                  const SourceSinkNodes &source_nodes,
                  const SourceSinkNodes &sink_nodes) const;

  private:
    // the level of each node in the graph (==hops in BFS from source)
    using LevelGraph = std::vector<Level>;

    // this is actually faster than using an unordered_set<Edge>, stores all edges that have
    // capacity grouped by node
    using FlowEdges = std::vector<std::set<NodeID>>;

    // The level graph (see [1]) is based on a BFS computation. We assign a level to all nodes
    // (starting with 0 for all source nodes) and assign the hop distance in the residual graph as
    // the level of the node.
    //    a
    //  /   \ 
    // s     t
    //  \   /
    //    b
    // would assign s = 0, a,b = 1, t=2
    LevelGraph ComputeLevelGraph(const BisectionGraphView &view,
                                 const std::vector<NodeID> &border_source_nodes,
                                 const SourceSinkNodes &source_nodes,
                                 const SourceSinkNodes &sink_nodes,
                                 const FlowEdges &flow) const;

    // Using the above levels (see ComputeLevelGraph), we can use multiple DFS (that can now be
    // directed at the sink) to find a flow that completely blocks the level graph (i.e. no path
    // with increasing level exists from `s` to `t`).
    std::size_t BlockingFlow(FlowEdges &flow,
                             LevelGraph &levels,
                             const BisectionGraphView &view,
                             const SourceSinkNodes &source_nodes,
                             const std::vector<NodeID> &border_sink_nodes) const;

    // Finds a single augmenting path from a node to the sink side following levels in the level
    // graph. We don't actually remove the edges, so we have to check for increasing level values.
    // Since we know which sinks have been reached, we actually search for these paths starting at
    // sink nodes, instead of the source, so we can save a few dfs runs
    std::vector<NodeID> GetAugmentingPath(LevelGraph &levels,
                                          const NodeID from,
                                          const BisectionGraphView &view,
                                          const FlowEdges &flow,
                                          const SourceSinkNodes &source_nodes) const;

    // Builds an actual cut result from a level graph
    MinCut MakeCut(const BisectionGraphView &view,
                   const LevelGraph &levels,
                   const std::size_t flow_value) const;
};

} // namespace osrm::partitioner

// Implementation of Dinics [1] algorithm for max-flow/min-cut.
// [1] https://www.cs.bgu.ac.il/~dinitz/D70.pdf

#endif // OSRM_PARTITIONER_DINIC_MAX_FLOW_HPP_
