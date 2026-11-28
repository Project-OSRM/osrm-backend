#include "partitioner/renumber.hpp"

#include "util/permutation.hpp"

#include <numeric>

namespace osrm::partitioner
{
namespace
{
// Returns a vector that is indexed by node ID marking the level at which it is a border node
std::vector<LevelID> getHighestBorderLevel(const DynamicEdgeBasedGraph &graph,
                                           const std::vector<Partition> &partitions)
{
    std::vector<LevelID> border_level(graph.GetNumberOfNodes(), 0);

    for (const auto level : util::irange<LevelID>(1, partitions.size() + 1))
    {
        const auto &partition = partitions[level - 1];
        for (auto node : util::irange<NodeID>(0, graph.GetNumberOfNodes()))
        {
            for (auto edge : graph.GetAdjacentEdgeRange(node))
            {
                auto target = graph.GetTarget(edge);
                if (partition[node] != partition[target])
                {
                    // level is monotone increasing so we wil
                    // always overwrite here with a value equal
                    // or greater then the current border_level
                    border_level[node] = level;
                    border_level[target] = level;
                }
            }
        }
    }

    return border_level;
}
} // namespace

std::vector<std::uint32_t> makePermutation(const DynamicEdgeBasedGraph &graph,
                                           const std::vector<Partition> &partitions)
{
    std::vector<NodeID> ordering(graph.GetNumberOfNodes());
    std::vector<NodeID> scratch(ordering.size());
    std::iota(ordering.begin(), ordering.end(), 0);

    // Sort the nodes by cell ID recursively:
    // Nodes in the same cell will be sorted by cell ID on the level below
    for (const auto &partition : partitions)
        util::stableOrderByKey(ordering, scratch, partition);

    // Now sort the nodes by the level at which they are a border node, descening.
    // That means nodes that are border nodes on the highest level will have a very low ID,
    // whereas nodes that are nerver border nodes are sorted to the end of the array.
    // Note: Since we use a stable sort that preserves the cell sorting within each level
    auto border_level = getHighestBorderLevel(graph, partitions);
    util::stableOrderByKey(ordering, scratch, border_level, true);

    for (const auto rank : util::irange<NodeID>(0, graph.GetNumberOfNodes()))
        scratch[ordering[rank]] = rank;
    return scratch;
}
} // namespace osrm::partitioner
