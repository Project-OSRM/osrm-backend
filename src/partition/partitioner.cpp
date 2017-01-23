#include "partition/partitioner.hpp"
#include "partition/bisection_graph.hpp"

#include "partition/recursive_bisection.hpp"

namespace osrm
{
namespace partition
{

int Partitioner::Run(const PartitionConfig &config)
{
    // do the partitioning
    std::vector<BisectionNode> nodes;
    std::vector<BisectionEdge> edges;

    BisectionGraph graph(nodes,edges);

    RecursiveBisection recursive_bisection(graph);

    return 0;
}

} // namespace partition
} // namespace osrm
