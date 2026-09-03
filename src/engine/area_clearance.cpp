#include "engine/area_clearance.hpp"

#include "engine/area_visibility.hpp"

#include "util/coordinate_calculation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace osrm::engine::area
{

namespace
{

struct Nearest
{
    double distance_squared;
    Point at;
};

/**
 * The point of a segment closest to @p p, by projecting onto the line and clamping to the
 * segment.  A degenerate segment, which OSM rings do produce, collapses to its first
 * point rather than dividing by zero.
 */
Nearest nearest_on_segment(const Point &p, const Point &a, const Point &b)
{
    const auto dx = b.x - a.x;
    const auto dy = b.y - a.y;
    const auto length_squared = dx * dx + dy * dy;

    auto t = 0.0;
    if (length_squared > 0.0)
    {
        t = std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / length_squared, 0.0, 1.0);
    }

    const Point at{a.x + t * dx, a.y + t * dy};
    const auto ex = p.x - at.x;
    const auto ey = p.y - at.y;
    return {ex * ex + ey * ey, at};
}

} // namespace

Clearance clearance(const Point &point, std::span<const Ring> rings)
{
    Clearance best;
    auto best_squared = std::numeric_limits<double>::infinity();
    // The nearest thing that is not what the point is already standing on; see
    // Clearance::room.
    auto room_squared = std::numeric_limits<double>::infinity();

    // Free space is inside the outer ring and outside every obstacle.  Computed here
    // rather than left to the caller because the distance is meaningless without it: the
    // two sides of a wall are the same distance from it.
    for (std::size_t r = 0; r < rings.size(); ++r)
    {
        const auto &ring = rings[r];
        if (ring.size() < 2)
        {
            continue;
        }

        for (std::size_t i = 0; i < ring.size(); ++i)
        {
            const auto found = nearest_on_segment(point, ring[i], ring[(i + 1) % ring.size()]);
            // Strictly less, so the first segment wins a tie.  A point equidistant from
            // two segments is on the medial axis, where which one is named is arbitrary;
            // what matters is that the choice is the same on every platform and in every
            // run, because a path is built out of these answers.
            if (found.distance_squared < best_squared)
            {
                best_squared = found.distance_squared;
                best.nearest = found.at;
                best.ring = r;
                best.segment = i;
            }
            if (found.distance_squared > ON_GEOMETRY * ON_GEOMETRY &&
                found.distance_squared < room_squared)
            {
                room_squared = found.distance_squared;
            }
        }
    }

    if (!std::isfinite(best_squared))
    {
        return {};
    }

    best.distance = std::sqrt(best_squared);
    best.room = std::isfinite(room_squared) ? std::sqrt(room_squared) : best.distance;
    // The free space is closed, so a point on the boundary is on legal ground.  Asking
    // inside_area() alone would not settle it: it is strict, and ray casting on a point
    // that lies exactly on an edge answers arbitrarily.  The distance to the nearest
    // geometry has just been computed, and a point on the boundary is the one at zero.
    best.inside = best.distance <= ON_GEOMETRY || inside_area(point, rings);
    if (best.distance > 0.0)
    {
        best.gradient = {(point.x - best.nearest.x) / best.distance,
                         (point.y - best.nearest.y) / best.distance};
    }
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
