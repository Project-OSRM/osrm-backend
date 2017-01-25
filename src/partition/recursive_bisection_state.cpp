#include "partition/recursive_bisection_state.hpp"

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
    bisection_ids.resize(bisection_graph.GetNumberOfNodes(), BisectionID{0});
}

RecursiveBisectionState::~RecursiveBisectionState()
{
    std::cout << "Internal Result\n";
    std::cout << "IDArray:";
    for (auto id : id_array)
        std::cout << " " << id;
    std::cout << std::endl;

    std::cout << "BisectionIDs:";
    for (auto id : bisection_ids)
        std::cout << " " << (std::bitset<4>(id));

    std::cout << std::endl;
}

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

RecursiveBisectionState::IDIterator RecursiveBisectionState::ApplyBisection(
    const IDIterator begin, const IDIterator end, const std::vector<bool> &partition)
{
    // augment the partition ids
    for (auto itr = begin; itr != end; ++itr)
    {
        bisection_ids[*itr] <<= 1;
        bisection_ids[*itr] |= partition[std::distance(begin, itr)];
    }

    auto first = id_array.begin() + std::distance(id_array.cbegin(), begin);
    auto last = id_array.begin() + std::distance(id_array.cbegin(), end);

    // Keep items with `0` as partition id to the left, move other to the right
    auto by_last_bit = [this](const auto nid) {
        return BisectionID{0} == (bisection_ids[nid] & 1);
    };

    return std::stable_partition(first, last, by_last_bit);
}

} // namespace partition
} // namespace osrm
