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
    using container_type::container_type;
};

} // namespace geometry
} // namespace mapbox
