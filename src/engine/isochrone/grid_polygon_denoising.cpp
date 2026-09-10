#include "engine/isochrone/grid_polygon_denoising.hpp"

#include <boost/multiprecision/cpp_int.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace osrm::engine::isochrone
{
namespace
{

using ExactInteger = boost::multiprecision::int256_t;

ExactInteger absoluteTwiceArea(const GridRing &ring)
{
    ExactInteger area = 0;
    for (std::size_t index = 0; index + 1 < ring.size(); ++index)
    {
        area += ExactInteger{ring[index].x} * ring[index + 1].y -
                ExactInteger{ring[index + 1].x} * ring[index].y;
    }
    return area < 0 ? -area : area;
}

bool isValidRing(const GridRing &ring)
{ return ring.size() >= 4 && ring.front() == ring.back() && absoluteTwiceArea(ring) != 0; }

bool isBelowThreshold(const ExactInteger &area,
                      const long double largest_outer_area,
                      const double threshold)
{
    return area.convert_to<long double>() / largest_outer_area <
           static_cast<long double>(threshold);
}

} // namespace

bool denoiseGridPolygons(std::vector<GridPolygon> &polygons, const double threshold)
{
    if (!std::isfinite(threshold) || threshold < 0. || threshold > 1.)
        return false;
    if (threshold == 0. || polygons.empty())
        return true;

    ExactInteger largest_outer_area = 0;
    for (const auto &polygon : polygons)
    {
        if (!isValidRing(polygon.outer))
            return false;
        largest_outer_area = std::max(largest_outer_area, absoluteTwiceArea(polygon.outer));
        if (std::any_of(polygon.holes.begin(),
                        polygon.holes.end(),
                        [](const GridRing &hole) { return !isValidRing(hole); }))
        {
            return false;
        }
    }

    const auto largest_area = largest_outer_area.convert_to<long double>();
    std::erase_if(
        polygons,
        [largest_area, threshold](const GridPolygon &polygon)
        { return isBelowThreshold(absoluteTwiceArea(polygon.outer), largest_area, threshold); });
    for (auto &polygon : polygons)
    {
        std::erase_if(
            polygon.holes,
            [largest_area, threshold](const GridRing &hole)
            { return isBelowThreshold(absoluteTwiceArea(hole), largest_area, threshold); });
    }
    return true;
}

} // namespace osrm::engine::isochrone
