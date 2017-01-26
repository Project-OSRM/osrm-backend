#ifndef OSRM_PARTITION_INERTIAL_FLOW_HPP_
#define OSRM_PARTITION_INERTIAL_FLOW_HPP_

#include "partition/dinic_max_flow.hpp"
#include "partition/graph_view.hpp"

#include <unordered_set>
#include <vector>

namespace osrm
{
namespace partition
{

class InertialFlow
{
  public:
    InertialFlow(const GraphView &view);

    DinicMaxFlow::MinCut ComputePartition(const std::size_t num_slopes,
                                          const double balance,
                                          const double source_sink_rate);

  private:
    // Spatially ordered sources and sink ids.
    // The node ids refer to nodes in the GraphView.
    struct SpatialOrder
    {
        std::unordered_set<NodeID> sources;
        std::unordered_set<NodeID> sinks;
    };

    // Creates a spatial order of n * sources "first" and n * sink "last" node ids.
    // The slope determines the spatial order for sorting node coordinates.
    SpatialOrder MakeSpatialOrder(double ratio, double slope) const;

    // Makes n cuts with different spatial orders and returns the best.
    DinicMaxFlow::MinCut BestMinCut(std::size_t n, double ratio) const;

    // The subgraph to partition into two parts.
    const GraphView &view;
};

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_INERTIAL_FLOW_HPP_
