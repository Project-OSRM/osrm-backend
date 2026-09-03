#ifndef OSRM_ENGINE_AREA_BAND_HPP
#define OSRM_ENGINE_AREA_BAND_HPP

#include "engine/area_visibility.hpp"

#include <cstddef>
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
};

/**
 * @brief The soft clearance floor.
 *
 * `rho - delta * (1 - exp(-rho / delta))`.  Behaves like `rho - delta` once there is room
 * to spare, so the full comfort margin applies in the open, but it vanishes only where
 * `rho` does.  Subtracting the margin outright would instead zero everything within
 * `delta` of an obstacle, which closes every passage narrower than `2 delta` and
 * disconnects places people actually walk.
 *
 * Near zero it behaves like `rho^2 / (2 delta)`, rising gently out of contact, and it is
 * strictly increasing, so it never reorders two clearances or flips the sign of a
 * gradient.
 */
double soft_floor(double clearance, double comfort);

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
 * Interior nodes want strictly positive clearance to work with.  A taut path does not
 * provide it: the shortest way past a rectangular obstacle runs along its edges, so the
 * path grazes the geometry for most of its length and the bubbles there have no radius.
 * The answer is to plan on geometry that has been eroded by the hard margin, so the taut
 * path turns on offset arcs and every node starts with room around it; see
 * plans/elastic-band/plan.md.  Given a path that touches the geometry anyway, the band
 * does what it can and certificate_holds() reports that the result is not certified,
 * rather than either looping or pretending.
 *
 * @param path    at least two points, the first and last being the anchors
 * @param rings   the area, outer ring first
 */
Band smooth(std::span<const Point> path,
            std::span<const Ring> rings,
            const BandParameters &parameters);

/**
 * @brief Whether consecutive discs overlap, which is what makes the band collision-free.
 *
 * Anchors are exempt.  A journey that starts at a portal starts on the boundary at zero
 * clearance, so its first disc has no radius and can overlap nothing; the certificate
 * applies from the first interior node outward.
 */
bool certificate_holds(const Band &band);

} // namespace osrm::engine::area

#endif // OSRM_ENGINE_AREA_BAND_HPP
