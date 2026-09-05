#include "engine/area_fillet.hpp"

#include "engine/area_clearance.hpp"

#include "util/web_mercator.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace osrm::engine::area
{

namespace
{

Point operator-(const Point &a, const Point &b) { return {a.x - b.x, a.y - b.y}; }
Point operator+(const Point &a, const Point &b) { return {a.x + b.x, a.y + b.y}; }
Point operator*(const Point &a, const double s) { return {a.x * s, a.y * s}; }
double length(const Point &p) { return std::hypot(p.x, p.y); }
double dot(const Point &a, const Point &b) { return a.x * b.x + a.y * b.y; }
double cross(const Point &a, const Point &b) { return a.x * b.y - a.y * b.x; }
Point unit(const Point &p)
{
    const auto len = length(p);
    return len > 0.0 ? Point{p.x / len, p.y / len} : Point{0.0, 0.0};
}

//! An arc is sampled this finely, as a fraction of the margin; the drawing is thinned
//! afterwards to what can be seen, so this only has to keep the chords off the geometry
//! and the turn per sample gentle: an eighth of the margin on a margin's radius is seven
//! degrees a sample.
constexpr double ARC_STEP = 0.125;

//! How many points along the bisector are tried for a corner's offset.
constexpr std::size_t OFFSET_SAMPLES = 8;

//! A run from an anchor carries at most this much of itself as tangent length, so a
//! straight stretch always remains at the anchor and the arc never ends on it.
constexpr double ANCHOR_ROOM = 0.9;

//! An offset run has to keep heading the way the taut run did, within this cosine, and
//! keep this much of its length.  Two corners close together and pushed apart can swing
//! the run between them round or fold it up, and the arcs either side then meet in a
//! cusp: a 173 degree spike on a path that was otherwise two smooth curves.
constexpr double KEEP_HEADING = 0.5;
constexpr double KEEP_LENGTH = 0.2;

//! An offset corner may turn at most this much more, in radians, than the taut corner
//! did.  An anchor stays put while the corner beside it moves out, so that corner's turn
//! grows, and a corner turning 73 degrees on the taut path was turning 163 on the offset
//! one: an arc through that, with the anchor run too short to carry any radius, is a
//! hook.  The offset is for keeping the runs off the walls, not for sharpening corners.
constexpr double TURN_SLACK = 15.0 * M_PI / 180.0;

double turn_at(std::span<const Point> points, const std::size_t i)
{
    const auto a = unit(points[i] - points[i - 1]);
    const auto b = unit(points[i + 1] - points[i]);
    if (!(length(a) > 0.0) || !(length(b) > 0.0))
    {
        return 0.0;
    }
    return std::acos(std::clamp(dot(a, b), -1.0, 1.0));
}

//! A corner's offset along its bisector is `margin / cos(turn / 2)`, which runs away as
//! the turn approaches a reversal; capped here at about four margins.
constexpr double OFFSET_CAP = 4.0;

//! Each corner is scaled down by this factor when it fails, and given up on below the
//! floor, so a corner is retried at most eight or so times.
constexpr double BACK_OFF = 0.5;
constexpr double GIVE_UP = 1e-2;

/**
 * The path with anchors as given and each corner moved out along its bisector: as far as
 * the first point with the margin of room around it, or the point with the most room if
 * none has that much, and never further than would put both runs the margin off the
 * edges they were grazing.
 *
 * The room is what decides it.  Going the full distance regardless put a corner in a
 * three metre corridor against the far wall, and a corner turning 126 degrees eleven
 * metres out into the plaza with the path 1.8 times its length: the margin is a distance
 * to keep from the geometry, not a distance to travel.  In a passage narrower than twice
 * the margin this settles on the widest point, which is the centreline.
 */
std::vector<Point> offset_corners(std::span<const Point> path,
                                  std::span<const Ring> rings,
                                  std::span<const double> scales,
                                  const double margin)
{
    std::vector<Point> out(path.begin(), path.end());
    for (std::size_t i = 1; i + 1 < path.size(); ++i)
    {
        const auto a = unit(path[i] - path[i - 1]);
        const auto b = unit(path[i + 1] - path[i]);
        // A taut path bends around what it turns at, so the geometry is on the inside of
        // the turn and the way off it is the outside bisector.
        const auto outward = unit(a - b);
        if (!(length(outward) > 0.0) || !(scales[i] > 0.0))
        {
            continue;
        }
        // moving `d` along the bisector puts the point `d cos(turn/2)` off each run
        const auto wanted = margin * scales[i];
        const auto cos_half = std::sqrt(std::max(0.0, (1.0 + dot(a, b)) * 0.5));
        const auto furthest = std::min(wanted / std::max(cos_half, 1e-6), wanted * OFFSET_CAP);

        auto best = path[i];
        auto most_room = -1.0;
        for (std::size_t k = 1; k <= OFFSET_SAMPLES; ++k)
        {
            const auto candidate =
                path[i] + outward * (furthest * static_cast<double>(k) /
                                     static_cast<double>(OFFSET_SAMPLES));
            const auto here = clearance(candidate, rings);
            if (!here.inside)
            {
                break;
            }
            if (here.distance >= wanted)
            {
                best = candidate;
                break;
            }
            if (here.distance > most_room)
            {
                most_room = here.distance;
                best = candidate;
            }
        }
        out[i] = best;
    }
    return out;
}

/**
 * The rounded path, and for each of its points which corner of the input produced it,
 * so that a segment found outside the free space can be charged to a corner.
 */
struct Rounded
{
    std::vector<Point> points;
    std::vector<std::size_t> owner;

    //! Two fillets on one run meet exactly at its midpoint, tangent point on tangent
    //! point, and the segment between two copies of a point has whatever direction the
    //! rounding left it: a corner of 178 degrees that is nowhere to be seen.  A point
    //! that repeats the previous one is not added.
    void add(const Point &p, const std::size_t corner, const double tolerance)
    {
        if (!points.empty() && length(p - points.back()) <= tolerance)
        {
            return;
        }
        points.push_back(p);
        owner.push_back(corner);
    }
};

Rounded round(std::span<const Point> path,
              std::span<const Point> corners,
              std::span<const double> scales,
              const double margin)
{
    const auto n = corners.size();
    Rounded out;
    const auto same_place = margin * 1e-9;
    out.add(corners.front(), 0, same_place);
    for (std::size_t i = 1; i + 1 < n; ++i)
    {
        const auto a = unit(corners[i] - corners[i - 1]);
        const auto b = unit(corners[i + 1] - corners[i]);
        const auto turn = std::acos(std::clamp(dot(a, b), -1.0, 1.0));
        const auto orientation = cross(a, b);
        if (!(length(a) > 0.0) || !(length(b) > 0.0) || turn < 1e-4 || !(scales[i] > 0.0) ||
            orientation == 0.0)
        {
            out.add(corners[i], i, same_place);
            continue;
        }

        // A fillet of the margin's radius, unless the runs either side are too short to
        // carry its tangent length: a run between two corners has to carry half of it
        // for each, a run from an anchor all of it.
        const auto tan_half = std::tan(turn * 0.5);
        const auto cos_half = std::cos(turn * 0.5);
        const auto room_in = length(corners[i] - corners[i - 1]) * (i == 1 ? ANCHOR_ROOM : 0.5);
        const auto room_out =
            length(corners[i + 1] - corners[i]) * (i + 2 == n ? ANCHOR_ROOM : 0.5);
        // The arc's apex lies `radius (1 / cos(turn/2) - 1)` back towards the corner it
        // rounds from the offset point, so on a corner that could not be moved the whole
        // way the radius is held to what keeps the apex off the geometry.
        const auto offset = length(corners[i] - path[i]);
        const auto apex_limit = cos_half < 1.0 - 1e-9 ? offset * cos_half / (1.0 - cos_half)
                                                       : margin * scales[i];
        const auto tangent = std::min({std::min(margin * scales[i], apex_limit) * tan_half,
                                       room_in,
                                       room_out});
        const auto radius = tangent / tan_half;
        if (!(radius > 0.0))
        {
            out.add(corners[i], i, same_place);
            continue;
        }

        const auto from = corners[i] - a * tangent;
        const auto centre = corners[i] + unit(b - a) * (radius / std::cos(turn * 0.5));
        const auto samples =
            std::max(std::size_t{2},
                     static_cast<std::size_t>(std::ceil(radius * turn / (margin * ARC_STEP))));
        const auto spoke = from - centre;
        for (std::size_t k = 0; k <= samples; ++k)
        {
            const auto angle = turn * static_cast<double>(k) / static_cast<double>(samples) *
                               (orientation > 0.0 ? 1.0 : -1.0);
            const auto c = std::cos(angle), s = std::sin(angle);
            out.add(centre + Point{spoke.x * c - spoke.y * s, spoke.x * s + spoke.y * c},
                    i,
                    same_place);
        }
    }
    out.add(corners.back(), n - 1, same_place);
    return out;
}

RoundedPath as_given(std::span<const Point> path, std::span<const Ring> rings)
{
    RoundedPath out;
    out.points.assign(path.begin(), path.end());
    out.legal = path_in_closed_area(out.points, rings);
    return out;
}

//! Below this, in metres, a point's departure from the line drawn without it is drawing
//! nothing: a decimetre is the finest the API emits, polyline6.
constexpr double DRAWS_NOTHING_METRES = 0.1;

//! An obstacle no bigger across than this is street furniture, a planter, a tree pit,
//! a bench, a bollard, and a path may pass it on whichever side is shorter.  Above it
//! is a kiosk, a fountain, a building, and the path keeps the side it was given.
//! Measured on Ile-de-France plazas: the planters at corridor junctions are 5 to 7 m
//! across their bounding box, tree pits about 1 m, the smallest fountains over 10.
constexpr double STREET_FURNITURE_METRES = 8.0;

/**
 * Douglas-Peucker: keep the fewest points that leave every dropped one within the
 * tolerance of the line drawn through the kept ones.  Recursive on the farthest point,
 * which is the textbook form and is deterministic, since the farthest point is chosen by
 * strict comparison and a tie goes to the earlier index.
 */
void thin_between(std::span<const Point> points,
                  const std::size_t first,
                  const std::size_t last,
                  const double tolerance,
                  std::vector<bool> &kept)
{
    if (last <= first + 1)
    {
        return;
    }
    auto farthest = first;
    auto farthest_squared = 0.0;
    for (std::size_t i = first + 1; i < last; ++i)
    {
        const auto off = distance_squared_to_segment(points[i], points[first], points[last]);
        if (off > farthest_squared)
        {
            farthest_squared = off;
            farthest = i;
        }
    }
    if (farthest_squared > tolerance * tolerance)
    {
        kept[farthest] = true;
        thin_between(points, first, farthest, tolerance, kept);
        thin_between(points, farthest, last, tolerance, kept);
    }
}

std::vector<Point> thin(std::span<const Point> points, const double tolerance)
{
    std::vector<Point> out;
    if (points.empty())
    {
        return out;
    }
    std::vector<bool> kept(points.size(), false);
    kept.front() = kept.back() = true;
    thin_between(points, 0, points.size() - 1, tolerance, kept);
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        if (kept[i])
        {
            out.push_back(points[i]);
        }
    }
    return out;
}

} // namespace

