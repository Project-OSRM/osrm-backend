#ifndef OSRM_ENGINE_AREA_FILLET_HPP
#define OSRM_ENGINE_AREA_FILLET_HPP

#include "engine/area_band.hpp"

#include <span>

namespace osrm::engine::area
{

/**
 * @brief Round the corners of a taut path with tangent arcs, held off the geometry by a
 * margin, the way a road is set out.
 *
 * The taut path is straight runs between the corners it turns at, and every corner sits
 * on the geometry.  Each corner is moved out along its bisector so that both runs end up
 * the margin off the edges they were grazing, and then replaced by a circular arc tangent
 * to both runs, of radius equal to the margin where the runs are long enough to carry it.
 * The result is straight lines and arcs and nothing else: a straight run is never broken
 * and a curve is a curve, so nothing here can zigzag.  Anchors do not move.
 *
 * Every segment of the result is tested against the closed free space.  Where an offset
 * or an arc leaves it, that corner's offset and radius are halved and the path rebuilt,
 * down to the taut corner itself, which is always legal.  A passage narrower than twice
 * the margin therefore gets whatever offset fits.
 *
 * This replaced the elastic band's relaxation as the shape the engine draws.  A relaxed
 * polyline negotiates every node against piecewise-linear geometry and comes out as
 * jagged as the walls at the node spacing, and no gate after the fact makes that smooth.
 *
 * @param path    at least two points, the first and last being the anchors
 * @param rings   the area, outer ring first
 * @param margin  how far off the geometry to hold the path, in projected units
 * @return the rounded path, or the taut path as given when no rounding is legal; its
 *         `certified` flag says whether what came back lies in the closed free space
 */
Band round_corners(std::span<const Point> path, std::span<const Ring> rings, double margin);

} // namespace osrm::engine::area

#endif // OSRM_ENGINE_AREA_FILLET_HPP
