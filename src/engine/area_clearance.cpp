#include "engine/area_clearance.hpp"

#include "engine/area_visibility.hpp"

#include "util/coordinate_calculation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace osrm::engine::area
{

Clearance clearance(const Point &point, std::span<const Ring> rings)
{
    Clearance best;
    auto best_squared = std::numeric_limits<double>::infinity();

    for (const auto &ring : rings)
    {
        if (ring.size() < 2)
        {
            continue;
        }

        for (std::size_t i = 0; i < ring.size(); ++i)
        {
            const auto found =
                distance_squared_to_segment(point, ring[i], ring[(i + 1) % ring.size()]);
            best_squared = std::min(best_squared, found);
        }
    }

    if (!std::isfinite(best_squared))
    {
        return {};
    }

    best.distance = std::sqrt(best_squared);
    // Free space is inside the outer ring and outside every obstacle, and which side of
    // the geometry the point is on is settled here rather than left to the caller because
    // the distance is meaningless without it: the two sides of a wall are the same
    // distance from it.
    //
    // The free space is closed, so a point on the boundary is on legal ground.  Asking
    // inside_area() alone would not settle it: it is strict, and ray casting on a point
    // that lies exactly on an edge answers arbitrarily.  The distance to the nearest
    // geometry has just been computed, and a point on the boundary is the one at zero.
    best.inside = best.distance <= ON_GEOMETRY || inside_area(point, rings);
    return best;
}

double metres_per_projected_unit(const double latitude_degrees)
{
    // One degree of longitude at the equator, which is what a projected unit is there.
    // Mercator stretches by 1/cos(latitude), so a projected unit covers correspondingly
    // less ground as one moves away from it.
    namespace detail = util::coordinate_calculation::detail;
    const auto metres_per_degree =
        static_cast<double>(detail::EARTH_RADIUS) * detail::DEGREE_TO_RAD;
    const auto latitude = std::clamp(latitude_degrees, -89.9, 89.9);
    return metres_per_degree * std::cos(latitude * detail::DEGREE_TO_RAD);
}

} // namespace osrm::engine::area
