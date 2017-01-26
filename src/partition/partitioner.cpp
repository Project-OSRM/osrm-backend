#include "partition/partitioner.hpp"
#include "partition/bisection_graph.hpp"
#include "partition/recursive_bisection.hpp"
#include "storage/io.hpp"
#include "util/coordinate.hpp"
#include "util/log.hpp"

#include <iterator>
#include <vector>

#include <boost/assert.hpp>

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
    auto compressed_node_based_graph =
        LoadCompressedNodeBasedGraph(config.compressed_node_based_graph_path.string());

    util::Log() << "Loaded compressed node based graph: "
                << compressed_node_based_graph.edges.size() << " edges, "
                << compressed_node_based_graph.coordinates.size() << " nodes";

    sortBySourceThenTarget(begin(compressed_node_based_graph.edges),
                           end(compressed_node_based_graph.edges));

    std::vector<BisectionNode> nodes =
        computeNodes(compressed_node_based_graph.coordinates, compressed_node_based_graph.edges);
    std::vector<BisectionEdge> edges = adaptToBisectionEdge(compressed_node_based_graph.edges);

    BisectionGraph graph(nodes, edges);

    RecursiveBisection recursive_bisection(1024, 1.1, 0.25, graph);

    return 0;
}

} // namespace partition
} // namespace osrm
