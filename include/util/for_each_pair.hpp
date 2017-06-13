#ifndef FOR_EACH_PAIR_HPP
#define FOR_EACH_PAIR_HPP

#include <iterator>
#include <numeric>
#include <utility>

namespace osrm
{
namespace util
{

// TODO: check why this is not an option here:
// std::adjacent_find(begin, end, [=](const auto& l, const auto& r){ return function(), false; });
template <typename ForwardIterator, typename Function>
void for_each_pair(ForwardIterator begin, ForwardIterator end, Function &&function)
{
    if (begin == end)
    {
        return;
    }

    auto next = begin;
    next = std::next(next);

    while (next != end)
    {
        std::forward<Function>(function)(*begin, *next);
        begin = std::next(begin);
        next = std::next(next);
    }
}

template <class ContainerT, typename Function>
void for_each_pair(ContainerT &container, Function &&function)
{
    using std::begin;
    using std::end;
    for_each_pair(begin(container), end(container), std::forward<Function>(function));
}

} // namespace util
} // namespace osrm

#endif /* FOR_EACH_PAIR_HPP */
