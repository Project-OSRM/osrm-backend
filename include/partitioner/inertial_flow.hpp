#ifndef OSRM_PARTITIONER_INERTIAL_FLOW_HPP_
#define OSRM_PARTITIONER_INERTIAL_FLOW_HPP_

#include "partitioner/bisection_graph_view.hpp"
#include "partitioner/dinic_max_flow.hpp"

namespace osrm::partitioner
{

DinicMaxFlow::MinCut computeInertialFlowCut(const BisectionGraphView &view,
                                            const std::size_t num_slopes,
                                            const double balance,
                                            const double source_sink_rate);

} // namespace osrm::partitioner

#endif // OSRM_PARTITIONER_INERTIAL_FLOW_HPP_
