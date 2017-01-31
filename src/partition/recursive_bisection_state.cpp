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
    auto by_flag_bit = [this, flag](const auto node) {
        return BisectionID{0} == (bisection_ids[node.original_id] & flag);
    };

    auto begin = bisection_graph.Begin() + std::distance(bisection_graph.CBegin(), const_begin);
    const auto end = begin + std::distance(const_begin, const_end);

    auto center = std::stable_partition(begin, end, by_flag_bit);

    return const_begin + std::distance(begin, center);
}

} // namespace partition
} // namespace osrm
