#include "partitioner/renumber.hpp"

#include "util/permutation.hpp"

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
    std::vector<std::uint32_t> ordering(graph.GetNumberOfNodes());
    std::iota(ordering.begin(), ordering.end(), 0);

    // Sort the nodes by cell ID recursively:
    // Nodes in the same cell will be sorted by cell ID on the level below
    for (const auto &partition : partitions)
    {
        std::stable_sort(ordering.begin(),
                         ordering.end(),
                         [&partition](const auto lhs, const auto rhs)
                         { return partition[lhs] < partition[rhs]; });
    }

    // Now sort the nodes by the level at which they are a border node, descening.
    // That means nodes that are border nodes on the highest level will have a very low ID,
    // whereas nodes that are nerver border nodes are sorted to the end of the array.
    // Note: Since we use a stable sort that preserves the cell sorting within each level
    auto border_level = getHighestBorderLevel(graph, partitions);
    std::stable_sort(ordering.begin(),
                     ordering.end(),
                     [&border_level](const auto lhs, const auto rhs)
                     { return border_level[lhs] > border_level[rhs]; });

    return util::orderingToPermutation(ordering);
}
} // namespace osrm::partitioner
