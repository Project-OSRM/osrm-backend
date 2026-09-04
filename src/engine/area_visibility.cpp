#include "engine/area_visibility.hpp"

#include "extractor/area/util.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace osrm::engine::area
{

double distance_squared_to_segment(const Point &point, const Point &a, const Point &b)
{
    const auto dx = b.x - a.x, dy = b.y - a.y;
    const auto length_squared = dx * dx + dy * dy;
    auto t = 0.0;
    if (length_squared > 0.0)
    {
        t = std::clamp(((point.x - a.x) * dx + (point.y - a.y) * dy) / length_squared, 0.0, 1.0);
    }
    const auto ex = point.x - (a.x + t * dx), ey = point.y - (a.y + t * dy);
    return ex * ex + ey * ey;
}

namespace
{
/** Call `function` for every edge of the ring, closing it. */
template <typename Fun> void for_each_edge(Ring ring, Fun function)
{
    for (std::size_t i = 0; i < ring.size(); ++i)
        function(ring[i], ring[(i + 1) % ring.size()]);
}

/** Whether `point` lies on the boundary of any ring, to within `tolerance`. */
bool on_any_ring(const Point &point, std::span<const Ring> rings, const double tolerance)
{
    const auto limit = tolerance * tolerance;
    for (const Ring &ring : rings)
    {
        // A plain loop rather than for_each_edge, which cannot stop early.  This runs
        // once per vertex from visible_vertices, so scanning a whole ring after the
        // answer is known costs the snapping path a factor of the ring's length.
        for (std::size_t i = 0; i < ring.size(); ++i)
        {
            if (distance_squared_to_segment(point, ring[i], ring[(i + 1) % ring.size()]) <= limit)
                return true;
        }
    }
    return false;
}

/**
 * How close to a ring counts as on it: a little relative to the size of what is being
 * measured, plus a floor for the degenerate case.  Projected coordinates are web mercator
 * degrees, so the absolute term is far below any real geometry.
 */
double on_ring_tolerance(const double scale) { return scale * 1e-9 + ON_GEOMETRY; }

double cross(const double ax, const double ay, const double bx, const double by)
{
    return ax * by - ay * bx;
}

} // namespace

bool in_closed_area(const Point &point, std::span<const Ring> rings, const double tolerance)
{
    return inside_area(point, rings) || on_any_ring(point, rings, tolerance);
}

bool segment_in_closed_area(const Point &from, const Point &to, std::span<const Ring> rings)
{
    const auto dx = to.x - from.x, dy = to.y - from.y;
    const auto span = std::hypot(dx, dy);
    const auto tolerance = on_ring_tolerance(span);
    if (!(span > 0.0))
    {
        return in_closed_area(from, rings, tolerance);
    }

    // Every parameter along the segment at which it might change sides.  The ends are
    // always cuts, so there is at least one piece to test.
    std::vector<double> cuts{0.0, 1.0};
    const auto cut_at = [&cuts](const double t)
    {
        if (t > 0.0 && t < 1.0)
        {
            cuts.push_back(t);
        }
    };

    for (const Ring &ring : rings)
    {
        for (std::size_t i = 0; i < ring.size(); ++i)
        {
            const Point &a = ring[i];
            const Point &b = ring[(i + 1) % ring.size()];
            const auto ex = b.x - a.x, ey = b.y - a.y;
            const auto denominator = cross(dx, dy, ex, ey);

            // Parallel, so the edge cannot carry the segment from one side to the other.
            // It can still bound a stretch that runs along it, and where such a stretch
            // begins and ends is exactly where the segment stops being on the boundary
            // and starts being inside or outside.  Those two ends are cuts.
            if (std::abs(denominator) <= span * std::hypot(ex, ey) * 1e-12)
            {
                for (const Point &end : {a, b})
                {
                    const auto along = ((end.x - from.x) * dx + (end.y - from.y) * dy);
                    if (std::abs(cross(end.x - from.x, end.y - from.y, dx, dy)) <=
                        tolerance * span)
                    {
                        cut_at(along / (span * span));
                    }
                }
                continue;
            }

            const auto t = cross(a.x - from.x, a.y - from.y, ex, ey) / denominator;
            const auto u = cross(a.x - from.x, a.y - from.y, dx, dy) / denominator;
            // Touching an end of the edge counts: that is the case where the segment
            // leaves a vertex of the ring, which is where the proper-crossing test goes
            // wrong and the reason this function exists.
            constexpr double SLACK = 1e-12;
            if (u >= -SLACK && u <= 1.0 + SLACK)
            {
                cut_at(t);
            }
        }
    }

    std::sort(cuts.begin(), cuts.end());
    for (std::size_t i = 0; i + 1 < cuts.size(); ++i)
    {
        const auto width = cuts[i + 1] - cuts[i];
        if (!(width * span > tolerance))
        {
            // Two cuts at the same place, or as near as makes no difference. There is no
            // piece between them to be inside anything.
            continue;
        }
        const auto middle = (cuts[i] + cuts[i + 1]) / 2;
        const Point at{from.x + dx * middle, from.y + dy * middle};
        if (!in_closed_area(at, rings, tolerance))
        {
            return false;
        }
    }
    return true;
}

bool path_in_closed_area(std::span<const Point> points, std::span<const Ring> rings)
{
    if (points.empty())
    {
        return true;
    }
    if (points.size() == 1)
    {
        return segment_in_closed_area(points.front(), points.front(), rings);
    }
    for (std::size_t i = 0; i + 1 < points.size(); ++i)
    {
        if (!segment_in_closed_area(points[i], points[i + 1], rings))
        {
            return false;
        }
    }
    return true;
}

bool crosses_ring(const Point &from, const Point &to, Ring ring)
{
    const auto same = [](const Point &p, const Point &q) { return p.x == q.x && p.y == q.y; };
    // A plain loop rather than for_each_edge, which cannot stop early.  visible_vertices
    // calls this once per vertex per ring, so a whole-ring scan after the first crossing
    // is found is a factor of the ring's length on the snapping path.
    for (std::size_t i = 0; i < ring.size(); ++i)
    {
        const Point &a = ring[i];
        const Point &b = ring[(i + 1) % ring.size()];
        // an edge sharing an endpoint with from..to meets it only there, and cannot
        // obstruct it -- the same reasoning as in the sweep
        if (same(a, from) || same(a, to) || same(b, from) || same(b, to))
            continue;
        if (extractor::area::intersect(&from, &to, &a, &b))
            return true;
    }
    return false;
}

bool inside_ring(const Point &point, Ring ring)
{
    // ray casting: count the edges crossed by a ray going in +x from the point
    bool inside = false;
    for_each_edge(ring,
                  [&](const Point &a, const Point &b)
                  {
                      if ((a.y > point.y) != (b.y > point.y) &&
                          point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x)
                          inside = !inside;
                  });
    return inside;
}

bool inside_area(const Point &point, std::span<const Ring> rings)
{
    if (rings.empty() || !inside_ring(point, rings.front()))
        return false;
    for (std::size_t i = 1; i < rings.size(); ++i)
        if (inside_ring(point, rings[i]))
            return false;
    return true;
}

std::vector<std::size_t> visible_vertices(const Point &point, std::span<const Ring> rings)
{
    std::vector<std::size_t> visible;
    std::size_t index = 0;
    for (const Ring &ring : rings)
    {
        for (const Point &vertex : ring)
        {
            // Does the segment run through the interior, or outside it?  One interior
            // sample answers that once crossings are ruled out below, because a segment
            // cannot leave the area and come back without crossing an edge.
            //
            // The sample has to tolerate the boundary.  inside_area is strict, and ray
            // casting on a point lying exactly on an edge answers arbitrarily, so a
            // midpoint that lands on a ring was read as outside.  That is not a rare case,
            // it is the commonest one there is: the midpoint of two adjacent vertices is
            // always on the ring between them.  The effect was to drop the walls from the
            // visibility graph, and a planner with no edge along a wall cannot route past
            // an obstacle, only around it.
            const Point midpoint{(point.x + vertex.x) / 2, (point.y + vertex.y) / 2};
            const auto tolerance =
                on_ring_tolerance(std::hypot(vertex.x - point.x, vertex.y - point.y));
            bool blocked = !in_closed_area(midpoint, rings, tolerance);
            for (const Ring &other : rings)
            {
                if (blocked)
                    break;
                blocked = crosses_ring(point, vertex, other);
            }
            if (!blocked)
                visible.push_back(index);
            ++index;
        }
    }
    return visible;
}

} // namespace osrm::engine::area
