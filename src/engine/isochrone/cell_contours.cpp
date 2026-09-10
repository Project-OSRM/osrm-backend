#include "engine/isochrone/cell_contours.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace osrm::engine::isochrone
{
namespace
{

struct GridPointHash
{
    std::size_t operator()(const GridPoint point) const
    {
        const auto x = static_cast<std::uint64_t>(point.x);
        const auto y = static_cast<std::uint64_t>(point.y);
        return static_cast<std::size_t>((x << 32) ^ (x >> 32) ^ y ^ (y >> 32));
    }
};

struct BoundaryEdge
{
    GridPoint from;
    GridPoint to;
    int component;
    bool used = false;
};

struct ComponentRing
{
    GridRing ring;
    int component;
};

struct ComponentPolygon
{
    GridPolygon polygon;
    int component;
};

double signedArea(const GridRing &ring)
{
    double twice_area = 0.;
    for (std::size_t index = 0; index + 1 < ring.size(); ++index)
    {
        const auto &from = ring[index];
        const auto &to = ring[index + 1];
        twice_area += static_cast<double>(from.x) * to.y - static_cast<double>(to.x) * from.y;
    }
    return twice_area / 2.;
}

bool pointLess(const GridPoint left, const GridPoint right)
{ return left.x < right.x || (left.x == right.x && left.y < right.y); }

// A boundary walker starts a ring at whichever edge it encounters first.  Make
// that implementation detail invisible to callers: every (already oriented)
// ring starts at its lexicographically least vertex.  This is also useful to
// callers that compare repeated contours byte-for-byte.
GridRing canonicalizeRing(GridRing ring)
{
    BOOST_ASSERT(ring.size() >= 4);
    BOOST_ASSERT(ring.front() == ring.back());

    const auto first = std::min_element(ring.begin(),
                                        ring.end() - 1,
                                        [](const GridPoint left, const GridPoint right)
                                        { return pointLess(left, right); });
    std::rotate(ring.begin(), first, ring.end() - 1);
    ring.back() = ring.front();
    return ring;
}

bool ringLess(const GridRing &left, const GridRing &right)
{
    BOOST_ASSERT(!left.empty());
    BOOST_ASSERT(!right.empty());
    return pointLess(left.front(), right.front());
}

bool hasCapacity(const std::size_t used, const std::size_t additional, const std::size_t maximum)
{ return used <= maximum && additional <= maximum - used; }

GridRing simplifyRing(GridRing ring)
{
    BOOST_ASSERT(ring.size() >= 4);
    BOOST_ASSERT(ring.front() == ring.back());

    ring.pop_back();
    GridRing simplified;
    simplified.reserve(ring.size() + 1);

    for (std::size_t index = 0; index < ring.size(); ++index)
    {
        const auto &previous = ring[(index + ring.size() - 1) % ring.size()];
        const auto &current = ring[index];
        const auto &next = ring[(index + 1) % ring.size()];
        const auto incoming_x = current.x - previous.x;
        const auto incoming_y = current.y - previous.y;
        const auto outgoing_x = next.x - current.x;
        const auto outgoing_y = next.y - current.y;

        // Removing a 180-degree reversal would turn a valid hairpin into a
        // degenerate ring. Marching-squares contours only simplify runs that
        // continue in the same direction.
        const auto cross = incoming_x * outgoing_y - incoming_y * outgoing_x;
        const auto dot = incoming_x * outgoing_x + incoming_y * outgoing_y;
        if (cross == 0 && dot > 0)
            continue;

        simplified.push_back(current);
    }

    if (!simplified.empty())
        simplified.push_back(simplified.front());
    return simplified;
}

int turnPriority(const BoundaryEdge &from, const BoundaryEdge &to, const bool prefer_right)
{
    const auto incoming_x = from.to.x - from.from.x;
    const auto incoming_y = from.to.y - from.from.y;
    const auto outgoing_x = to.to.x - to.from.x;
    const auto outgoing_y = to.to.y - to.from.y;
    const auto cross = incoming_x * outgoing_y - incoming_y * outgoing_x;
    const auto dot = incoming_x * outgoing_x + incoming_y * outgoing_y;

    if (cross > 0)
        return prefer_right ? 1 : 3;
    if (dot > 0)
        return 2;
    if (cross < 0)
        return prefer_right ? 3 : 1;
    return 0;
}

} // namespace

std::optional<std::vector<GridPolygon>> buildCellContours(const std::span<const double> values,
                                                          const std::size_t width,
                                                          const std::size_t height,
                                                          const double cutoff,
                                                          const std::size_t maximum_coordinates)
{
    BOOST_ASSERT(width <= static_cast<std::size_t>(std::numeric_limits<int>::max()));
    BOOST_ASSERT(height <= static_cast<std::size_t>(std::numeric_limits<int>::max()));
    BOOST_ASSERT(width == 0 || height <= std::numeric_limits<std::size_t>::max() / width);
    BOOST_ASSERT(values.size() == width * height);

    if (width == 0 || height == 0)
        return std::vector<GridPolygon>{};

    const auto is_inside = [&](const std::size_t x, const std::size_t y)
    { return values[y * width + x] <= cutoff; };

    // Contour the exact union of four-connected occupied cells. At an
    // alternating diagonal corner we need to know whether the two occupied
    // cells belong to one component: one component keeps its exterior and
    // interior cycles distinct, while two components stay distinct.
    std::vector<int> components(values.size(), -1);
    std::vector<std::size_t> worklist;
    int next_component = 0;
    for (std::size_t y = 0; y < height; ++y)
    {
        for (std::size_t x = 0; x < width; ++x)
        {
            const auto start = y * width + x;
            if (!is_inside(x, y) || components[start] >= 0)
                continue;

            BOOST_ASSERT(next_component < std::numeric_limits<int>::max());
            components[start] = next_component++;
            worklist.clear();
            worklist.push_back(start);
            for (std::size_t index = 0; index < worklist.size(); ++index)
            {
                const auto cell = worklist[index];
                const auto cell_x = cell % width;
                const auto cell_y = cell / width;
                const auto visit = [&](const std::size_t neighbor)
                {
                    if (components[neighbor] < 0)
                    {
                        components[neighbor] = components[start];
                        worklist.push_back(neighbor);
                    }
                };
                if (cell_y > 0 && is_inside(cell_x, cell_y - 1))
                    visit(cell - width);
                if (cell_x + 1 < width && is_inside(cell_x + 1, cell_y))
                    visit(cell + 1);
                if (cell_y + 1 < height && is_inside(cell_x, cell_y + 1))
                    visit(cell + width);
                if (cell_x > 0 && is_inside(cell_x - 1, cell_y))
                    visit(cell - 1);
            }
        }
    }

    std::vector<BoundaryEdge> edges;
    edges.reserve(4 * values.size());
    const auto point = [](const std::size_t x, const std::size_t y)
    { return GridPoint{static_cast<std::int64_t>(x), static_cast<std::int64_t>(y)}; };
    for (std::size_t y = 0; y < height; ++y)
    {
        for (std::size_t x = 0; x < width; ++x)
        {
            if (!is_inside(x, y))
                continue;
            const auto component = components[y * width + x];
            BOOST_ASSERT(component >= 0);

            // Direct each cell boundary counter-clockwise, keeping the
            // occupied cell on the left. Shared edges are internal and do
            // not contribute to the union boundary.
            if (y == 0 || !is_inside(x, y - 1))
                edges.push_back({point(x, y), point(x + 1, y), component, false});
            if (x + 1 == width || !is_inside(x + 1, y))
                edges.push_back({point(x + 1, y), point(x + 1, y + 1), component, false});
            if (y + 1 == height || !is_inside(x, y + 1))
                edges.push_back({point(x + 1, y + 1), point(x, y + 1), component, false});
            if (x == 0 || !is_inside(x - 1, y))
                edges.push_back({point(x, y + 1), point(x, y), component, false});
        }
    }

    std::unordered_multimap<GridPoint, std::size_t, GridPointHash> outgoing;
    outgoing.reserve(edges.size());
    for (std::size_t index = 0; index < edges.size(); ++index)
        outgoing.emplace(edges[index].from, index);

    std::vector<ComponentRing> rings;
    std::size_t coordinate_count = 0;
    for (std::size_t first_edge = 0; first_edge < edges.size(); ++first_edge)
    {
        if (edges[first_edge].used)
            continue;

        GridRing ring;
        ring.reserve(16);
        const auto start = edges[first_edge].from;
        const auto component = edges[first_edge].component;
        auto edge_index = first_edge;
        ring.push_back(start);

        for (std::size_t step = 0; step <= edges.size(); ++step)
        {
            auto &edge = edges[edge_index];
            BOOST_ASSERT(!edge.used);
            edge.used = true;
            ring.push_back(edge.to);

            if (edge.to == start)
                break;

            const auto [first, last] = outgoing.equal_range(edge.to);
            auto next_edge = last;
            auto next_priority = -1;
            std::size_t candidate_count = 0;
            for (auto candidate = first; candidate != last; ++candidate)
            {
                const auto candidate_index = candidate->second;
                if (!edges[candidate_index].used &&
                    edges[candidate_index].component == edge.component)
                    ++candidate_count;
            }
            for (auto candidate = first; candidate != last; ++candidate)
            {
                const auto candidate_index = candidate->second;
                if (edges[candidate_index].used ||
                    edges[candidate_index].component != edge.component)
                    continue;
                const auto priority =
                    turnPriority(edge, edges[candidate_index], candidate_count > 1);
                if (priority > next_priority)
                {
                    next_edge = candidate;
                    next_priority = priority;
                }
            }
            BOOST_ASSERT(next_edge != last);
            if (next_edge == last)
                return {};
            edge_index = next_edge->second;
        }

        BOOST_ASSERT(ring.front() == ring.back());
        if (ring.size() >= 4 && ring.front() == ring.back())
        {
            auto simplified = simplifyRing(std::move(ring));
            if (simplified.size() >= 4)
            {
                if (!hasCapacity(coordinate_count, simplified.size(), maximum_coordinates))
                    return std::nullopt;
                coordinate_count += simplified.size();
                rings.push_back({canonicalizeRing(std::move(simplified)), component});
            }
        }
    }

    std::vector<ComponentPolygon> polygons;
    std::vector<ComponentRing> holes;
    std::vector<std::size_t> polygon_indices(static_cast<std::size_t>(next_component),
                                             std::numeric_limits<std::size_t>::max());
    for (auto &ring : rings)
    {
        if (signedArea(ring.ring) > 0.)
        {
            const auto polygon_index = polygons.size();
            BOOST_ASSERT(ring.component >= 0);
            BOOST_ASSERT(static_cast<std::size_t>(ring.component) < polygon_indices.size());
            BOOST_ASSERT(polygon_indices[ring.component] ==
                         std::numeric_limits<std::size_t>::max());
            polygon_indices[ring.component] = polygon_index;
            polygons.push_back({{std::move(ring.ring), {}}, ring.component});
        }
        else
            holes.push_back(std::move(ring));
    }

    for (auto &hole : holes)
    {
        BOOST_ASSERT(hole.component >= 0);
        BOOST_ASSERT(static_cast<std::size_t>(hole.component) < polygon_indices.size());
        const auto polygon_index = polygon_indices[hole.component];
        BOOST_ASSERT(polygon_index != std::numeric_limits<std::size_t>::max());
        if (polygon_index == std::numeric_limits<std::size_t>::max())
            return {};
        polygons[polygon_index].polygon.holes.push_back(std::move(hole.ring));
    }

    for (auto &polygon : polygons)
        std::sort(polygon.polygon.holes.begin(), polygon.polygon.holes.end(), ringLess);
    std::sort(polygons.begin(),
              polygons.end(),
              [](const ComponentPolygon &left, const ComponentPolygon &right)
              { return ringLess(left.polygon.outer, right.polygon.outer); });

    std::vector<GridPolygon> result;
    result.reserve(polygons.size());
    for (auto &polygon : polygons)
        result.push_back(std::move(polygon.polygon));
    return result;
}

std::vector<GridPolygon> buildCellContours(const std::span<const double> values,
                                           const std::size_t width,
                                           const std::size_t height,
                                           const double cutoff)
{
    auto contours =
        buildCellContours(values, width, height, cutoff, std::numeric_limits<std::size_t>::max());
    BOOST_ASSERT(contours);
    return std::move(*contours);
}

} // namespace osrm::engine::isochrone
