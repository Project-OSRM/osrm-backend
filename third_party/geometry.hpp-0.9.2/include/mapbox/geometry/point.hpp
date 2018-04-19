#pragma once

namespace mapbox {
namespace geometry {

template <typename T>
struct point
{
    using coordinate_type = T;

    constexpr point()
        : x(), y()
    {}
    constexpr point(T x_, T y_)
        : x(x_), y(y_)
    {}

    T x;
    T y;
};

template <typename T>
constexpr bool operator==(point<T> const& lhs, point<T> const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
constexpr bool operator!=(point<T> const& lhs, point<T> const& rhs)
{
    return !(lhs == rhs);
}

} // namespace geometry
} // namespace mapbox
