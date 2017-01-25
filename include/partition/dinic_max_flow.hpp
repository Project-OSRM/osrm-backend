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
    using PartitionResult = std::vector<bool>;
    using SourceSinkNodes = std::set<NodeID>;
    using LevelGraph = std::unordered_map<NodeID, std::uint32_t>;
    using FlowEdges = std::unordered_set<std::pair<NodeID, NodeID>>;

    PartitionResult operator()(const GraphView &view,
                               const SourceSinkNodes &sink_nodes,
                               const SourceSinkNodes &source_nodes) const;

  private:
    LevelGraph ComputeLevelGraph(const GraphView &view,
                                 const SourceSinkNodes &source_nodes,
                                 const FlowEdges &flow) const;

    void AugmentFlow(FlowEdges &flow,
                     const GraphView &view,
                     const SourceSinkNodes &source_nodes,
                     const SourceSinkNodes &sink_nodes,
                     const LevelGraph &levels) const;

    bool findPath(const NodeID from,
                  std::vector<NodeID> &path,
                  const GraphView &view,
                  const LevelGraph &levels,
                  const FlowEdges &flow,
                  const SourceSinkNodes &sink_nodes) const;
};

} // namespace partition
} // namespace osrm

// Implementation of Dinics [1] algorithm for max-flow/min-cut.
// [1] https://www.cs.bgu.ac.il/~dinitz/D70.pdf

#endif // OSRM_PARTITION_DINIC_MAX_FLOW_HPP_
