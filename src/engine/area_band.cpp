#include "engine/area_band.hpp"

#include "engine/area_clearance.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace osrm::engine::area
{

namespace
{

/**
 * A ceiling on how many nodes a band may grow to.
 *
 * Insertion is driven by the certificate, and a certificate that cannot be satisfied --
 * a node pinned against geometry with no room around it -- would otherwise ask for a node
 * between every pair for ever.  A bound turns that into a band that is merely dense
 * rather than a query that never returns.
 */
constexpr std::size_t MAX_NODES = 2000;

//! How much finer than the requested spacing insertion may go to satisfy the certificate.
constexpr double FINEST = 0.5;

Point operator-(const Point &a, const Point &b) { return {a.x - b.x, a.y - b.y}; }
Point operator+(const Point &a, const Point &b) { return {a.x + b.x, a.y + b.y}; }
Point operator*(const Point &a, const double s) { return {a.x * s, a.y * s}; }

double length(const Point &p) { return std::hypot(p.x, p.y); }

Point unit(const Point &p)
{
    const auto len = length(p);
    return len > 0.0 ? Point{p.x / len, p.y / len} : Point{0.0, 0.0};
}

double dot(const Point &a, const Point &b) { return a.x * b.x + a.y * b.y; }

struct Nearest
{
    double distance_squared;
    Point at;
};

Nearest nearest_on_segment(const Point &p, const Point &a, const Point &b)
{
    const auto dx = b.x - a.x, dy = b.y - a.y;
    const auto length_squared = dx * dx + dy * dy;
    auto t = 0.0;
    if (length_squared > 0.0)
    {
        t = std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / length_squared, 0.0, 1.0);
    }
    const Point at{a.x + t * dx, a.y + t * dy};
    const auto ex = p.x - at.x, ey = p.y - at.y;
    return {ex * ex + ey * ey, at};
}

//! Resample a polyline at a fixed spacing, keeping both ends.
std::vector<Point> resample(std::span<const Point> path, const double spacing)
{
    std::vector<Point> out{path.front()};
    if (!(spacing > 0.0))
    {
        out.insert(out.end(), path.begin() + 1, path.end());
        return out;
    }

    auto carried = 0.0;
    for (std::size_t i = 0; i + 1 < path.size(); ++i)
    {
        const auto from = path[i];
        const auto to = path[i + 1];
        const auto span = length(to - from);
        if (!(span > 0.0))
        {
            continue;
        }
        const auto direction = (to - from) * (1.0 / span);

        for (auto at = spacing - carried; at < span; at += spacing)
        {
            out.push_back(from + direction * at);
        }
        carried = std::fmod(carried + span, spacing);
    }

    out.push_back(path.back());
    return out;
}

/**
 * @brief The push away from everything nearby, not only from the nearest thing.
 *
 * Summed over every segment within the influence distance, each contributing
 * `(influence - distance)` along the direction away from it.
 *
 * Taking only the nearest feature, which is what the textbook force does, makes the
 * direction of the push jump wherever the nearest feature changes.  That set is the medial
 * axis, and a band crossing it has consecutive nodes shoved away from different obstacles
 * in different directions.  Measured on the corpus, every large turn in a smoothed band sat
 * exactly on such a change: 29 degrees where the nearest ring changed, 16 where the nearest
 * segment of one ring changed, against 0.1 to 5 degrees everywhere else.
 *
 * Summing removes the discontinuity rather than damping it.  Each term is continuous, and a
 * term reaching the influence distance fades to zero rather than switching off, so the
 * whole field is continuous and the medial axis stops being a feature of it.  This is the
 * Voronoi field of Dolgov et al. in the form this band needs.
 */
Point repulsion_from_everything(const Point &at,
                                std::span<const Ring> rings,
                                const double influence)
{
    Point total{0.0, 0.0};
    if (!(influence > 0.0))
    {
        return total;
    }

    for (const auto &ring : rings)
    {
        if (ring.size() < 2)
        {
            continue;
        }

        // One push per obstacle, from that obstacle's nearest point.
        //
        // Not one per segment, which is what this did first and is the wrong quantity: a
        // wall cut into sixteen short segments then pushes about sixteen times as hard as
        // the same wall left as one, so the force depends on how finely the geometry was
        // tessellated rather than on where it is.  The obstacles here are dilated holes
        // polygonised into sixteen-gons and the plaza wall is nine long segments, so the
        // holes were shouting and the walls whispering.
        //
        // It also wobbled.  Contributions from neighbouring segments of one arc point in
        // slightly different directions, so as a node slid past a vertex the direction of
        // the sum swung, and the band chased it: measured over the corpus, five changes of
        // bending direction per path where the taut path had none, and 145 degrees of
        // total turning to achieve a net 38.  With the repulsion switched off entirely the
        // same band turned 2 degrees in total, which is what said the force was the source
        // rather than the tension or the sweep budget.
        //
        // Summing over obstacles is still a sum, so this keeps the property the sum was
        // introduced for: the single nearest feature changes identity across the medial
        // axis and takes the force direction with it, and adding up everything in range
        // crosses that axis smoothly.
        auto nearest_squared = std::numeric_limits<double>::infinity();
        Point nearest{0.0, 0.0};
        for (std::size_t i = 0; i < ring.size(); ++i)
        {
            const auto found = nearest_on_segment(at, ring[i], ring[(i + 1) % ring.size()]);
            if (found.distance_squared < nearest_squared)
            {
                nearest_squared = found.distance_squared;
                nearest = found.at;
            }
        }

        const auto distance = std::sqrt(nearest_squared);
        if (!(distance > 0.0) || distance >= influence)
        {
            continue;
        }
        total = total + unit(at - nearest) * (influence - distance);
    }
    return total;
}

/**
 * The radius of the free disc at a point, which is its clearance.
 *
 * The true clearance, not the soft-floored one.  Passing it through the floor first was
 * tried, on the argument that a smaller radius keeps the certificate honest, and it is
 * honest but useless: near an obstacle the floor is quadratically small, so a node lifted
 * to 15% of the comfort margin gets a bubble of 1% of it, and no node density that
 * terminates can make consecutive bubbles overlap.  Over the corpus every single band
 * declined.
 *
 * The certificate is a proof that a segment lies in free space, and the free disc is
 * exactly the true clearance.  The comfort margin belongs to the repulsion force, which
 * is where the two-tier scheme puts it: a margin expressed as a force cannot close a
 * passage, and one expressed as a radius can.
 */
double bubble_radius(const Point &at, std::span<const Ring> rings, const double /*comfort*/)
{
    const auto here = clearance(at, rings);
    // Outside the free space there is no free disc, whatever the distance says.  The
    // distance is unsigned, so a node that has drifted inside an obstacle reports its
    // room to the far wall and would otherwise carry a large, entirely fictional bubble
    // that satisfies every check downstream.
    return here.inside ? here.distance : 0.0;
}

} // namespace

