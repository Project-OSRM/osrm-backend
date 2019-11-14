#pragma once

// mapbox
#include <mapbox/geometry/line_string.hpp>
// stl
#include <vector>

namespace mapbox {
namespace geometry {

template <typename T, template <typename...> class Cont = std::vector>
struct multi_line_string : Cont<line_string<T>>
{
    using coordinate_type = T;
    using line_string_type = line_string<T>;
    using container_type = Cont<line_string_type>;
    using size_type = typename container_type::size_type;

    template <class... Args>
    multi_line_string(Args&&... args) : container_type(std::forward<Args>(args)...)
    {
    }
    multi_line_string(std::initializer_list<line_string_type> args)
        : container_type(std::move(args)) {}
};

} // namespace geometry
} // namespace mapbox
