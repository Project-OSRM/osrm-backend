#include "partition/partitioner.hpp"
#include "partition/bisection_graph.hpp"
#include "partition/recursive_bisection.hpp"

#include "util/log.hpp"

#include <iterator>
#include <vector>

#include <boost/assert.hpp>

#include <tbb/task_scheduler_init.h>

namespace osrm
{
namespace partition
{

int Partitioner::Run(const PartitionConfig &config)
{
    const unsigned recommended_num_threads = tbb::task_scheduler_init::default_num_threads();
    const auto number_of_threads = std::min(recommended_num_threads, config.requested_num_threads);
    tbb::task_scheduler_init init(number_of_threads);

    auto compressed_node_based_graph =
        LoadCompressedNodeBasedGraph(config.compressed_node_based_graph_path.string());

    groupEdgesBySource(begin(compressed_node_based_graph.edges),
                       end(compressed_node_based_graph.edges));

    auto graph =
        makeBisectionGraph(compressed_node_based_graph.coordinates,
                           adaptToBisectionEdge(std::move(compressed_node_based_graph.edges)));

    RecursiveBisection recursive_bisection(1024, 1.1, 0.25, graph);

    return 0;
}

} // namespace partition
} // namespace osrm