/**
 * How much of the repulsion applies, as a function of distance along the band from the
 * nearer anchor.
 *
 * An anchor is pinned where it was asked to be, and on a portal that is the plaza wall
 * itself, at zero clearance.  Asking the node beside it to hold the full comfort margin
 * means gaining the whole margin over one node's spacing, which is a right angle, and it
 * lands at the one place on the path everybody looks.  So the margin is faded in, and the
 * band peels off the wall over a stretch instead of jumping off it.
 *
 * Two properties, both paid for on the corpus.  The reach is three comfort margins rather
 * than one: gaining a margin of clearance over a margin of walking is a 45 degree
 * departure, and one margin of reach measured 130 degrees at the anchors where three
 * measures 27.  The shape is smoothstep rather than a straight line because a linear ramp
 * has a corner in its derivative where it reaches full strength and the band puts a corner
 * in the path at exactly that point; the worst turns were sitting in the half margin
 * either side of it.
 */
double anchor_ramp(const double from_anchor, const double comfort)
{
    constexpr double REACH = 3.0;
    if (!(comfort > 0.0))
    {
        return 1.0;
    }
    const auto t = std::clamp(from_anchor / (comfort * REACH), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

double soft_floor(const double clearance_distance, const double comfort)
{
    if (!(comfort > 0.0))
    {
        return std::max(0.0, clearance_distance);
    }
    if (!(clearance_distance > 0.0))
    {
        return 0.0;
    }
    return clearance_distance - comfort * (1.0 - std::exp(-clearance_distance / comfort));
}

bool certificate_holds(const Band &band, std::span<const Ring> rings)
{
    if (band.points.size() != band.radii.size())
    {
        return false;
    }
    if (band.points.empty())
    {
        return true;
    }
    if (band.points.size() == 1)
    {
        return segment_in_closed_area(band.points.front(), band.points.front(), rings);
    }

    for (std::size_t i = 0; i + 1 < band.points.size(); ++i)
    {
        // The discs first, because they are cheap: two free discs that overlap contain
        // the segment between their centres, so the segment is free without looking at
        // any geometry.  This is what the band is built to satisfy, and in the open it
        // answers for the whole path.
        const auto covered = band.radii[i] > 0.0 && band.radii[i + 1] > 0.0 &&
                             length(band.points[i + 1] - band.points[i]) <
                                 band.radii[i] + band.radii[i + 1];
        if (covered)
        {
            continue;
        }

        // Where they have nothing to say, ask the geometry directly.
        //
        // They have nothing to say wherever the path touches something, since a disc
        // there has no radius -- and that is not a corner case, it is where shortest
        // paths live: the way past a rectangle runs along its sides. Refusing those was
        // refusing the legal paths that matter most, and it made the certificate a test
        // of how much room a path had rather than of whether it was legal.
        if (!segment_in_closed_area(band.points[i], band.points[i + 1], rings))
        {
            return false;
        }
    }
    return true;
}

namespace
{

//! One band at one resolution, relaxed to the sweep budget.  See smooth() for why this is
//! called more than once.
Band relax(std::span<const Point> path,
           std::span<const Ring> rings,
           const BandParameters &parameters,
           const double spacing)
{
    Band band;
    if (path.size() < 2)
    {
        band.points.assign(path.begin(), path.end());
        band.radii.assign(path.size(), 0.0);
        return band;
    }

    band.points = resample(path, spacing);
    band.radii.assign(band.points.size(), 0.0);
    const auto recompute = [&](const std::size_t i)
    { band.radii[i] = bubble_radius(band.points[i], rings, parameters.comfort); };
    for (std::size_t i = 0; i < band.points.size(); ++i)
    {
        recompute(i);
    }

    for (std::size_t sweep = 0; sweep < parameters.sweeps; ++sweep)
    {
        // How far each node is along the band from the nearer anchor.  An anchor is fixed
        // where it was asked to be, and on a portal that is the boundary itself, so
        // pushing the node beside it away from that same boundary at full strength puts a
        // kink at the one place everybody looks.  Measured on the corpus: 68 degrees at
        // the anchors against 36 in the middle.
        std::vector<double> from_anchor(band.points.size(), 0.0);
        for (std::size_t i = 1; i < band.points.size(); ++i)
        {
            from_anchor[i] = from_anchor[i - 1] + length(band.points[i] - band.points[i - 1]);
        }
        const auto total_length = from_anchor.back();
        for (auto &distance : from_anchor)
        {
            distance = std::min(distance, total_length - distance);
        }

        // Gauss-Seidel, in index order: each node sees its predecessor's new position.
        // Converges faster than updating everything from the old positions, and the order
        // is the index order rather than anything derived from a distance, so it is the
        // same everywhere.
        for (std::size_t i = 1; i + 1 < band.points.size(); ++i)
        {
            const auto &previous = band.points[i - 1];
            const auto &next = band.points[i + 1];
            const auto at = band.points[i];

            // Tension: pull towards the midpoint of the neighbours, so the force grows
            // with how far the node is off the chord between them.
            //
            // Quinlan's normalised form, the sum of the two unit vectors, is scale-free
            // and was tried first.  Its magnitude decays with the curvature it is
            // removing, so it crawls exactly where the path is nearly straight, and
            // against a fixed sweep budget it does not arrive: an open square converged
            // from 61 to 56 in thirty sweeps when the answer was 50.  Spacing is held by
            // resampling and by the maintenance below, so the property that form buys is
            // one this band does not need.
            auto internal = ((previous + next) * 0.5 - at) * parameters.contraction;

            // Only the part across the path changes its shape.  The part along it slides
            // nodes towards each other, bunching them in the middle while the geometry
            // stays exactly as kinked as it was.
            const auto tangent = unit(next - previous);
            internal = internal - tangent * dot(internal, tangent);

            const auto here = clearance(at, rings);
            // Faded in over the first stretch of the band, so the path leaves an anchor
            // along the way it arrived rather than being shoved off the wall the moment
            // it starts.
            auto external =
                repulsion_from_everything(at, rings, parameters.comfort) *
                (parameters.repulsion * anchor_ramp(from_anchor[i], parameters.comfort));
            if (length(external) == 0.0 && here.distance < parameters.comfort)
            {
                Point away{0.0, 0.0};
                {
                    // Sitting exactly on the geometry, where the clearance has no
                    // gradient to follow.  The bisector of the two neighbours points into
                    // the area, which is enough to get the node off the corner; after
                    // that the gradient exists and takes over.  A taut path bends
                    // precisely at such corners, so this is the ordinary case on the
                    // first sweep, not a curiosity.
                    // Sitting exactly on the geometry, where every distance is zero and
                    // the sum above has nothing to point along.  The bisector of the two
                    // neighbours points into the area, which is enough to get off the
                    // corner; after that the field exists and takes over.
                    away = unit(unit(previous - at) + unit(next - at));
                }
                external = away * (parameters.repulsion * parameters.comfort);
            }

            auto move = (internal + external) * parameters.step;

            // Never leave the bubble in one step.  This is what carries the certificate
            // through the whole optimisation: if a node cannot leave its own disc of free
            // space, the path cannot leave free space, and no collision test is needed at
            // any point.
            //
            // Against the true clearance, not the soft-floored radius.  The floor is
            // there to shape the repulsion and to keep the certificate conservative, and
            // near an obstacle it is quadratically small: half a metre of real room
            // becomes two centimetres of bubble.  Clamping to that freezes exactly the
            // nodes that most need to move while their neighbours a little further out
            // move freely, and the path zigzags between them.  The free disc is what the
            // step must not leave, and the free disc is the true clearance.
            const auto limit = here.distance * 0.5;
            const auto moved = length(move);
            if (limit > 0.0 && moved > limit)
            {
                move = move * (limit / moved);
            }
            else if (!(limit > 0.0))
            {
                // No room at all: the node is on the geometry, its bubble has no radius,
                // and the certificate has nothing to say about where it may go.  This is
                // the one step not covered by it, so it is the one step that has to be
                // checked outright.
                //
                // A small step lets a node pinned on a corner get off it, which is worth
                // having because a taut path bends exactly on corners.  Unchecked it is
                // also the only way a node can leave the area, and over the corpus it
                // did.
                const auto escape = spacing * 0.25;
                if (moved > escape && moved > 0.0)
                {
                    move = move * (escape / moved);
                }
                // The destination being inside the area is not enough.  This step is
                // the only one not bounded by a free disc, so it is the only one that
                // can pass over something on the way, and a small obstacle is exactly
                // what it can step over: a node walked clean across one on the corpus
                // and the band came out on the far side of it, in a different homotopy
                // class from the path it was given, with every segment of the result
                // still in free space and the certificate still satisfied.  What has to
                // be checked is the ground covered, not the destination.
                const auto destination = at + move;
                auto swept_something = !inside_area(destination, rings);
                for (const auto &ring : rings)
                {
                    if (swept_something)
                    {
                        break;
                    }
                    swept_something = crosses_ring(at, destination, ring);
                }
                if (swept_something)
                {
                    move = {0.0, 0.0};
                }
            }

            // The bubble bounds where the node may go.  It says nothing about the two
            // segments hanging off it, and those are what sweep across the plane as the
            // node slides.  Where consecutive bubbles overlap the certificate covers
            // them, but it is silent about the first and last segment of the band, and on
            // a three point band that is every segment it has: the middle node drifted
            // for eleven sweeps with nothing watching its segments and took the path
            // round the other side of a small obstacle.
            //
            // So a move that would drag a segment across geometry is refused.  Two
            // crossing tests per node per sweep, and the deformation is then continuous
            // by construction rather than by an argument that has exceptions.
            const auto destination = at + move;
            const auto moved_radius = clearance(destination, rings).distance;

            // Where the two bubbles overlap, the segment between them lies in their union
            // and is free: that is the certificate, and it costs a comparison.  The
            // crossing test is only for the segments the certificate cannot speak for.
            // Using it for every segment refuses moves that are provably safe, and over
            // the corpus that took the number of paths improved from 130 to 12.
            const auto covered = [&](const Point &neighbour, const double neighbour_radius)
            {
                if (length(neighbour - destination) < moved_radius + neighbour_radius)
                {
                    return true;
                }
                return std::none_of(rings.begin(),
                                    rings.end(),
                                    [&](const Ring &ring)
                                    { return crosses_ring(neighbour, destination, ring); });
            };

            if (covered(previous, band.radii[i - 1]) && covered(next, band.radii[i + 1]))
            {
                band.points[i] = destination;
                recompute(i);
            }
        }

        // Maintenance: keep the chain of bubbles overlapping, and stop paying for nodes
        // that are not holding it together.  Density then follows the geometry on its
        // own, dense where the corridor is tight and sparse across open ground.
        for (std::size_t i = 0; i + 1 < band.points.size();)
        {
            const auto gap = length(band.points[i + 1] - band.points[i]);
            // A node lying on the geometry has a bubble of no radius, so no gap next to
            // it can ever satisfy the certificate and halving it does not help.  Without
            // a floor the loop subdivides until the node ceiling stops it, which is how a
            // four point path became two thousand.  The floor stops the subdivision and
            // certificate_holds() then reports honestly that the band is not certified
            // there, which is the truth and is what erosion is for.
            // The first and last segments are the ones the certificate exempts, because
            // an anchor sits where it was asked to and on a portal that is the boundary
            // itself, where the clearance is zero.  Subdividing them cannot ever satisfy
            // a condition that involves a bubble of no radius, so it runs to the floor
            // and leaves a dense jittery cluster at each end: a three node path came back
            // with 157, eight times finer than the spacing asked for, and at that density
            // the smallest wobble is a large angle.
            const auto at_an_end = i == 0 || i + 2 >= band.points.size();
            // Never finer than the sampling asked for.  The certificate can ask for more
            // nodes than that where the geometry is tight, and giving them to it is how a
            // three node path became a hundred and fifty: tension then propagates one
            // node per sweep, so the band needed hundreds of sweeps to converge and at
            // thirty it was nowhere near, which is what the wiggle was.
            const auto too_fine = spacing > 0.0 && gap < spacing * FINEST;

            // Two reasons to put a node in, and the second one was missing.
            //
            // The certificate wants one when consecutive bubbles stop meeting, because
            // then nothing vouches for the segment between them.  But the sampling wants
            // one too, and the two are not the same request: in the open the bubbles are
            // tens of metres across and meet however far apart the nodes drift, so the
            // certificate is silent exactly where the band is thinning out.
            //
            // Nothing else restores the spacing.  Tension is filtered to its component
            // across the path, deliberately, so it never slides a node along the band to
            // even out its neighbours; nodes can drift apart and stay there.  Where they
            // drift most is where the path bends hardest, since that is where the two
            // neighbours of a node pull it furthest off the chord, so the band ended up
            // coarsest at precisely the corners it most needed resolution to round.
            // Measured on one corpus path: gaps of four and six times the target spacing
            // at the two corners, against one times along the straight stretches between
            // them, and the whole of that path's turning packed into the six nodes that
            // straddled each corner.
            //
            // Halving a gap of one and a half spacings gives three quarters of one, which
            // is above the FINEST floor, so this does not fight the thinning pass below.
            // Inserting where the path turns sharply was tried, and does not work.
            //
            // The reasoning is sound as far as it goes: at a pinch point both the spacing
            // rule and the certificate are satisfied, so nothing asks for another node,
            // and the whole turn lands on one of them. One corpus band turned 71 degrees
            // at a single node in its middle, with its neighbours spaced normally and its
            // bubbles overlapping, while squeezing past an obstacle at a third of the
            // comfort margin.
            //
            // Halving the gaps there describes that corner more finely and does not change
            // it. The node sits where two repulsions balance, and adding neighbours does
            // not move the equilibrium: the worst turn over the corpus did not shift by a
            // hundredth of a degree, the mean got slightly worse, and it cost 15 percent in
            // time. The sharp turns that remain are the shape the forces settle on, not an
            // artefact of how finely it is sampled.
            constexpr double COARSEST = 1.5;
            const auto certificate_wants = gap >= band.radii[i] + band.radii[i + 1];
            const auto too_coarse = spacing > 0.0 && gap > spacing * COARSEST;
            if ((certificate_wants || too_coarse) && gap > 0.0 && !too_fine && !at_an_end)
            {
                const auto middle = (band.points[i] + band.points[i + 1]) * 0.5;
                // The certificate asks for a node here precisely when the two bubbles do
                // not meet, which is precisely when the straight line between them is not
                // known to be in free space.  So the midpoint is the one point in this
                // whole algorithm most likely to be inside an obstacle, and inserting it
                // unchecked is how a node ends up there.  Leave the gap uncertified
                // instead: certificate_holds() then says so.
                if (!inside_area(middle, rings))
                {
                    ++i;
                    continue;
                }
                band.points.insert(band.points.begin() + static_cast<std::ptrdiff_t>(i) + 1,
                                   middle);
                band.radii.insert(band.radii.begin() + static_cast<std::ptrdiff_t>(i) + 1, 0.0);
                recompute(i + 1);
                // Do not advance: the new node may itself not reach its neighbour, and a
                // pathological gap would otherwise be halved once and left.
                if (band.points.size() > MAX_NODES)
                {
                    break;
                }
                continue;
            }
            ++i;
        }

        for (std::size_t i = 1; i + 1 < band.points.size();)
        {
            const auto without = length(band.points[i + 1] - band.points[i - 1]);
            // Redundant for the certificate, since the neighbours already reach each
            // other, and not needed to carry the shape either.
            //
            // The second condition is not in the textbook and the band does not work
            // without it.  Bubbles in the open are enormous -- tens of metres across a
            // square -- so by the certificate alone every interior node is redundant and
            // a twenty node band collapses to three in one sweep.  The certificate stays
            // satisfied and the path stops being able to represent a curve at all.  A
            // node is kept if dropping it would stretch the gap past the sampling the
            // band was asked for.
            const auto redundant = without < band.radii[i - 1] + band.radii[i + 1];
            // Not coarser than the sampling asked for.  Letting the band thin out where
            // the free discs are large was tried, on the argument that density is only
            // needed where the corridor is tight; it produces long segments in the open
            // meeting short ones at an obstacle, and the transition between them is a
            // spike. Worst turn went from 47 degrees to 160.
            const auto sampled_enough = !(spacing > 0.0) || without <= spacing;
            if (redundant && sampled_enough)
            {
                band.points.erase(band.points.begin() + static_cast<std::ptrdiff_t>(i));
                band.radii.erase(band.radii.begin() + static_cast<std::ptrdiff_t>(i));
                continue;
            }
            ++i;
        }
    }

    // Hand back nothing that cannot be proved.
    //
    // Where the certificate holds, consecutive discs overlap, the segment between them
    // lies in their union, and the path is in free space by induction.  Where it does not
    // hold, that argument is unavailable and the segment may cut across an obstacle: over
    // the corpus one did, winding cleanly round the far side of it.  A band that has
    // wandered into a different homotopy class is worse than no smoothing at all, because
    // the planner's choice of which side to pass is the part a person notices.
    //
    // So an uncertified result is discarded and the path is returned as it came.  This is
    // the case erosion removes: with the geometry offset by the hard margin, no interior
    // node starts on it, every bubble has a radius, and the certificate holds throughout.
    // The two segments the certificate cannot speak for: an anchor sits where it was
    // asked to, which on a portal is the boundary itself, so its bubble has no radius and
    // can overlap nothing.  Those two are checked outright instead.  They are also the
    // ones most able to do damage, being the longest and the least constrained, and over
    // the corpus one of them swept clean across an obstacle and came out the far side.
    band.certified = certificate_holds(band, rings);
    if (const auto *why = std::getenv("OSRM_BAND_WHY"); why != nullptr)
    {
        std::fprintf(stderr,
                     "BAND n=%zu certified=%d\n",
                     band.points.size(),
                     static_cast<int>(band.certified));
    }
    if (!band.certified)
    {
        band.points.assign(path.begin(), path.end());
        band.radii.assign(band.points.size(), 0.0);
        for (std::size_t i = 0; i < band.points.size(); ++i)
        {
            recompute(i);
        }
        // The input is handed back as it came, and it gets the same examination as the
        // band did.  It is a taut path, so usually it grazes the geometry and does not
        // certify either; saying so is the point.  A caller that may only walk on proved
        // ground reads this and keeps whatever it had.
        band.certified = certificate_holds(band, rings);
    }

    return band;
}
} // namespace

Band smooth(std::span<const Point> path,
            std::span<const Ring> rings,
            const BandParameters &parameters)
{
    Band band;
    if (path.size() < 2)
    {
        band.points.assign(path.begin(), path.end());
        band.radii.assign(path.size(), 0.0);
        return band;
    }

    // Resample finely enough that the band has something to bend with.  A spacing wider
    // than the path leaves three nodes, which is not a curve and is also where the
    // certificate has least to say.
    // A quarter of the comfort margin unless told otherwise; see BandParameters::spacing.
    auto spacing = parameters.spacing > 0.0 ? parameters.spacing : parameters.comfort * 0.25;
    auto total = 0.0;
    for (std::size_t i = 0; i + 1 < path.size(); ++i)
    {
        total += length(path[i + 1] - path[i]);
    }
    if (total > 0.0)
    {
        constexpr std::size_t MINIMUM_NODES = 8;
        spacing = spacing > 0.0 ? std::min(spacing, total / MINIMUM_NODES) : total / MINIMUM_NODES;
    }

    /*
     * Coarse to fine, rather than relaxing the finished resolution and hoping.
     *
     * Tension is a diffusion, and Gauss-Seidel moves information one node per sweep, so a
     * band of n nodes needs on the order of n sweeps before its two ends know about each
     * other.  At the spacing this runs at that is a couple of hundred, against a budget of
     * sixty, and the shortfall does not show up as a visibly kinked path: it shows up as a
     * long slow wobble, a band that never settles which way it is bending.  Measured on
     * the corpus at a single resolution, a band swept 145 degrees in total where the taut
     * path it came from swept 36, and it changed its bending direction five times per path
     * against the taut path's zero, while ending up pointing the same way to within four
     * degrees.  All of that turning cancels out.  It is the low frequencies that are slow,
     * and raising the budget barely touches them: sixteen times the sweeps, at sixteen
     * times the cost, only halved the excess.
     *
     * So the low frequencies are settled where they are cheap.  Relax a band of eight
     * nodes, which converges end to end in a few sweeps because it is eight nodes; use it
     * as the starting shape for one of sixteen, and so on down to the target spacing.
     * Each level only has to resolve the detail the level above could not represent, which
     * is local, and local is what a fixed sweep budget is good at.  The levels above the
     * finest cost almost nothing, since a level with half the nodes costs half as much and
     * the series sums to less than the finest level alone.
     *
     * Every level is a real band and refuses the same moves, so the homotopy argument is
     * unchanged: the path cannot cross geometry at any resolution, and the side the
     * planner chose is still the side that comes out.
     */
    constexpr std::size_t COARSEST_NODES = 8;
    std::vector<double> levels;
    if (total > 0.0)
    {
        for (auto coarse = total / COARSEST_NODES; coarse > spacing * 1.5; coarse *= 0.5)
        {
            levels.push_back(coarse);
        }
        std::reverse(levels.begin(), levels.end());
    }
    levels.push_back(spacing);

    std::vector<Point> working{path.begin(), path.end()};
    for (const auto level : levels)
    {
        band = relax(working, rings, parameters, level);
        working = band.points;
    }
    return band;
}

} // namespace osrm::engine::area
