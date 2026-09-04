#ifndef OSRM_ENGINE_AREA_BAND_HPP
#define OSRM_ENGINE_AREA_BAND_HPP

#include "engine/area_visibility.hpp"

#include "util/coordinate.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace osrm::engine::area
{

/**
 * @brief What the band is made of, and what it settles at.
 *
 * All lengths are in projected units.  The parameters a person would state are metres, a
 * comfort margin of a metre or so; convert once with metres_per_projected_unit() and keep
 * the geometry in one unit throughout.
 */
struct BandParameters
{
    /**
     * The comfort margin, `delta` in the literature and `d_0` as an influence distance.
     * Obstacles push only within this range, so in the open the band feels nothing and
     * settles on a straight line, and in a passage narrower than twice this the pushes
     * from both sides balance and it settles on the centreline.  Without the cutoff a
     * band would wander even in the middle of an empty square.
     */
    double comfort = 0.0;

    //! Tension. Pulls each node towards its neighbours, which is what shortens and
    //! straightens the path. Above about two the band starts declining paths it cannot
    //! certify, and above ten it throws them across the plaza.
    double contraction = 1.5;

    //! How hard obstacles push, relative to the tension. The two are in equilibrium at
    //! the shape the band settles into, so only their ratio matters.
    double repulsion = 1.0;

    /**
     * How much of the computed force is applied per sweep.
     *
     * Not a stability knob, which is what it looks like.  Smaller is worse here: the band
     * is under-converged rather than overshooting, so halving the step just leaves it
     * further from equilibrium when the sweeps run out.  Measured over the corpus, 0.02
     * gives 17 degrees of turn where 0.5 gives 6.
     */
    double step = 0.5;

    /**
     * Target distance between nodes.  Left at zero it follows the comfort margin, which
     * is what it should do: measured over the corpus, the quality of the result depends
     * on the ratio between the two far more than on either alone.
     *
     * A quarter of the comfort margin unless set.  That is what certifies: at a 5 m
     * comfort margin the corpus certifies 173 paths of 177 at a quarter, 137 at a half,
     * 91 at one times and 55 at twice.  A band that does not certify is not smoothed at
     * all, so paying for nodes is worth it.  Each halving doubles the cost.
     *
     * The right ratio depends on which margin is doing the work, and that is worth
     * understanding before tuning it.
     *
     * When the *hard* margin is the larger of the two, the band rounds corners at the
     * eroded boundary and the repulsion never fires; twice the comfort margin is then the
     * best spacing, and finer is worse, because tension propagates one node per sweep and
     * a band with twice the nodes needs four times the sweeps to converge.
     *
     * When the *soft* margin is the larger, which is the configuration the two-tier scheme
     * is actually for, corners are rounded at about the comfort margin and the spacing has
     * to resolve that: one times the comfort margin certifies every path in the corpus
     * where twice it certifies 155 of 177, and a half is better still on the worst case.
     * Each halving doubles the cost.
     *
     * There is no single number here.  What there is: a corner rounded at radius `r` and
     * sampled every `h` turns by about `h / r` at each node, so a path cannot look smoother
     * than its sampling allows, however long the band is relaxed for.
     */
    double spacing = 0.0;

    /**
     * How many sweeps to run.  A fixed budget rather than a convergence tolerance on
     * purpose: a loop that stops when movement falls below a threshold can run a
     * different number of times on different platforms and produce a different path, and
     * this branch has already lost a week to a geometry predicate that behaved
     * differently under one compiler.
     */
    std::size_t sweeps = 30;
};

/**
 * @brief A deformable path, and the discs that prove it is in free space.
 */
struct Band
{
    std::vector<Point> points;
    /**
     * The radius of the free disc at each point, which is its clearance passed through
     * the soft floor.  A path whose consecutive discs overlap lies entirely within their
     * union, hence in free space, so validity is a comparison of distances against these
     * and never a segment-versus-obstacle test.
     */
    std::vector<double> radii;
    /**
     * Whether this path is proved to lie in the free space.
     *
     * smooth() hands back the input unchanged when it cannot prove its own result, so a
     * returned band is not by itself a promise, and the taut input usually cannot be
     * proved either: it grazes the geometry, where the discs have no radius.  A caller
     * that may only walk on proved ground reads this rather than assuming.
     */
    bool certified = false;
};

/**
 * @brief Deform a path until it is locally as short as its surroundings permit.
 *
 * The input is a taut path, straight between the vertices it bends at, which is optimal
 * in length and has two defects a person can see: it kinks, and it touches the corners it
 * turns at.  The band pulls the kinks straight and the repulsion holds it off the
 * geometry, and the result is the shape a rubber band threaded along the path would take.
 *
 * The first and last points are anchors and do not move.  Everything between them is
 * resampled, so the returned path is not the input with the same points in new positions.
 *
 * This is a local deformation: the band moves continuously through free space and cannot
 * cross an obstacle, so whichever side of the fountain the planner chose is the side the
 * result goes.  That is what makes it safe to apply to a path somebody else computed.
 *
 * A taut path grazes the geometry for most of its length, because the shortest way past
 * a rectangular obstacle runs along its edges.  Such a path is legal and certifies as it
 * stands, see certificate_holds(), and it does get smoothed: a node against a wall has a
 * clearance of zero but plenty of room, which is what bounds its step, and the segments
 * it moves over are tested against the rings directly where no free disc can speak for
 * them.  What such a band cannot do is hold the comfort margin at the anchors, since an
 * anchor on a portal sits on the boundary and does not move; the margin is faded in
 * along the path instead.
 *
 * @param path    at least two points, the first and last being the anchors
 * @param rings   the area, outer ring first
 */
Band smooth(std::span<const Point> path,
            std::span<const Ring> rings,
            const BandParameters &parameters);

/**
 * @brief Whether every point of the path is on legal ground.
 *
 * Legal means the closed free space: inside the area, or on the boundary of it or of one
 * of its obstacles.  Running along the edge of an obstacle is walking, not trespassing,
 * and a path that does so certifies.
 *
 * Each segment is proved one of two ways.  Where both ends have room, the free discs
 * overlap and contain the segment between them, which settles it without touching the
 * geometry.  Where they do not -- against a wall, or from an anchor sitting on the
 * boundary, where a disc has no radius at all -- the segment is tested against the rings
 * directly by segment_in_closed_area().
 *
 * This is what smooth() sets Band::certified from, and it is total: it needs no help from
 * its caller for any part of the path.
 */
bool certificate_holds(const Band &band, std::span<const Ring> rings);

/**
 * @brief smooth(), for a path and an area given as coordinates.
 *
 * This is the band as the engine calls it.  The rings and the path are projected once,
 * the comfort margin is converted from metres at the path's own latitude, and the result
 * is projected back.  The two anchors come back exactly as they went in, so a leg's
 * endpoints do not drift by a rounding.
 *
 * The result is thinned first, by Douglas-Peucker to a decimetre.  The band is sampled
 * every quarter margin whether or not the path bends there, and a straight run across an
 * open square would otherwise come back as forty points on a line.  A decimetre is the
 * finest the API draws, so a node that moves the line by less than that draws nothing.
 *
 * @param comfort_metres  the margin; zero or less means do nothing
 * @return the whole smoothed path, anchors included, or nothing when there is nothing to
 *         draw differently: the band declined, or what it produced is the input to within
 *         a decimetre.  Nothing always means "draw the path as it was given".
 */
std::optional<std::vector<util::Coordinate>>
smooth_coordinates(const std::vector<std::span<const util::Coordinate>> &rings,
                   std::span<const util::Coordinate> path,
                   double comfort_metres);

} // namespace osrm::engine::area

#endif // OSRM_ENGINE_AREA_BAND_HPP
