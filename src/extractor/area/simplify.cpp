#include "extractor/area/simplify.hpp"

#include "extractor/area/util.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace osrm::extractor::area
{

namespace
{

//! A ring below this many vertices is not a ring any more.
constexpr std::size_t SMALLEST_RING = 3;

//! Metres per degree of latitude.  Longitude is this scaled by cos(latitude).
constexpr double METRES_PER_DEGREE = 111319.49079327358;

/**
 * @brief One ring, thinned.
 *
 * Straight Visvalingam-Whyatt: find the vertex whose triangle with its two neighbours is
 * smallest, drop it, and repeat.  Dropping a vertex changes what its neighbours' triangles
 * are, so their areas are recomputed, which is what makes this different from ranking every
 * vertex once against the original shape.
 *
 * The search for the smallest is a scan rather than a heap.  This runs once per area at
 * extraction, over rings of at most a few thousand vertices, so the quadratic term costs
 * a few million operations on the largest area in a region -- against a heap that would
 * need lazy deletion, and with it a tie-break that no longer depends only on the data.
 * Determinism is worth more here than the exponent: two runs over one input have to
 * produce the same file, or every downstream comparison stops meaning anything.
 *
 * Ties are broken by node id for the same reason.  Two vertices can easily have the same
 * effective area -- a rectangular kerb has four of them -- and "whichever the scan reached
 * first" is a property of the ring's rotation, not of the input.
 */
std::vector<osmium::NodeRef>
thin(const std::vector<osmium::NodeRef> &ring, const double threshold, const NodeRefSet &keep)
{
    if (ring.size() <= SMALLEST_RING)
    {
        return ring;
    }

    // `alive` indexes into `ring`; removing from the middle of a vector is cheaper than
    // the bookkeeping a linked list would need at these sizes
    std::vector<std::size_t> alive(ring.size());
    for (std::size_t i = 0; i < ring.size(); ++i)
    {
        alive[i] = i;
    }

    const auto pinned = [&keep, &ring](std::size_t index)
    { return keep.find(ring[index]) != keep.end(); };

    while (alive.size() > SMALLEST_RING)
    {
        auto smallest = std::numeric_limits<double>::infinity();
        std::size_t victim = alive.size();

        for (std::size_t i = 0; i < alive.size(); ++i)
        {
            if (pinned(alive[i]))
            {
                continue;
            }
            // the ring is closed, so the first and last vertices have neighbours too
            const auto &before = ring[alive[(i + alive.size() - 1) % alive.size()]];
            const auto &at = ring[alive[i]];
            const auto &after = ring[alive[(i + 1) % alive.size()]];

            const auto area = effective_area(before, at, after);
            if (area < smallest ||
                (area == smallest && victim < alive.size() && at.ref() < ring[alive[victim]].ref()))
            {
                smallest = area;
                victim = i;
            }
        }

        if (victim == alive.size() || smallest >= threshold)
        {
            // everything left either carries shape or has to be kept
            break;
        }
        alive.erase(alive.begin() + static_cast<std::ptrdiff_t>(victim));
    }

    std::vector<osmium::NodeRef> thinned;
    thinned.reserve(alive.size());
    for (const auto index : alive)
    {
        thinned.push_back(ring[index]);
    }
    return thinned;
}

/**
 * @brief Does the ring cross itself?
 *
 * Every pair of edges that do not share a vertex, tested with the area code's own
 * predicate rather than a new one: it is already careful about shared endpoints and about
 * behaving the same on x86_64 and ARM64, and a second opinion about what "crossing" means
 * is the last thing this needs.
 *
 * Quadratic, over a ring that has just been thinned, once per area at extraction.
 */
bool is_simple(const std::vector<osmium::NodeRef> &ring)
{
    const auto count = ring.size();
    for (std::size_t i = 0; i < count; ++i)
    {
        for (std::size_t j = i + 1; j < count; ++j)
        {
            // edges sharing a vertex meet there by construction
            if (j == i || (j + 1) % count == i || (i + 1) % count == j)
            {
                continue;
            }
            if (intersect(&ring[i], &ring[(i + 1) % count], &ring[j], &ring[(j + 1) % count]))
            {
                return false;
            }
        }
    }
    return true;
}

} // namespace

double effective_area(const osmium::NodeRef &before,
                      const osmium::NodeRef &at,
                      const osmium::NodeRef &after)
{
    // Equirectangular about the middle vertex.  Over a triangle whose sides are metres
    // this is exact enough that the error is far below the grid the coordinates are
    // stored on, and unlike a spherical formula it cannot lose precision on a sliver.
    const auto scale = std::cos(at.location().lat_without_check() * M_PI / 180.0);
    const auto x = [scale](const osmium::NodeRef &node)
    { return node.location().lon_without_check() * scale * METRES_PER_DEGREE; };
    const auto y = [](const osmium::NodeRef &node)
    { return node.location().lat_without_check() * METRES_PER_DEGREE; };

    return std::fabs((x(at) - x(before)) * (y(after) - y(before)) -
                     (x(after) - x(before)) * (y(at) - y(before))) /
           2.0;
}

OsmiumPolygon simplify(const OsmiumPolygon &poly, const double threshold, const NodeRefSet &keep)
{
    if (!(threshold > 0.0))
    {
        return poly;
    }

    OsmiumPolygon simplified;

    const std::vector<osmium::NodeRef> outer(poly.outer().begin(), poly.outer().end());
    const auto thinned_outer = thin(outer, threshold, keep);
    for (const auto &vertex : thinned_outer)
    {
        simplified.outer().push_back(vertex);
    }

    for (const auto &inner : poly.inners())
    {
        const std::vector<osmium::NodeRef> ring(inner.begin(), inner.end());
        auto thinned = thin(ring, threshold, keep);
        if (thinned.size() < SMALLEST_RING)
        {
            // an obstacle that thinned away is still an obstacle; keep it as drawn
            thinned = ring;
        }
        simplified.inners().emplace_back();
        for (const auto &vertex : thinned)
        {
            simplified.inners().back().push_back(vertex);
        }
    }

    // Visvalingam-Whyatt makes no promise about simplicity.  Dropping a vertex from a
    // narrow neck can walk one side of the ring through the other, and a plaza whose
    // outline crosses itself answers point-in-polygon and visibility in ways that are not
    // wrong so much as meaningless.  It is rare, it is cheap to detect on a ring that has
    // just been made smaller, and the polygon as drawn is always an acceptable answer, so
    // check and fall back rather than reason about when it cannot happen.
    if (!is_simple(thinned_outer))
    {
        return poly;
    }
    for (const auto &inner : simplified.inners())
    {
        if (!is_simple(std::vector<osmium::NodeRef>(inner.begin(), inner.end())))
        {
            return poly;
        }
    }

    return simplified;
}

} // namespace osrm::extractor::area
