#ifndef GEOMETRY_COMPRESSOR_HPP
#define GEOMETRY_COMPRESSOR_HPP

#include "extractor/scripting_environment.hpp"
#include "util/typedefs.hpp"

#include "traffic_flow_control_nodes.hpp"
#include "util/node_based_graph.hpp"

#include <memory>
#include <unordered_set>
#include <vector>

namespace osrm
{
namespace extractor
{

class CompressedEdgeContainer;
struct TurnRestriction;
struct UnresolvedManeuverOverride;

class GraphCompressor
{
    using EdgeData = util::NodeBasedDynamicGraph::EdgeData;

  public:
    void Compress(const std::unordered_set<NodeID> &barrier_nodes,
                  const TrafficFlowControlNodes &traffic_signals,
                  const TrafficFlowControlNodes &stop_signs,
                  const TrafficFlowControlNodes &give_way_signs,
                  ScriptingEnvironment &scripting_environment,
                  std::vector<TurnRestriction> &turn_restrictions,
                  std::vector<UnresolvedManeuverOverride> &maneuver_overrides,
                  util::NodeBasedDynamicGraph &graph,
                  const std::vector<NodeBasedEdgeAnnotation> &node_data_container,
                  CompressedEdgeContainer &geometry_compressor);

  private:
    void PrintStatistics(unsigned original_number_of_nodes,
                         unsigned original_number_of_edges,
                         const util::NodeBasedDynamicGraph &graph) const;
};
} // namespace extractor
} // namespace osrm

#endif
