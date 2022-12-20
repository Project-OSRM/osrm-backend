#ifndef FOR_EACH_INDEXED_HPP
#define FOR_EACH_INDEXED_HPP

#include <iterator>
#include <numeric>
#include <utility>

namespace osrm::util
{

template <typename ForwardIterator, typename Function>
void for_each_indexed(ForwardIterator first, ForwardIterator last, Function function)
{
    for (size_t i = 0; first != last; ++first, ++i)
    {
        function(i, *first);
    }
}

template <class ContainerT, typename Function>
void for_each_indexed(ContainerT &container, Function function)
{
    for_each_indexed(std::begin(container), std::end(container), function);
}

} // namespace osrm::util

#endif /* FOR_EACH_INDEXED_HPP */
