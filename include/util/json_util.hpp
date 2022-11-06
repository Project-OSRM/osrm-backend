#ifndef JSON_UTIL_HPP
#define JSON_UTIL_HPP

#include "osrm/json_container.hpp"

#include <cmath>
#include <limits>

namespace osrm
{
namespace util
{
namespace json
{
// Make sure we don't have inf and NaN values
template <typename T> T clamp_float(T d)
{
    if (std::isnan(d) || std::numeric_limits<T>::infinity() == d)
    {
        return std::numeric_limits<T>::max();
    }
    if (-std::numeric_limits<T>::infinity() == d)
    {
        return std::numeric_limits<T>::lowest();
    }

    return d;
}
} // namespace json
} // namespace util
} // namespace osrm

#endif // JSON_UTIL_HPP
