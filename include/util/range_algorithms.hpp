#ifndef RANGE_ALGORITHMS_HPP
#define RANGE_ALGORITHMS_HPP

#include <algorithm>

namespace osrm
{

template <class Container>
auto max_element(const Container &c) -> decltype(std::max_element(c.begin(), c.end()))
{
    return std::max_element(c.begin(), c.end());
}

template <class Container>
auto max_element(const Container &c) -> decltype(std::max_element(c.cbegin(), c.cend()))
{
    return std::max_element(c.cbegin(), c.cend());
}
}

#endif // RANGE_ALGORITHMS_HPP
