#include "engine/area_fillet.hpp"

#include "engine/area_clearance.hpp"

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

Band as_given(std::span<const Point> path, std::span<const Ring> rings)
{
    Band band;
    band.points.assign(path.begin(), path.end());
    band.radii.assign(path.size(), 0.0);
    band.certified = certificate_holds(band, rings);
    return band;
}

} // namespace

Band round_corners(std::span<const Point> path, std::span<const Ring> rings, const double margin)
{
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
            Band band;
            band.points = std::move(rounded.points);
            band.radii.assign(band.points.size(), 0.0);
            band.certified = true;
            return band;
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

} // namespace osrm::engine::area
