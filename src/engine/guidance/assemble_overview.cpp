#ifndef ENGINE_GUIDANCE_ASSEMBLE_OVERVIEW_HPP
#define ENGINE_GUIDANCE_ASSEMBLE_OVERVIEW_HPP

#include "engine/douglas_peucker.hpp"
#include "engine/guidance/leg_geometry.hpp"
#include "util/viewport.hpp"

#include <iterator>
#include <limits>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{
namespace
{

unsigned calculateOverviewZoomLevel(const std::vector<LegGeometry> &leg_geometries)
{
    util::Coordinate south_west{util::FixedLongitude{std::numeric_limits<int>::max()},
                                util::FixedLatitude{std::numeric_limits<int>::max()}};
    util::Coordinate north_east{util::FixedLongitude{std::numeric_limits<int>::min()},
                                util::FixedLatitude{std::numeric_limits<int>::min()}};

    for (const auto &leg_geometry : leg_geometries)
    {
        for (const auto coord : leg_geometry.locations)
        {
            south_west.lon = std::min(south_west.lon, coord.lon);
            south_west.lat = std::min(south_west.lat, coord.lat);

            north_east.lon = std::max(north_east.lon, coord.lon);
            north_east.lat = std::max(north_east.lat, coord.lat);
        }
    }

    return util::viewport::getFittedZoom(south_west, north_east);
}

std::vector<util::Coordinate> simplifyGeometry(const std::vector<LegGeometry> &leg_geometries,
                                               const unsigned zoom_level)
{
    std::vector<util::Coordinate> overview_geometry;
    auto leg_index = 0UL;
    for (const auto &geometry : leg_geometries)
    {
        auto simplified_geometry =
            douglasPeucker(geometry.locations.begin(), geometry.locations.end(), zoom_level);
        // not the last leg
        if (leg_index < leg_geometries.size() - 1)
        {
            simplified_geometry.pop_back();
        }
        overview_geometry.insert(
            overview_geometry.end(), simplified_geometry.begin(), simplified_geometry.end());
    }
    return overview_geometry;
}
}

std::vector<util::Coordinate> assembleOverview(const std::vector<LegGeometry> &leg_geometries,
                                               const bool use_simplification)
{
    if (use_simplification)
    {
        const auto zoom_level = std::min(18u, calculateOverviewZoomLevel(leg_geometries));
        return simplifyGeometry(leg_geometries, zoom_level);
    }
    BOOST_ASSERT(!use_simplification);

    auto overview_size =
        std::accumulate(leg_geometries.begin(),
                        leg_geometries.end(),
                        0,
                        [](const std::size_t sum, const LegGeometry &leg_geometry) {
                            return sum + leg_geometry.locations.size();
                        }) -
        leg_geometries.size() + 1;
    std::vector<util::Coordinate> overview_geometry;
    overview_geometry.reserve(overview_size);

    auto leg_index = 0UL;
    for (const auto &geometry : leg_geometries)
    {
        auto begin = geometry.locations.begin();
        auto end = geometry.locations.end();
        if (leg_index < leg_geometries.size() - 1)
        {
            end = std::prev(end);
        }
        overview_geometry.insert(overview_geometry.end(), begin, end);
    }

    return overview_geometry;
}

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif
