#ifndef OSRM_PARTITION_INERTIAL_FLOW_HPP_
#define OSRM_PARTITION_INERTIAL_FLOW_HPP_

#include "partition/bisection_graph_view.hpp"
#include "partition/dinic_max_flow.hpp"

namespace osrm
{
namespace partition
{

DinicMaxFlow::MinCut computeInertialFlowCut(const BisectionGraphView &view,
                                            const std::size_t num_slopes,
                                            const double balance,
                                            const double source_sink_rate);

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_INERTIAL_FLOW_HPP_
