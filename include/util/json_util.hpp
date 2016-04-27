#ifndef JSON_UTIL_HPP
#define JSON_UTIL_HPP

#include "osrm/json_container.hpp"
#include "util/container.hpp"

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

template <typename... Args> Array make_array(Args... args)
{
    Array a;
    // TODO: check why a.values.emplace_back(args...); is not an option here
    append_to_container(a.values, args...);
    return a;
}

// Easy acces to object hierachies
inline Value &get(Value &value) { return value; }

template <typename... Keys> Value &get(Value &value, const char *key, Keys... keys)
{
    using recursive_object_t = mapbox::util::recursive_wrapper<Object>;
    return get(value.get<recursive_object_t>().get().values[key], keys...);
}

template <typename... Keys> Value &get(Value &value, unsigned key, Keys... keys)
{
    using recursive_array_t = mapbox::util::recursive_wrapper<Array>;
    return get(value.get<recursive_array_t>().get().values[key], keys...);
}

} // namespace json
} // namespace util
} // namespace osrm

#endif // JSON_UTIL_HPP
