#ifndef OSRM_ENGINE_AREA_FILLET_HPP
#define OSRM_ENGINE_AREA_FILLET_HPP

#include "engine/area_visibility.hpp"

#include "util/coordinate.hpp"

#include <optional>
#include <span>
#include <vector>

namespace osrm::engine::area
{

/**
 * @brief A path drawn across an area, and whether every segment of it is on legal ground.
 */
struct RoundedPath
{
    std::vector<Point> points;
    //! Every segment lies in the closed free space: inside the area or along a ring.
    //! round_corners() only ever hands back a rounded shape that is; the taut input it
    //! falls back to may not be, and this says so.
    bool legal = false;
};

/**
 * @brief Round the corners of a taut path with tangent arcs, held off the geometry by a
 * margin, the way a road is set out.
 *
 * The taut path is straight runs between the corners it turns at, and every corner sits
 * on the geometry.  Each corner is moved out along its bisector, as far as the first
 * point with the margin of room around it or the widest point if none has that much, so
 * that the runs either side sit off the edges they were grazing; and it is then replaced
 * by a circular arc tangent to both runs, of the margin's radius where the runs are long
 * enough to carry it.  The result is straight lines and arcs and nothing else: a straight
 * run is never broken and a curve is a curve, so nothing here can zigzag.  Anchors do
 * not move.
 *
 * Every segment of the result is tested against the closed free space, and a corner
 * whose offset swings its run round, sharpens its turn, or leaves the free space has its
 * offset and radius halved and the path rebuilt, down to the taut corner itself, which
 * is always legal.  A passage narrower than twice the margin therefore gets whatever
 * offset fits, which is the centreline.
 *
 * This replaced an elastic band as the shape the engine draws.  A relaxed polyline
 * negotiates every node against piecewise-linear geometry and comes out as jagged as the
 * walls at the node spacing, and no gate applied afterwards makes it smooth.  Measured on
 * 1,884 taut geodesics through Ile-de-France plazas at a 5 m margin, the mean sharpest
 * corner went from 46 degrees to 18, the band's best was 37, and the cost per leg from
 * tens of milliseconds to microseconds.
 *
 * @param path    at least two points, the first and last being the anchors
 * @param rings   the area, outer ring first
 * @param margin  how far off the geometry to hold the path, in projected units
 * @return the rounded path, or the taut path as given when no rounding is legal
 */
RoundedPath
round_corners(std::span<const Point> path, std::span<const Ring> rings, double margin);

/**
 * @brief round_corners(), for a path and an area given as coordinates.
 *
 * This is what the engine calls.  The rings and the path are projected once, the margin
 * is converted from metres at the path's own latitude, and the result is thinned by
 * Douglas-Peucker to a decimetre, the finest the API draws, and projected back with the
 * two anchors returned exactly as given, so a leg's endpoints do not drift by a rounding.
 *
 * @param margin_metres  the margin; zero or less means do nothing
 * @return the whole rounded path, anchors included, or nothing when there is nothing to
 *         draw differently: no rounding was legal, or what came back is the input to
 *         within a decimetre.  Nothing always means "draw the path as it was given".
 */
std::optional<std::vector<util::Coordinate>>
round_corners(const std::vector<std::span<const util::Coordinate>> &rings,
              std::span<const util::Coordinate> path,
              double margin_metres);

} // namespace osrm::engine::area

#endif // OSRM_ENGINE_AREA_FILLET_HPP
