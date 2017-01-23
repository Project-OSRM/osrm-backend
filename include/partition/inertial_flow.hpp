#ifndef OSRM_PARTITION_INERTIAL_FLOW_HPP_
#define OSRM_PARTITION_INERTIAL_FLOW_HPP_

#include "partition/graph_view.hpp"
#include <vector>

namespace osrm
{
namespace partition
{

class InertialFlow
{
  public:
    InertialFlow(const GraphView &view);

    std::vector<bool> ComputePartition(const double balance, const double source_sink_rate);
  private:
    const GraphView &view;
};

} // namespace partition
} // namespace osrm

#endif // OSRM_PARTITION_INERTIAL_FLOW_HPP_
