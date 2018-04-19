#pragma once

#include <mapbox/geometry/box.hpp>
#include <mapbox/geometry/for_each_point.hpp>

#include <limits>

namespace mapbox {
namespace geometry {

template <typename G, typename T = typename G::coordinate_type>
box<T> envelope(G const& geometry)
{
    using limits = std::numeric_limits<T>;

    T min_t = limits::has_infinity ? -limits::infinity() : limits::min();
    T max_t = limits::has_infinity ?  limits::infinity() : limits::max();

    point<T> min(max_t, max_t);
    point<T> max(min_t, min_t);

    for_each_point(geometry, [&] (point<T> const& point) {
        if (min.x > point.x) min.x = point.x;
        if (min.y > point.y) min.y = point.y;
        if (max.x < point.x) max.x = point.x;
        if (max.y < point.y) max.y = point.y;
    });

    return box<T>(min, max);
}

} // namespace geometry
} // namespace mapbox
