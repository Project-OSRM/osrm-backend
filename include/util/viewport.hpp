#ifndef UTIL_VIEWPORT_HPP
#define UTIL_VIEWPORT_HPP

#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"

#include <boost/assert.hpp>

#include <cmath>
#include <tuple>

// Port of https://github.com/mapbox/geo-viewport

namespace osrm
{
namespace util
{
namespace viewport
{

namespace detail
{
static constexpr unsigned MAX_ZOOM = 18;
static constexpr unsigned MIN_ZOOM = 1;
// this is an upper bound to current display sizes
static constexpr double VIEWPORT_WIDTH = 8 * coordinate_calculation::mercator::TILE_SIZE;
static constexpr double VIEWPORT_HEIGHT = 5 * coordinate_calculation::mercator::TILE_SIZE;
static double INV_LOG_2 = 1. / std::log(2);
}

unsigned getFittedZoom(util::Coordinate south_west, util::Coordinate north_east)
{
    const auto min_x = coordinate_calculation::mercator::degreeToPixel(toFloating(south_west.lon), detail::MAX_ZOOM);
    const auto max_y = coordinate_calculation::mercator::degreeToPixel(toFloating(south_west.lat), detail::MAX_ZOOM);
    const auto max_x = coordinate_calculation::mercator::degreeToPixel(toFloating(north_east.lon), detail::MAX_ZOOM);
    const auto min_y = coordinate_calculation::mercator::degreeToPixel(toFloating(north_east.lat), detail::MAX_ZOOM);
    const double width_ratio = (max_x - min_x) / detail::VIEWPORT_WIDTH;
    const double height_ratio = (max_y - min_y) / detail::VIEWPORT_HEIGHT;
    const auto zoom = detail::MAX_ZOOM - std::max(std::log(width_ratio), std::log(height_ratio)) * detail::INV_LOG_2;

    if (std::isfinite(zoom))
        return std::max<unsigned>(detail::MIN_ZOOM, zoom);
    else
      return detail::MIN_ZOOM;
}

}
}
}

#endif
