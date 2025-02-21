#ifndef OSRM_UTIL_MSB_HPP
#define OSRM_UTIL_MSB_HPP

#include <bit>
#include <boost/assert.hpp>
#include <cstdint>
#include <limits>

namespace osrm::util
{

template <typename T> std::size_t msb(T value)
{
    BOOST_ASSERT(value > 0);

    static_assert(std::is_integral<T>::value && !std::is_signed<T>::value, "Integer required.");
    constexpr auto MSB_INDEX = std::numeric_limits<unsigned char>::digits * sizeof(T) - 1;

    return MSB_INDEX - std::countl_zero(value);
}

} // namespace osrm::util

#endif
