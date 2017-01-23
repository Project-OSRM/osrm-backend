#include "partition/inertial_flow.hpp"

namespace osrm
{
namespace partition
{

InertialFlow::InertialFlow(const GraphView &view_) : view(view_) {}

std::vector<bool> InertialFlow::ComputePartition(const double balance, const double source_sink_rate)
{
    std::vector<bool> partition(view.NumberOfNodes());
    std::size_t i = 0;
    for( auto itr = partition.begin(); itr != partition.end(); ++itr )
    {
        *itr = (i++ % 2) != 0;
    }
    return partition;
}

} // namespace partition
} // namespace osrm
