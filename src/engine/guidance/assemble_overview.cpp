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
        for (const auto &coord : leg_geometry.locations)
        {
            south_west.lon = std::min(south_west.lon, coord.lon);
            south_west.lat = std::min(south_west.lat, coord.lat);

            north_east.lon = std::max(north_east.lon, coord.lon);
            north_east.lat = std::max(north_east.lat, coord.lat);
        }
    }

    return util::viewport::getFittedZoom(south_west, north_east);
}
} // namespace

std::vector<util::Coordinate> assembleOverview(const std::vector<LegGeometry> &leg_geometries,
                                               const bool use_simplification)
{
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

    using GeometryIter = decltype(overview_geometry)::const_iterator;

    auto leg_reverse_index = leg_geometries.size();
    const auto insert_without_overlap = [&leg_reverse_index, &overview_geometry](GeometryIter begin,
                                                                                 GeometryIter end) {
        // not the last leg
        if (leg_reverse_index > 1)
        {
            --leg_reverse_index;
            end = std::prev(end);
        }
        overview_geometry.insert(overview_geometry.end(), begin, end);
    };

    if (use_simplification)
    {
        const auto zoom_level = std::min(18u, calculateOverviewZoomLevel(leg_geometries));
        for (const auto &geometry : leg_geometries)
        {
            const auto simplified =
                douglasPeucker(geometry.locations.begin(), geometry.locations.end(), zoom_level);
            insert_without_overlap(simplified.begin(), simplified.end());
        }
    }
    else
    {
        for (const auto &geometry : leg_geometries)
        {
            insert_without_overlap(geometry.locations.begin(), geometry.locations.end());
        }
    }

    return overview_geometry;
}

} // namespace guidance
} // namespace engine
} // namespace osrm
