#include "partition/inertial_flow.hpp"
#include "partition/dinic_max_flow.hpp"

#include <cstddef>
#include <set>
#include <cmath>
#include <bitset>

namespace osrm
{
namespace partition
{

InertialFlow::InertialFlow(const GraphView &view_) : view(view_) {}

std::vector<bool> InertialFlow::ComputePartition(const double balance, const double source_sink_rate)
{
    std::set<NodeID> sources;
    std::set<NodeID> sinks;

    std::size_t count = std::ceil(source_sink_rate * view.NumberOfNodes());

    auto itr = view.Begin();
    auto itr_end = view.End();
    while(count--)
    {
        --itr_end;
        sinks.insert(*itr_end);
        sources.insert(*itr);
        ++itr;
    }

    std::cout << "Running Flow" << std::endl;
    auto result = DinicMaxFlow()(view,sources,sinks);
    std::cout << "Partition: ";
    for( auto b : result)
        std::cout << b;
    std::cout << std::endl;
    return result;
}

} // namespace partition
} // namespace osrm
