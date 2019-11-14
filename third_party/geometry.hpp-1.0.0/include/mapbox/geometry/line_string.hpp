#pragma once

// mapbox
#include <mapbox/geometry/point.hpp>
// stl
#include <vector>

namespace mapbox {
namespace geometry {

template <typename T, template <typename...> class Cont = std::vector>
struct line_string : Cont<point<T>>
{
    using coordinate_type = T;
    using point_type = point<T>;
    using container_type = Cont<point_type>;
    using size_type = typename container_type::size_type;

    template <class... Args>
    line_string(Args&&... args) : container_type(std::forward<Args>(args)...)
    {
    }
    line_string(std::initializer_list<point_type> args)
        : container_type(std::move(args)) {}
};

} // namespace geometry
} // namespace mapbox
