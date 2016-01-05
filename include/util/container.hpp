#ifndef CONTAINER_HPP
#define CONTAINER_HPP

#include <algorithm>
#include <iterator>
#include <vector>

namespace osrm
{
namespace util
{

namespace detail
{
// Culled by SFINAE if reserve does not exist or is not accessible
template <typename T>
constexpr auto has_resize_method(T &t) noexcept -> decltype(t.resize(0), bool())
{
    return true;
}

// Used as fallback when SFINAE culls the template method
constexpr bool has_resize_method(...) noexcept { return false; }
}

template <typename Container> void sort_unique_resize(Container &vector) noexcept
{
    std::sort(std::begin(vector), std::end(vector));
    const auto number_of_unique_elements =
        std::unique(std::begin(vector), std::end(vector)) - std::begin(vector);
    if (detail::has_resize_method(vector))
    {
        vector.resize(number_of_unique_elements);
    }
}

// template <typename T> inline void sort_unique_resize_shrink_vector(std::vector<T> &vector)
// {
//     sort_unique_resize(vector);
//     vector.shrink_to_fit();
// }

// template <typename T> inline void remove_consecutive_duplicates_from_vector(std::vector<T>
// &vector)
// {
//     const auto number_of_unique_elements = std::unique(vector.begin(), vector.end()) -
//     vector.begin();
//     vector.resize(number_of_unique_elements);
// }

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
    return for_each_pair(std::begin(container), std::end(container), function);
}

template <class Container> void append_to_container(Container &&) {}

template <class Container, typename T, typename... Args>
void append_to_container(Container &&container, T value, Args &&... args)
{
    container.emplace_back(value);
    append_to_container(std::forward<Container>(container), std::forward<Args>(args)...);
}

} // namespace osrm
}

#endif /* CONTAINER_HPP */