namespace
{

//! How many steps a vertex is walked towards the chord between its neighbours per round.
constexpr std::size_t SLIDE_STEPS = 8;

//! Rounds of sliding and snapping before the string is called tight.
constexpr std::size_t TIGHTEN_ROUNDS = 16;

//! Below this fraction of the path's length, a round's gain is no gain.
constexpr double TIGHT_ENOUGH = 1e-9;

/**
 * Whether geometry inside a region stands in the way of replacing the path round it by
 * the chord that closes it.  The region is the polygon made of a stretch of the path and
 * the chord across it; a triangle when one vertex is dropped.
 *
 * A ring with some of its vertices inside and some outside pokes into the region, and
 * since the path and the chord are both legal it can only do so by being what the path
 * went round: the chord would pass it on the other side.  That blocks.  A ring wholly
 * inside is an obstacle the chord hops over entirely, which is a change of side too,
 * and blocks unless the ring is no bigger across than @p hop: a planter in a corridor
 * junction may be passed on whichever side is shorter, a building may not.  The outer
 * ring is never hopped.
 */
bool region_blocks(std::span<const Point> region, std::span<const Ring> rings, const double hop)
{
    const auto n = region.size();
    if (n < 3)
    {
        return false;
    }
    // Even-odd ray casting, which also answers for a region the path crosses back over.
    // A point on the region's boundary, which is where the vertices the path stands on
    // are, is neither inside nor outside: it does not poke in, and it does not stop a
    // ring from counting as wholly enclosed.
    enum class Where { outside, on_boundary, inside };
    const auto where = [&](const Point &p)
    {
        auto inside = false;
        for (std::size_t i = 0, j = n - 1; i < n; j = i++)
        {
            const auto &a = region[i];
            const auto &b = region[j];
            if (distance_squared_to_segment(p, a, b) <= 1e-24)
            {
                return Where::on_boundary;
            }
            if ((a.y > p.y) != (b.y > p.y) &&
                p.x < a.x + (b.x - a.x) * (p.y - a.y) / (b.y - a.y))
            {
                inside = !inside;
            }
        }
        return inside ? Where::inside : Where::outside;
    };
    for (std::size_t r = 0; r < rings.size(); ++r)
    {
        const auto &ring = rings[r];
        auto inside = std::size_t{0}, touching = std::size_t{0};
        for (const auto &p : ring)
        {
            const auto w = where(p);
            inside += w == Where::inside;
            touching += w == Where::on_boundary;
        }
        if (inside == 0)
        {
            continue;
        }
        if (r == 0 || inside + touching < ring.size())
        {
            return true;
        }
        auto low = ring.front(), high = ring.front();
        for (const auto &p : ring)
        {
            low = {std::min(low.x, p.x), std::min(low.y, p.y)};
            high = {std::max(high.x, p.x), std::max(high.y, p.y)};
        }
        if (length(high - low) > hop)
        {
            return true;
        }
    }
    return false;
}

bool triangle_blocks(const Point &a,
                     const Point &b,
                     const Point &c,
                     std::span<const Ring> rings,
                     const double hop)
{
    const Point triangle[] = {a, b, c};
    return region_blocks(triangle, rings, hop);
}

/**
 * Whether moving the vertex between @p u and @p w from @p from to @p to keeps the path
 * legal and in the same homotopy class: both new segments in the closed free space, and
 * nothing of the geometry inside the two triangles the move sweeps.  A segment test alone
 * would let the string jump over a small obstacle that lay wholly between its old and
 * new positions.
 */
bool clean_move(const Point &u,
                const Point &w,
                const Point &from,
                const Point &to,
                std::span<const Ring> rings,
                const double hop)
{
    return segment_in_closed_area(u, to, rings) && segment_in_closed_area(to, w, rings) &&
           segment_in_closed_area(from, to, rings) && !triangle_blocks(u, from, to, rings, hop) &&
           !triangle_blocks(w, from, to, rings, hop);
}

/**
 * Drop every vertex the path can do without, one at a time: a vertex goes when the
 * chord across it is in the closed free space and the triangle it closes holds no
 * geometry, until no vertex goes.
 *
 * Not "the farthest vertex a straight line reaches", which was the first version and
 * changed the answer: a path that went round the north of an obstacle, whose two ends
 * could see each other along its south, was pulled straight past it.  A chord being
 * legal says nothing about what lies between the chord and the path it replaces.  With
 * both segments to the vertex legal as well as the chord, no ring edge can cross the
 * triangle's boundary, so a ring inside it has a vertex inside it, and checking the
 * vertices is enough.
 */
//! How many consecutive vertices one chord may replace.  An obstacle the path wraps
//! with several vertices on its corners cannot be hopped one vertex at a time, since
//! the chord across any one of them cuts the obstacle itself; the chord has to reach
//! from before the obstacle to after it.
constexpr std::size_t DROP_AT_ONCE = 6;

std::vector<Point>
drop_redundant(std::span<const Point> path, std::span<const Ring> rings, const double hop)
{
    std::vector<Point> pulled(path.begin(), path.end());
    auto dropped = true;
    while (dropped && pulled.size() > 2)
    {
        dropped = false;
        for (std::size_t i = 1; i + 1 < pulled.size();)
        {
            // the longest stretch from i that one chord can replace, the region between
            // them holding nothing the path may not hop
            auto replaced = std::size_t{0};
            for (auto k = std::min(DROP_AT_ONCE, pulled.size() - 1 - i); k >= 1; --k)
            {
                const auto &u = pulled[i - 1];
                const auto &w = pulled[i + k];
                if (!segment_in_closed_area(u, w, rings))
                {
                    continue;
                }
                const std::span<const Point> region{pulled.data() + (i - 1), k + 2};
                if (!region_blocks(region, rings, hop))
                {
                    replaced = k;
                    break;
                }
            }
            if (replaced > 0)
            {
                pulled.erase(pulled.begin() + static_cast<std::ptrdiff_t>(i),
                             pulled.begin() + static_cast<std::ptrdiff_t>(i + replaced));
                dropped = true;
            }
            else
            {
                ++i;
            }
        }
    }
    return pulled;
}

double path_length(std::span<const Point> points)
{
    auto total = 0.0;
    for (std::size_t i = 1; i < points.size(); ++i)
    {
        total += length(points[i] - points[i - 1]);
    }
    return total;
}

} // namespace

