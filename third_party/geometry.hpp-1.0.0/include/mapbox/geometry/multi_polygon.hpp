#pragma once

// mapbox
#include <mapbox/geometry/polygon.hpp>
// stl
#include <vector>

namespace mapbox {
namespace geometry {

template <typename T, template <typename...> class Cont = std::vector>
struct multi_polygon : Cont<polygon<T>>
{
    using coordinate_type = T;
    using polygon_type = polygon<T>;
    using container_type = Cont<polygon_type>;
    using size_type = typename container_type::size_type;

    template <class... Args>
    multi_polygon(Args&&... args) : container_type(std::forward<Args>(args)...)
    {
    }
    multi_polygon(std::initializer_list<polygon_type> args)
        : container_type(std::move(args)) {}
};

} // namespace geometry
} // namespace mapbox
