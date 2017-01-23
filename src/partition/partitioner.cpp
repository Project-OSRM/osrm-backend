#include "partition/partitioner.hpp"
#include "partition/bisection_graph.hpp"

#include "partition/recursive_bisection.hpp"

// TODO remove after testing
#include "util/coordinate.hpp"

namespace osrm
{
namespace partition
{

int Partitioner::Run(const PartitionConfig &config)
{
    struct TestEdge
    {
        NodeID source;
        NodeID target;
    };

    std::vector<TestEdge> input_edges;

    // 0 - 1 - 2 - 3
    // | \     |   |
    // 4 - 5 - 6 - 7

    input_edges.push_back({0, 1});
    input_edges.push_back({0, 4});
    input_edges.push_back({0, 5});

    input_edges.push_back({1, 0});
    input_edges.push_back({1, 2});

    input_edges.push_back({2, 1});
    input_edges.push_back({2, 3});
    input_edges.push_back({2, 6});

    input_edges.push_back({3, 2});
    input_edges.push_back({3, 7});

    input_edges.push_back({4, 0});
    input_edges.push_back({4, 5});

    input_edges.push_back({5, 0});
    input_edges.push_back({5, 4});
    input_edges.push_back({5, 6});

    input_edges.push_back({6, 2});
    input_edges.push_back({6, 5});
    input_edges.push_back({6, 7});

    input_edges.push_back({7, 3});
    input_edges.push_back({7, 6});

    input_edges = groupBySource(std::move(input_edges));

    std::vector<util::Coordinate> coordinates;

    for (std::size_t i = 0; i < 8; ++i)
    {
        coordinates.push_back(
            {util::FloatLongitude{(i % 4) / 4.0}, util::FloatLatitude{(double)(i / 4)}});
    }

    // do the partitioning
    std::vector<BisectionNode> nodes = computeNodes(coordinates, input_edges);
    std::vector<BisectionEdge> edges = adaptToBisectionEdge(input_edges);

    BisectionGraph graph(nodes, edges);

    RecursiveBisection recursive_bisection(1024, 1.1, 0.25, graph);

    return 0;
}

} // namespace partition
} // namespace osrm
