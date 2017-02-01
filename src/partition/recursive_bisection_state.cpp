#include "partition/recursive_bisection_state.hpp"

#include <algorithm>
#include <numeric>

// TODO remove
#include <bitset>
#include <iostream>

namespace osrm
{
namespace partition
{

RecursiveBisectionState::RecursiveBisectionState(BisectionGraph &bisection_graph_)
    : bisection_graph(bisection_graph_)
{
    bisection_ids.resize(bisection_graph.NumberOfNodes(), BisectionID{0});
}

RecursiveBisectionState::~RecursiveBisectionState() {}

RecursiveBisectionState::BisectionID
RecursiveBisectionState::GetBisectionID(const NodeID node) const
{
    return bisection_ids[node];
}

RecursiveBisectionState::NodeIterator
RecursiveBisectionState::ApplyBisection(const NodeIterator const_begin,
                                        const NodeIterator const_end,
                                        const std::size_t depth,
                                        const std::vector<bool> &partition)
{
    // augment the partition ids
    const auto flag = BisectionID{1} << depth;
    for (auto itr = const_begin; itr != const_end; ++itr)
    {
        const auto nid = std::distance(const_begin, itr);
        if (partition[nid])
            bisection_ids[itr->original_id] |= flag;
    }

    // Keep items with `0` as partition id to the left, move other to the right
    auto by_flag_bit = [this, flag](const auto &node) {
        return BisectionID{0} == (bisection_ids[node.original_id] & flag);
    };

    auto begin = bisection_graph.Begin() + std::distance(bisection_graph.CBegin(), const_begin);
    const auto end = begin + std::distance(const_begin, const_end);

    // remap the edges
    std::vector<NodeID> mapping(std::distance(const_begin, const_end), SPECIAL_NODEID);
    // calculate a mapping of all node ids
    std::size_t lesser_id = 0, upper_id = 0;
    std::transform(const_begin,
                   const_end,
                   mapping.begin(),
                   [by_flag_bit, &lesser_id, &upper_id](const auto &node) {
                       return by_flag_bit(node) ? lesser_id++ : upper_id++;
                   });

    // erase all edges that point into different partitions
    std::for_each(begin, end, [&](auto &node) {
        const auto node_flag = by_flag_bit(node);
        bisection_graph.RemoveEdges(node, [&](const BisectionGraph::EdgeT &edge) {
            const auto target_flag = by_flag_bit(*(const_begin + edge.target));
            return (node_flag != target_flag);
        });
    });

    auto center = std::stable_partition(begin, end, by_flag_bit);

    // remap all remaining edges
    std::for_each(const_begin, const_end, [&](const auto &node) {
        for (auto &edge : bisection_graph.Edges(node))
            edge.target = mapping[edge.target];
    });

    return const_begin + std::distance(begin, center);
}

const std::vector<RecursiveBisectionState::BisectionID> &
RecursiveBisectionState::BisectionIDs() const
{
    return bisection_ids;
}

} // namespace partition
} // namespace osrm
