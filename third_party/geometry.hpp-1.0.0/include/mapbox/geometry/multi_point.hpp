#pragma once

// mapbox
#include <mapbox/geometry/point.hpp>
// stl
#include <vector>

namespace mapbox {
namespace geometry {

template <typename T, template <typename...> class Cont = std::vector>
struct multi_point : Cont<point<T>>
{
    using coordinate_type = T;
    using point_type = point<T>;
    using container_type = Cont<point_type>;
    using size_type = typename container_type::size_type;

    template <class... Args>
    multi_point(Args&&... args) : container_type(std::forward<Args>(args)...)
    {
    }
    multi_point(std::initializer_list<point_type> args)
        : container_type(std::move(args)) {}
};

} // namespace geometry
} // namespace mapbox
