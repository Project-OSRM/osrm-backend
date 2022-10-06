#ifndef OSRM_UTIL_MSB_HPP
#define OSRM_UTIL_MSB_HPP

#include <boost/assert.hpp>

#include <climits>
#include <cstdint>
#include <utility>

namespace osrm
{
namespace util
{

// get the msb of an integer
// return 0 for integers without msb
template <typename T> std::size_t msb(T value)
{
    static_assert(std::is_integral<T>::value && !std::is_signed<T>::value, "Integer required.");
    std::size_t msb = 0;
    while (value > 0)
    {
        value >>= 1u;
        msb++;
    }
    BOOST_ASSERT(msb > 0);
    return msb - 1;
}

#if (defined(__clang__) || defined(__GNUC__) || defined(__GNUG__))
inline std::size_t msb(unsigned long long v)
{
    BOOST_ASSERT(v > 0);
    constexpr auto MSB_INDEX = CHAR_BIT * sizeof(unsigned long long) - 1;
    return MSB_INDEX - __builtin_clzll(v);
}
inline std::size_t msb(unsigned long v)
{
    BOOST_ASSERT(v > 0);
    constexpr auto MSB_INDEX = CHAR_BIT * sizeof(unsigned long) - 1;
    return MSB_INDEX - __builtin_clzl(v);
}
inline std::size_t msb(unsigned int v)
{
    BOOST_ASSERT(v > 0);
    constexpr auto MSB_INDEX = CHAR_BIT * sizeof(unsigned int) - 1;
    return MSB_INDEX - __builtin_clz(v);
}
#endif
} // namespace util
} // namespace osrm

#endif
