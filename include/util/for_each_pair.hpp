#ifndef FOR_EACH_PAIR_HPP
#define FOR_EACH_PAIR_HPP

#include <iterator>
#include <numeric>

namespace osrm
{
namespace util
{

// TODO: check why this is not an option here:
// std::adjacent_find(begin, end, [=](const auto& l, const auto& r){ return function(), false; });
template <typename ForwardIterator, typename Function>
Function for_each_pair(ForwardIterator begin, ForwardIterator end, Function function)
{
    if (begin == end)
    {
        return function;
    }

    auto next = begin;
    next = std::next(next);

    while (next != end)
    {
        function(*begin, *next);
        begin = std::next(begin);
        next = std::next(next);
    }
    return function;
}

template <class ContainerT, typename Function>
Function for_each_pair(ContainerT &container, Function function)
{
    using std::begin;
    using std::end;
    return for_each_pair(begin(container), end(container), function);
}

} // namespace util
} // namespace osrm

#endif /* FOR_EACH_PAIR_HPP */
