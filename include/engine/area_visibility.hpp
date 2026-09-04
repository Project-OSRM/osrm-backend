#ifndef OSRM_ENGINE_AREA_VISIBILITY_HPP
#define OSRM_ENGINE_AREA_VISIBILITY_HPP

#include "extractor/area/util.hpp"

#include "util/coordinate.hpp"
#include "util/web_mercator.hpp"

#include <boost/geometry/core/cs.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace osrm::engine::area
{

/**
 * @brief A projected point, in metres, for the geometry predicates to work on.
 *
 * The predicates in extractor/area/util.hpp are templates that read coordinates through
 * boost::geometry::get(), so they work on any adapted point type.  Rather than
 * reimplement them here -- they are delicate, and one copy of them is quite enough -- we
 * adapt a small local type and reuse them unchanged.
 *
 * util::Coordinate itself is deliberately not adapted: it is used throughout the engine,
 * and giving it boost::geometry traits would apply to every translation unit that
 * happens to include this header.
 */
struct Point
{
    double x = 0.0;
    double y = 0.0;
};

/**
 * @brief Project a WGS84 coordinate into web mercator degrees.
 *
 * Only orientation and intersection are asked of these coordinates, both of which are
 * preserved by the projection, so the units do not matter as long as they are the same
 * for every point.  Distances are measured on the original coordinates instead.
 */
inline Point project(const util::Coordinate coordinate)
{
    return {static_cast<double>(util::toFloating(coordinate.lon)),
            util::web_mercator::latToY(util::toFloating(coordinate.lat))};
}

/**
 * @brief One ring of an area, as a view over the shared vertex array.
 *
 * The first ring of an area is its outer boundary, the rest are obstacles.
 */
using Ring = std::span<const Point>;

/**
 * @brief Return true if the open segment from..to crosses the given ring.
 *
 * Endpoints are excluded: a segment that merely touches a vertex of the ring, which is
 * the usual case when the far end *is* one of the ring's vertices, does not cross it.
 */
bool crosses_ring(const Point &from, const Point &to, Ring ring);

/**
 * @brief Return true if the point lies strictly inside the ring.
 */
bool inside_ring(const Point &point, Ring ring);

/**
 * @brief Return true if the point is inside the area: inside its outer ring and outside
 * every obstacle.
 */
bool inside_area(const Point &point, std::span<const Ring> rings);

/**
 * @brief How close to the geometry counts as being on it, in projected units.
 *
 * About a tenth of a millimetre at the equator and less further from it.  The value has
 * to sit in a gap, and there is a wide one: OSM coordinates are stored to 1e-7 degrees,
 * roughly a centimetre, so nothing below that is a distinction the input can express,
 * while the arithmetic done on them rounds at 1e-16 or so.  Anywhere in between will do,
 * and this is the middle of it.
 *
 * Too tight and a path running along an edge is refused for being a nanometre off it,
 * which is not a defect of the path but of the arithmetic: 12 taut paths in the corpus
 * were rejected that way.  Too loose and a path could cut a real corner.
 */
constexpr double ON_GEOMETRY = 1e-9;

/**
 * @brief Return true if the point is in the closed free space: inside the area, or on
 * the boundary of any of its rings to within `tolerance`.
 *
 * The free space is closed, so walking along the edge of an obstacle is walking on legal
 * ground.  This matters more than it sounds: the shortest way past a rectangle runs along
 * its sides, so the boundary is not an edge case here but the commonest place a path is
 * found.  It also has to be asked with a tolerance, because inside_area() is strict and
 * ray casting answers arbitrarily for a point that lies exactly on an edge -- which the
 * midpoint of two adjacent vertices always does.
 */
bool in_closed_area(const Point &point, std::span<const Ring> rings, double tolerance);

/**
 * @brief Return true if every point of the segment from..to is in the closed free space.
 *
 * A complete test, not a sample.  The segment is cut at every parameter where it could
 * change sides -- each crossing of a ring edge, and each end of a stretch where it runs
 * along one -- and the midpoint of each resulting piece is tested.  A piece with no cut
 * inside it lies wholly in the free space, wholly in an obstacle, or wholly on the
 * boundary, so its midpoint speaks for all of it.
 *
 * This is what a proper-crossing test such as crosses_ring() cannot do.  That test
 * exempts a ring edge sharing an endpoint with the segment, on the reasoning that it
 * meets it only there; the reasoning holds for the sweep, where both ends are ring
 * vertices, and fails for a segment that leaves a vertex straight into the obstacle it
 * belongs to.
 */
bool segment_in_closed_area(const Point &from, const Point &to, std::span<const Ring> rings);

/**
 * @brief Return true if every segment of the polyline is in the closed free space.
 *
 * Empty and single-point paths are legal when their one point is.  This is the test the
 * drawn shape of a plaza route has to pass, see round_corners() in area_fillet.hpp.
 */
bool path_in_closed_area(std::span<const Point> points, std::span<const Ring> rings);

/** The squared distance from @p point to the segment a..b. */
double distance_squared_to_segment(const Point &point, const Point &a, const Point &b);

/**
 * @brief Return the indices of the area's vertices that the point can see.
 *
 * A vertex is visible when the straight line to it stays inside the area: it crosses no
 * ring, and its midpoint is inside.  These are the vertices the snapping code turns into
 * phantom candidates, each reached by a straight line and costed at the walking speed --
 * which is what makes a route leave the area directly instead of first travelling to
 * whichever meshed line the coordinate happened to land near.
 *
 * Indices are into the flattened vertex array, i.e. across all rings in order.
 */
std::vector<std::size_t> visible_vertices(const Point &point, std::span<const Ring> rings);

} // namespace osrm::engine::area

namespace boost::geometry::traits
{
template <> struct tag<osrm::engine::area::Point>
{
    using type = point_tag;
};
template <> struct dimension<osrm::engine::area::Point> : boost::mpl::int_<2>
{
};
template <> struct coordinate_type<osrm::engine::area::Point>
{
    using type = double;
};
template <> struct coordinate_system<osrm::engine::area::Point>
{
    using type = boost::geometry::cs::cartesian;
};
template <> struct access<osrm::engine::area::Point, 0>
{
    static inline double get(const osrm::engine::area::Point &p) { return p.x; }
    static inline void set(osrm::engine::area::Point &p, const double &value) { p.x = value; }
};
template <> struct access<osrm::engine::area::Point, 1>
{
    static inline double get(const osrm::engine::area::Point &p) { return p.y; }
    static inline void set(osrm::engine::area::Point &p, const double &value) { p.y = value; }
};
} // namespace boost::geometry::traits

#endif // OSRM_ENGINE_AREA_VISIBILITY_HPP
