#ifndef OSRM_ENGINE_AREA_FILLET_HPP
#define OSRM_ENGINE_AREA_FILLET_HPP

#include "engine/area_visibility.hpp"

#include "util/coordinate.hpp"
#include "util/function_ref.hpp"

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
 * @brief A planner to ask for the shortest legal path between two points of the area,
 * both ends included, or nothing when it has none.
 *
 * pull_taut() uses it to re-route a stretch of the path round street furniture: passing
 * a planter on the inner side of a corridor junction needs new corners, not fewer, and
 * no chord across the old ones can supply them.  Without a planner only chords are
 * tried, and a planter the path wraps with corners of its own is kept on its side.
 */
using Replan = util::FunctionRef<std::optional<std::vector<Point>>(const Point &, const Point &)>;
using ReplanCoordinates =
    util::FunctionRef<std::optional<std::vector<util::Coordinate>>(util::Coordinate, util::Coordinate)>;

/**
 * @brief Pull a path taut: drop every vertex that the path can do without.
 *
 * A vertex is dropped when the chord across it lies in the closed free space and the
 * triangle it closes holds no geometry it may not hop, so the path never jumps to the
 * other side of an obstacle it went round, unless that obstacle is small; see @p hop.  What remains is straight runs between corners the geometry
 * forces, which is what round_corners() assumes of its input.  A shortest path from the visibility graph is
 * already that and comes back unchanged.  A path from anything else, a grid search with
 * its staircase of steps, a mesh path with a collinear vertex, a trace with noise in it,
 * is not, and rounding it as it stands rounds every step and keeps the wobble.
 *
 * Then the string is tightened.  Each surviving vertex is walked towards the chord
 * between its neighbours as far as the move stays legal and sweeps no geometry, and
 * offered the corners of the geometry as places to stand; whatever shortens the path in
 * its own homotopy class is taken, and a vertex that reaches the chord is dropped.  The
 * result is the locally shortest path through the same gaps the input threaded, with
 * every remaining corner on the geometry: a wander round an obstacle comes out as the
 * geodesic round that obstacle, not as the shortcut past it.  A segment of the input that
 * is itself outside the free space is kept as it is, and the result's legality is judged
 * downstream.
 *
 * The anchors are never dropped.
 *
 * @param hop  an obstacle no bigger across than this may be passed on whichever side is
 *             shorter; zero keeps the path on the side of every obstacle it was given.
 *             The engine passes the size of street furniture, so a planter or a tree
 *             pit in a corridor junction does not make the path go round it the long
 *             way, while a kiosk-sized thing and anything larger keeps its side.
 */
std::vector<Point> pull_taut(std::span<const Point> path,
                             std::span<const Ring> rings,
                             double hop = 0.0,
                             std::optional<Replan> replan = std::nullopt);

/**
 * @brief Round the corners of a path with tangent arcs, held off the geometry by a
 * margin, the way a road is set out.
 *
 * The path is pulled taut first, see pull_taut(), so what is rounded is straight runs
 * between the corners the geometry forces, each corner on the inside of its turn.  Each corner is moved out along its bisector, as far as the first
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
 * @param path    at least two points, the first and last being the anchors; need not be
 *                taut
 * @param rings   the area, outer ring first
 * @param margin  how far off the geometry to hold the path, in projected units
 * @param hop     an obstacle no bigger across than this is passed on whichever side is
 *                shorter; see pull_taut()
 * @return the rounded path, or the pulled path unrounded when no rounding is legal
 */
RoundedPath round_corners(std::span<const Point> path,
                          std::span<const Ring> rings,
                          double margin,
                          double hop = 0.0,
                          std::optional<Replan> replan = std::nullopt);

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
              double margin_metres,
              std::optional<ReplanCoordinates> replan = std::nullopt);

} // namespace osrm::engine::area

#endif // OSRM_ENGINE_AREA_FILLET_HPP
