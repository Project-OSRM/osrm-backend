#ifndef OSRM_UTIL_PERMUTATION_HPP
#define OSRM_UTIL_PERMUTATION_HPP

#include "util/integer_range.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <concepts>
#include <numeric>
#include <vector>

namespace osrm::util
{

namespace permutation_detail
{
template <typename T> static inline void swap(T &a, T &b) noexcept { std::swap(a, b); }

static inline void swap(std::vector<bool>::reference a, std::vector<bool>::reference b) noexcept
{ std::vector<bool>::swap(a, b); }
} // namespace permutation_detail

template <typename RandomAccessIterator, typename IndexT>
void inplacePermutation(const RandomAccessIterator &begin,
                        RandomAccessIterator end,
                        const std::vector<IndexT> &old_to_new)
{

    std::size_t size = std::distance(begin, std::move(end));
    BOOST_ASSERT(old_to_new.size() == size);
    // we need a little bit auxililary space since we need to mark
    // replaced elements in a non-destructive way
    std::vector<bool> was_replaced(size, false);
    for (auto index : util::irange<IndexT>(0, size))
    {
        if (was_replaced[index])
        {
            continue;
        }

        if (old_to_new[index] == index)
        {
            was_replaced[index] = true;
            continue;
        }

        // iterate over a cycle in the permutation
        auto buffer = begin[index];
        auto old_index = index;
        auto new_index = old_to_new[old_index];
        for (; new_index != index; old_index = new_index, new_index = old_to_new[new_index])
        {
            was_replaced[old_index] = true;
            permutation_detail::swap(buffer, begin[new_index]);
        }
        was_replaced[old_index] = true;
        permutation_detail::swap(buffer, begin[index]);
    }
}

// Stably reorder node indices using a dense integer key for each node.
template <typename Index, std::unsigned_integral Key>
void stableOrderByKey(std::vector<Index> &ordering,
                      std::vector<Index> &scratch,
                      const std::vector<Key> &keys,
                      const bool descending = false)
{
    BOOST_ASSERT(keys.size() == ordering.size() && scratch.size() == ordering.size());
    if (keys.empty())
        return;

    const auto max_key = *std::max_element(keys.begin(), keys.end());
    if (max_key == 0)
        return;

    // Keep the histogram bounded even for sparse identifiers.
    if (max_key >= keys.size())
    {
        std::stable_sort(ordering.begin(),
                         ordering.end(),
                         [&](const auto lhs, const auto rhs)
                         { return descending ? keys[lhs] > keys[rhs] : keys[lhs] < keys[rhs]; });
        return;
    }

    std::vector<std::size_t> offsets(static_cast<std::size_t>(max_key) + 1, 0);
    for (const auto key : keys)
        ++offsets[key];
    if (descending)
        std::exclusive_scan(offsets.rbegin(), offsets.rend(), offsets.rbegin(), std::size_t{0});
    else
        std::exclusive_scan(offsets.begin(), offsets.end(), offsets.begin(), std::size_t{0});

    for (const auto node : ordering)
        scratch[offsets[keys[node]]++] = node;
    ordering.swap(scratch);
}

template <typename IndexT>
std::vector<IndexT> orderingToPermutation(const std::vector<IndexT> &ordering)
{
    std::vector<IndexT> permutation(ordering.size());
    for (auto index : util::irange<IndexT>(0, ordering.size()))
        permutation[ordering[index]] = index;

    return permutation;
}
} // namespace osrm::util

#endif
