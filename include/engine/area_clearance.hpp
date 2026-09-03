#ifndef OSRM_ENGINE_AREA_CLEARANCE_HPP
#define OSRM_ENGINE_AREA_CLEARANCE_HPP

#include "engine/area_visibility.hpp"

#include <cstddef>
#include <span>

namespace osrm::engine::area
{

/**
 * @brief How much room there is at a point, and which way the nearest obstacle lies.
 *
 * This is the quantity an elastic band is built on.  The clearance is the radius of a
 * disc around the point that is guaranteed free of geometry, so a path whose consecutive
 * points have overlapping discs is collision-free without any segment-versus-obstacle
 * test ever being done.  The gradient is what pushes a path away from whatever it is
 * closest to.
 */
struct Clearance
{
    //! Distance to the nearest point of the area's geometry, in projected units.
    double distance = 0.0;
    //! The nearest point itself, which is on the segment named below.
    Point nearest{};
    /**
     * Unit vector from @c nearest towards the query point, which is the direction the
     * clearance grows fastest in.  Zero when the point lies on the geometry, where the
     * clearance is not differentiable and there is no direction to give.
     */
    Point gradient{};
    /**
     * Whether the point is in the area's free space: inside the outer ring and outside
     * every obstacle.
     *
     * The distance alone cannot say.  It is unsigned, so a point a metre inside a
     * fountain and a point a metre away from it report the same clearance, and anything
     * that treats the distance as room will believe the first one has room.  That is not
     * hypothetical: it is why a band could sit 5.71 m inside an obstacle and still be
     * certified, since every disc it carried looked perfectly valid.
     */
    bool inside = false;
    //! Which ring the nearest point is on, indexing the span that was passed in.
    std::size_t ring = 0;
    //! Which segment of that ring, by the index of its first vertex.
    std::size_t segment = 0;
};

/**
 * @brief The clearance at a point.
 *
 * Measures against every ring, the outer boundary included: a path has to stay off the
 * walls of a plaza as much as off the fountain in the middle of it. The point is not
 * required to be inside the area, and the distance is unsigned, so a point outside gets
 * its distance to the boundary rather than a negative number.  Whether it is inside is
 * reported separately, in Clearance::inside; the distance on its own says how far the
 * geometry is, never which side of it the point is on.
 *
 * A flat scan over the segments.  Plazas have hundreds of them and correctness comes
 * first; an r-tree here would buy a logarithm and cost the ability to check this against
 * a brute-force scan, which is currently how it is tested.  Revisit when a profile says
 * to.
 */
Clearance clearance(const Point &point, std::span<const Ring> rings);

/**
 * @brief How many metres one projected unit is, at a given latitude.
 *
 * Web Mercator is conformal, so locally it scales every direction alike and a distance in
 * projected units can be turned into metres by one factor.  That factor grows with
 * latitude, so it is taken at the area being worked on rather than globally.
 *
 * This exists because the band's parameters are lengths a person would recognise, a
 * clearance of a quarter of a metre and a comfort margin of one, while the geometry
 * predicates all work in projected units.  Converting once, here, keeps the two apart.
 */
double metres_per_projected_unit(double latitude_degrees);

} // namespace osrm::engine::area

#endif // OSRM_ENGINE_AREA_CLEARANCE_HPP
