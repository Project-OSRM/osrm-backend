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

RecursiveBisectionState::RecursiveBisectionState(const BisectionGraph &bisection_graph_)
    : bisection_graph(bisection_graph_)
{
    id_array.resize(bisection_graph.GetNumberOfNodes());
    std::iota(id_array.begin(), id_array.end(), NodeID{0});
    position_array.resize(bisection_graph.GetNumberOfNodes());
    std::iota(position_array.begin(), position_array.end(), 0);
    bisection_ids.resize(bisection_graph.GetNumberOfNodes(), BisectionID{0});
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

RecursiveBisectionState::BisectionID RecursiveBisectionState::GetBisectionID(const NodeID nid) const
{
    return bisection_ids[nid];
}

RecursiveBisectionState::IDIterator
RecursiveBisectionState::ApplyBisection(const IDIterator begin,
                                        const IDIterator end,
                                        const std::size_t depth,
                                        const std::vector<bool> &partition)
{
    // augment the partition ids
    const auto flag = BisectionID{1} << depth;
    for (auto itr = begin; itr != end; ++itr)
    {
        if (partition[std::distance(begin, itr)])
            bisection_ids[*itr] |= flag;
    }

    auto first = id_array.begin() + std::distance(id_array.cbegin(), begin);
    auto last = id_array.begin() + std::distance(id_array.cbegin(), end);

    // Keep items with `0` as partition id to the left, move other to the right
    auto by_last_bit = [this](const auto nid) {
        return BisectionID{0} == (bisection_ids[nid] & 1);
    };

    auto center = std::stable_partition(first, last, by_last_bit);

    // remember the new position to any node between begin and end
    std::for_each(
        first,
        last,
        [ this, position = std::distance(id_array.begin(), first) ](const auto node_id) mutable {
            position_array[node_id] = position++;
            BOOST_ASSERT(id_array[position_array[node_id]] == node_id);
        });

    BOOST_ASSERT(position_array[*first] == std::distance(id_array.begin(), first));
    BOOST_ASSERT(position_array[*center] == std::distance(id_array.begin(), center));

    return center;
}

std::uint32_t RecursiveBisectionState::GetPosition(NodeID nid) const { return position_array[nid]; }

} // namespace partition
} // namespace osrm
