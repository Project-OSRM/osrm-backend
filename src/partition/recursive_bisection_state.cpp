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
    id_array.resize(bisection_graph.NumberOfNodes());
    std::iota(id_array.begin(), id_array.end(), NodeID{0});
    position_array.resize(bisection_graph.NumberOfNodes());
    std::iota(position_array.begin(), position_array.end(), 0);
    bisection_ids.resize(bisection_graph.NumberOfNodes(), BisectionID{0});
}

RecursiveBisectionState::~RecursiveBisectionState() {}

const RecursiveBisectionState::IDIterator RecursiveBisectionState::Begin() const
{
    return id_array.cbegin();
}

const RecursiveBisectionState::IDIterator RecursiveBisectionState::End() const
{
    return id_array.cend();
}

RecursiveBisectionState::BisectionID
RecursiveBisectionState::GetBisectionID(const NodeIterator node) const
{
    return bisection_ids[node->original_id];
}

RecursiveBisectionState::NodeIterator
RecursiveBisectionState::ApplyBisection(NodeIterator begin,
                                        const NodeIterator end,
                                        const std::size_t depth,
                                        const std::vector<bool> &partition)
{
    // augment the partition ids
    const auto flag = BisectionID{1} << depth;
    for (auto itr = begin; itr != end; ++itr)
    {
        if (partition[std::distance(begin, itr)])
            bisection_ids[itr->original_id] |= flag;
    }

    // Keep items with `0` as partition id to the left, move other to the right
    auto by_flag_bit = [this, flag](const auto node) {
        return BisectionID{0} == (bisection_ids[node.original_id] & flag);
    };

    auto center = std::stable_partition(begin, end, by_flag_bit);

    return center;
}

std::uint32_t RecursiveBisectionState::GetPosition(NodeID nid) const { return position_array[nid]; }

} // namespace partition
} // namespace osrm
