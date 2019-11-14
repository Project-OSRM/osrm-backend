#pragma once

// mapbox
#include <mapbox/geometry/point.hpp>

// stl
#include <vector>

namespace mapbox {
namespace geometry {

template <typename T, template <typename...> class Cont = std::vector>
struct linear_ring : Cont<point<T>>
{
    using coordinate_type = T;
    using point_type = point<T>;
    using container_type = Cont<point_type>;
    using size_type = typename container_type::size_type;

    template <class... Args>
    linear_ring(Args&&... args) : container_type(std::forward<Args>(args)...)
    {
    }
    linear_ring(std::initializer_list<point_type> args)
        : container_type(std::move(args)) {}
};

template <typename T, template <typename...> class Cont = std::vector>
struct polygon : Cont<linear_ring<T>>
{
    using coordinate_type = T;
    using linear_ring_type = linear_ring<T>;
    using container_type = Cont<linear_ring_type>;
    using size_type = typename container_type::size_type;

    template <class... Args>
    polygon(Args&&... args) : container_type(std::forward<Args>(args)...)
    {
    }
    polygon(std::initializer_list<linear_ring_type> args)
        : container_type(std::move(args)) {}
};

} // namespace geometry
} // namespace mapbox
