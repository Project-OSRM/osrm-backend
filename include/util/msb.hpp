#ifndef OSRM_UTIL_MSB_HPP
#define OSRM_UTIL_MSB_HPP

#include <boost/assert.hpp>

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

#if (defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)) && __x86_64__
inline std::size_t msb(std::uint64_t v)
{
    BOOST_ASSERT(v > 0);
    return 63UL - __builtin_clzl(v);
}
inline std::size_t msb(std::uint32_t v)
{
    BOOST_ASSERT(v > 0);
    return 31UL - __builtin_clz(v);
}
#endif
}
}

#endif
