#ifndef OSRM_REORDER_FIRST_LAST_HPP
#define OSRM_REORDER_FIRST_LAST_HPP

#include <boost/assert.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>

namespace osrm
{
namespace partition
{

// Reorders the first n elements in the range to satisfy the comparator,
// and the last n elements to satisfy the comparator with arguments flipped.
// Note: no guarantees to the element's ordering inside the reordered ranges.
template <typename RandomIt, typename Comparator>
void reorderFirstLast(RandomIt first, RandomIt last, std::size_t n, Comparator comp)
{
    BOOST_ASSERT_MSG(n <= (last - first) / std::size_t{2}, "overlapping subranges not allowed");

    if (n == 0 || (last - first < 2))
        return;

    // Reorder first n: guarantees that the predicate holds for the first elements.
    std::nth_element(first, first + (n - 1), last, comp);

    // Reorder last n: guarantees that the flipped predicate holds for the last k elements.
    // We reorder from the end backwards up to the end of the already reordered range.
    // We can not use std::not2, since then e.g. std::less<> would lose its irreflexive
    // requirements.
    std::reverse_iterator<RandomIt> rfirst{last}, rlast{first + n};

    const auto flipped = [](auto fn) {
        return [fn](auto &&lhs, auto &&rhs) {
            return fn(std::forward<decltype(lhs)>(rhs), std::forward<decltype(rhs)>(lhs));
        };
    };

    std::nth_element(rfirst, rfirst + (n - 1), rlast, flipped(comp));
}

template <typename RandomAccessRange, typename Compare>
void reorderFirstLast(RandomAccessRange &rng, std::size_t n, Compare comp)
{
    using std::begin;
    using std::end;
    return reorderFirstLast(begin(rng), end(rng), n, comp);
}

} // ns partition
} // ns osrm

#endif
