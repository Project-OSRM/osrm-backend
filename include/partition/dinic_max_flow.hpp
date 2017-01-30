#ifndef OSRM_PARTITION_DINIC_MAX_FLOW_HPP_
#define OSRM_PARTITION_DINIC_MAX_FLOW_HPP_

#include "partition/graph_view.hpp"

#include <cstdint>
#include <functional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace std
{
template <> struct hash<std::pair<NodeID, NodeID>>
{
    std::size_t operator()(const std::pair<NodeID, NodeID> &flow_edge) const
    {
        std::size_t combined = (static_cast<std::size_t>(flow_edge.first) << 32) | flow_edge.second;
        return std::hash<std::size_t>()(combined);
    }
};
}

namespace osrm
{
namespace partition
{

class DinicMaxFlow
{
  public:
    using Level = std::uint32_t;
    using MinCut = struct
    {
        std::size_t num_nodes_source;
        std::size_t num_edges;
        std::vector<bool> flags;
    };
    using SourceSinkNodes = std::unordered_set<NodeID>;
    using LevelGraph = std::vector<Level>;
    using FlowEdges = std::vector<std::set<NodeID>>;

    MinCut operator()(const GraphView &view,
                      const SourceSinkNodes &sink_nodes,
                      const SourceSinkNodes &source_nodes) const;

  private:
    LevelGraph ComputeLevelGraph(const GraphView &view,
                                 const std::vector<NodeID> &border_source_nodes,
                                 const SourceSinkNodes &source_nodes,
                                 const SourceSinkNodes &sink_nodes,
                                 const FlowEdges &flow) const;

    std::uint32_t BlockingFlow(FlowEdges &flow,
                               LevelGraph &levels,
                               const GraphView &view,
                               const SourceSinkNodes &source_nodes,
                               const std::vector<NodeID> &border_sink_nodes) const;

    std::vector<NodeID> GetAugmentingPath(LevelGraph &levels,
                                          const NodeID from,
                                          const GraphView &view,
                                          const FlowEdges &flow,
                                          const SourceSinkNodes &sink_nodes) const;

    // Builds an actual cut result from a level graph
    MinCut MakeCut(const GraphView &view, const LevelGraph &levels) const;
};

} // namespace partition
} // namespace osrm

// Implementation of Dinics [1] algorithm for max-flow/min-cut.
// [1] https://www.cs.bgu.ac.il/~dinitz/D70.pdf

#endif // OSRM_PARTITION_DINIC_MAX_FLOW_HPP_
