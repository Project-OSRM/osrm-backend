#include "partition/partitioner.hpp"
#include "partition/bisection_graph.hpp"
#include "partition/recursive_bisection.hpp"
#include "storage/io.hpp"
#include "util/coordinate.hpp"
#include "util/log.hpp"

#include <iterator>
#include <vector>

#include <boost/assert.hpp>

#include <tbb/task_scheduler_init.h>

namespace osrm
{
namespace partition
{

struct CompressedNodeBasedGraphEdge
{
    NodeID source;
    NodeID target;
};

struct CompressedNodeBasedGraph
{
    CompressedNodeBasedGraph(storage::io::FileReader &reader)
    {
        // Reads:  | Fingerprint | #e | #n | edges | coordinates |
        // - uint64: number of edges (from, to) pairs
        // - uint64: number of nodes and therefore also coordinates
        // - (uint32_t, uint32_t): num_edges * edges
        // - (int32_t, int32_t: num_nodes * coordinates (lon, lat)

        const auto num_edges = reader.ReadElementCount64();
        const auto num_nodes = reader.ReadElementCount64();

        edges.resize(num_edges);
        coordinates.resize(num_nodes);

        reader.ReadInto(edges);
        reader.ReadInto(coordinates);
    }

    std::vector<CompressedNodeBasedGraphEdge> edges;
    std::vector<util::Coordinate> coordinates;
};

CompressedNodeBasedGraph LoadCompressedNodeBasedGraph(const std::string &path)
{
    const auto fingerprint = storage::io::FileReader::VerifyFingerprint;

    storage::io::FileReader reader(path, fingerprint);

    CompressedNodeBasedGraph graph{reader};
    return graph;
}

int Partitioner::Run(const PartitionConfig &config)
{
    const unsigned recommended_num_threads = tbb::task_scheduler_init::default_num_threads();
    const auto number_of_threads = std::min(recommended_num_threads, config.requested_num_threads);
    tbb::task_scheduler_init init(number_of_threads);

    auto compressed_node_based_graph =
        LoadCompressedNodeBasedGraph(config.compressed_node_based_graph_path.string());

    util::Log() << "Loaded compressed node based graph: "
                << compressed_node_based_graph.edges.size() << " edges, "
                << compressed_node_based_graph.coordinates.size() << " nodes";

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