std::vector<Point>
pull_taut(std::span<const Point> path, std::span<const Ring> rings, const double hop)
{
    auto pulled = drop_redundant(path, rings, hop);

    // Then tighten.  Dropping vertices leaves each survivor where the input had it,
    // which is off the obstacle it turns around by however far the input wandered, and
    // a path with that slack in it is not taut however few vertices it has.  So each
    // survivor is walked towards the chord between its neighbours as far as the move
    // stays clean, and offered the nearby corners of the geometry as places to stand;
    // whatever shortens the path and keeps it in its homotopy class is taken.  A vertex
    // that reaches the chord is redundant and the next round drops it.  This converges
    // on the locally shortest path through the same gaps the input threaded, which is
    // the taut string, with every remaining vertex on a corner of the geometry.
    for (std::size_t round = 0; round < TIGHTEN_ROUNDS && pulled.size() > 2; ++round)
    {
        const auto before = path_length(pulled);
        for (std::size_t i = 1; i + 1 < pulled.size(); ++i)
        {
            const auto &u = pulled[i - 1];
            const auto &w = pulled[i + 1];
            auto v = pulled[i];

            // slide towards the foot of the perpendicular on the chord, or the nearer
            // end of the chord when the foot is beyond it
            const auto chord = w - u;
            const auto chord_squared = dot(chord, chord);
            const auto t = chord_squared > 0.0
                               ? std::clamp(dot(v - u, chord) / chord_squared, 0.0, 1.0)
                               : 0.0;
            const auto foot = u + chord * t;
            for (std::size_t step = SLIDE_STEPS; step > 0; --step)
            {
                const auto candidate = v + (foot - v) * (static_cast<double>(step) /
                                                         static_cast<double>(SLIDE_STEPS));
                if (clean_move(u, w, v, candidate, rings, hop))
                {
                    v = candidate;
                    break;
                }
            }

            // and try the corners of the geometry nearest to where it now stands: a
            // string tightened against a corner rests on the corner, and the slide
            // above stops where a segment first grazes it, a little short of that
            auto best_length = length(v - u) + length(w - v);
            auto best = v;
            for (const auto &ring : rings)
            {
                for (const auto &corner : ring)
                {
                    const auto candidate_length = length(corner - u) + length(w - corner);
                    if (candidate_length < best_length && clean_move(u, w, v, corner, rings, hop))
                    {
                        best_length = candidate_length;
                        best = corner;
                    }
                }
            }
            pulled[i] = best;
        }
        pulled = drop_redundant(pulled, rings, hop);
        if (before - path_length(pulled) <= TIGHT_ENOUGH * before)
        {
            break;
        }
    }
    return pulled;
}

RoundedPath round_corners(std::span<const Point> given,
                          std::span<const Ring> rings,
                          const double margin,
                          const double hop)
{
    const auto pulled = pull_taut(given, rings, hop);
    const std::span<const Point> path{pulled};
    const auto n = path.size();
    if (n < 3 || !(margin > 0.0))
    {
        return as_given(path, rings);
    }

    std::vector<double> scales(n, 1.0);
    // Bounded: every failure halves a corner, and a corner below the floor is given up.
    const auto attempts = n * 12;
    for (std::size_t attempt = 0; attempt < attempts; ++attempt)
    {
        const auto corners = offset_corners(path, rings, scales, margin);

        // A run that no longer heads where it did is charged to both its corners before
        // any arc is drawn on it.
        auto swung = false;
        for (std::size_t i = 0; i + 1 < n; ++i)
        {
            const auto was = path[i + 1] - path[i];
            const auto now = corners[i + 1] - corners[i];
            const auto lengths = length(was) * length(now);
            if (!(length(was) > 0.0))
            {
                continue;
            }
            if (length(now) < KEEP_LENGTH * length(was) ||
                (lengths > 0.0 && dot(was, now) < KEEP_HEADING * lengths))
            {
                for (const auto corner : {i, i + 1})
                {
                    if (corner == 0 || corner + 1 >= n || !(scales[corner] > 0.0))
                    {
                        continue;
                    }
                    scales[corner] =
                        scales[corner] * BACK_OFF < GIVE_UP ? 0.0 : scales[corner] * BACK_OFF;
                    swung = true;
                }
            }
        }
        for (std::size_t i = 1; i + 1 < n; ++i)
        {
            if (scales[i] > 0.0 && turn_at(corners, i) > turn_at(path, i) + TURN_SLACK)
            {
                scales[i] = scales[i] * BACK_OFF < GIVE_UP ? 0.0 : scales[i] * BACK_OFF;
                swung = true;
            }
        }
        if (swung)
        {
            continue;
        }

        auto rounded = round(path, corners, scales, margin);

        auto bad = rounded.points.size();
        for (std::size_t s = 0; s + 1 < rounded.points.size(); ++s)
        {
            if (!segment_in_closed_area(rounded.points[s], rounded.points[s + 1], rings))
            {
                bad = s;
                break;
            }
        }
        if (bad == rounded.points.size())
        {
            RoundedPath out;
            out.points = std::move(rounded.points);
            out.legal = true;
            return out;
        }

        // Charge the corner or corners that made this segment: the arc it is part of,
        // or the two corners a run between arcs joins.
        auto charged = false;
        for (const auto corner : {rounded.owner[bad], rounded.owner[bad + 1]})
        {
            if (corner == 0 || corner + 1 >= n || !(scales[corner] > 0.0))
            {
                continue;
            }
            scales[corner] = scales[corner] * BACK_OFF < GIVE_UP ? 0.0 : scales[corner] * BACK_OFF;
            charged = true;
        }
        if (!charged)
        {
            // Nothing left to give: the taut path itself is what is illegal here.
            break;
        }
    }
    return as_given(path, rings);
}

std::optional<std::vector<util::Coordinate>>
round_corners(const std::vector<std::span<const util::Coordinate>> &rings,
              std::span<const util::Coordinate> path,
              const double margin_metres)
{
    if (!(margin_metres > 0.0) || path.size() < 2)
    {
        return std::nullopt;
    }

    std::vector<std::vector<Point>> storage;
    storage.reserve(rings.size());
    for (const auto &ring : rings)
    {
        std::vector<Point> points;
        points.reserve(ring.size());
        for (const auto coordinate : ring)
        {
            points.push_back(project(coordinate));
        }
        storage.push_back(std::move(points));
    }
    // only now that storage has stopped growing, so the spans stay valid
    std::vector<Ring> views;
    views.reserve(storage.size());
    for (const auto &points : storage)
    {
        views.emplace_back(points);
    }

    std::vector<Point> taut;
    taut.reserve(path.size());
    for (const auto coordinate : path)
    {
        taut.push_back(project(coordinate));
    }

    const auto metres_per_unit =
        metres_per_projected_unit(static_cast<double>(util::toFloating(path.front().lat)));
    const auto rounded = round_corners(
        taut, views, margin_metres / metres_per_unit, STREET_FURNITURE_METRES / metres_per_unit);
    if (!rounded.legal)
    {
        return std::nullopt;
    }
    const auto tolerance = DRAWS_NOTHING_METRES / metres_per_unit;
    const auto drawn = thin(rounded.points, tolerance);

    // The same shape as it was given, to within what can be drawn: say so, rather than
    // hand back a copy that differs in the last decimal and gets carried as computed.
    if (drawn.size() == taut.size())
    {
        auto same = true;
        for (std::size_t i = 0; i < drawn.size() && same; ++i)
        {
            same = length(drawn[i] - taut[i]) <= tolerance;
        }
        if (same)
        {
            return std::nullopt;
        }
    }

    std::vector<util::Coordinate> out;
    out.reserve(drawn.size());
    out.push_back(path.front());
    for (std::size_t i = 1; i + 1 < drawn.size(); ++i)
    {
        out.emplace_back(util::FloatLongitude{drawn[i].x}, util::web_mercator::yToLat(drawn[i].y));
    }
    out.push_back(path.back());
    return out;
}

} // namespace osrm::engine::area
