#include "engine/isochrone/grid_polygon_generalization.hpp"

#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/strategies/relate/cartesian.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

namespace osrm::engine::isochrone
{
namespace
{

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using ExactInteger = boost::multiprecision::int256_t;
// The R-tree is a broad-phase filter only. Floating-point conversion is monotonic, so it can add
// candidates when adjacent int64 coordinates round together but cannot hide an actual bounding-box
// overlap. Keeping the index floating point also avoids signed overflow in Boost's packed R-tree
// centroid calculation near the int64 limits. All topology decisions below still use exact integer
// arithmetic.
using IndexPoint = bg::model::point<double, 2, bg::cs::cartesian>;
using IndexBox = bg::model::box<IndexPoint>;
using IndexValue = std::pair<IndexBox, std::size_t>;

struct GridPointHash
{
    std::size_t operator()(const GridPoint point) const
    {
        const auto x = static_cast<std::uint64_t>(point.x);
        const auto y = static_cast<std::uint64_t>(point.y);
        return static_cast<std::size_t>((x << 32) ^ (x >> 32) ^ y ^ (y >> 32));
    }
};

bool pointLess(const GridPoint left, const GridPoint right)
{ return left.x < right.x || (left.x == right.x && left.y < right.y); }

ExactInteger cross(const GridPoint a, const GridPoint b, const GridPoint c)
{
    const auto ab_x = ExactInteger{b.x} - a.x;
    const auto ab_y = ExactInteger{b.y} - a.y;
    const auto ac_x = ExactInteger{c.x} - a.x;
    const auto ac_y = ExactInteger{c.y} - a.y;
    return ab_x * ac_y - ab_y * ac_x;
}

int sign(const ExactInteger &value) { return value > 0 ? 1 : value < 0 ? -1 : 0; }

ExactInteger twiceSignedArea(const GridRing &ring)
{
    ExactInteger area = 0;
    for (std::size_t index = 0; index + 1 < ring.size(); ++index)
    {
        area += ExactInteger{ring[index].x} * ring[index + 1].y -
                ExactInteger{ring[index + 1].x} * ring[index].y;
    }
    return area;
}

long double pointSegmentDistance(const GridPoint point,
                                 const GridPoint segment_start,
                                 const GridPoint segment_end)
{
    const auto dx = static_cast<long double>(segment_end.x) - segment_start.x;
    const auto dy = static_cast<long double>(segment_end.y) - segment_start.y;
    const auto px = static_cast<long double>(point.x) - segment_start.x;
    const auto py = static_cast<long double>(point.y) - segment_start.y;
    const auto length_squared = dx * dx + dy * dy;
    if (length_squared == 0.)
        return std::hypot(px, py);

    const auto fraction = std::clamp((px * dx + py * dy) / length_squared, 0.L, 1.L);
    return std::hypot(px - fraction * dx, py - fraction * dy);
}

bool hasWorkCapacity(const std::size_t used,
                     const std::size_t additional,
                     const std::size_t maximum)
{ return used <= maximum && additional <= maximum - used; }

struct WorkBudget
{
    std::size_t remaining;

    bool consume(const std::size_t amount)
    {
        if (amount > remaining)
            return false;
        remaining -= amount;
        return true;
    }
};

std::optional<GridRing> simplifyRing(const GridRing &ring,
                                     const long double tolerance,
                                     const std::unordered_set<GridPoint, GridPointHash> &pinned,
                                     WorkBudget &distance_budget)
{
    if (ring.size() <= 4)
        return ring;

    const auto point_count = ring.size() - 1;
    std::vector<bool> keep(point_count + 1, false);
    keep.front() = true;
    keep.back() = true;

    if (!distance_budget.consume(point_count - 1))
        return std::nullopt;
    std::size_t farthest = 1;
    long double farthest_distance = -1.;
    for (std::size_t index = 1; index < point_count; ++index)
    {
        const auto dx = static_cast<long double>(ring[index].x) - ring.front().x;
        const auto dy = static_cast<long double>(ring[index].y) - ring.front().y;
        const auto distance = dx * dx + dy * dy;
        if (distance > farthest_distance)
        {
            farthest = index;
            farthest_distance = distance;
        }
    }
    keep[farthest] = true;

    // A closed ring cannot be simplified against its coincident first/last point. A second
    // deterministic pivot also prevents a very large tolerance from collapsing a polygon below
    // three distinct vertices.
    if (!distance_budget.consume(point_count - 2))
        return std::nullopt;
    std::size_t second_pivot = point_count;
    long double second_distance = -1.;
    for (std::size_t index = 1; index < point_count; ++index)
    {
        if (index == farthest)
            continue;
        const auto distance = pointSegmentDistance(ring[index], ring.front(), ring[farthest]);
        if (distance > second_distance)
        {
            second_pivot = index;
            second_distance = distance;
        }
    }
    if (second_pivot == point_count || second_distance <= 0.)
        return std::nullopt;
    keep[second_pivot] = true;

    for (std::size_t index = 0; index < point_count; ++index)
    {
        if (pinned.contains(ring[index]))
            keep[index] = true;
    }

    std::vector<std::size_t> anchors;
    anchors.reserve(point_count + 1);
    for (std::size_t index = 0; index <= point_count; ++index)
    {
        if (keep[index])
            anchors.push_back(index);
    }

    std::vector<std::pair<std::size_t, std::size_t>> ranges;
    ranges.reserve(anchors.size());
    for (std::size_t index = 0; index + 1 < anchors.size(); ++index)
        ranges.emplace_back(anchors[index], anchors[index + 1]);

    while (!ranges.empty())
    {
        const auto [first, last] = ranges.back();
        ranges.pop_back();
        if (last <= first + 1)
            continue;

        const auto checks = last - first - 1;
        if (!distance_budget.consume(checks))
            return std::nullopt;

        std::size_t selected = last;
        auto maximum_distance = tolerance;
        for (auto index = first + 1; index < last; ++index)
        {
            const auto distance = pointSegmentDistance(ring[index], ring[first], ring[last]);
            if (distance > maximum_distance)
            {
                selected = index;
                maximum_distance = distance;
            }
        }
        if (selected == last)
            continue;

        keep[selected] = true;
        ranges.emplace_back(first, selected);
        ranges.emplace_back(selected, last);
    }

    GridRing simplified;
    simplified.reserve(point_count + 1);
    for (std::size_t index = 0; index < point_count; ++index)
    {
        if (keep[index])
            simplified.push_back(ring[index]);
    }
    simplified.push_back(simplified.front());
    return simplified;
}

IndexBox makeBox(const GridPoint first, const GridPoint second)
{
    return {IndexPoint{static_cast<double>(std::min(first.x, second.x)),
                       static_cast<double>(std::min(first.y, second.y))},
            IndexPoint{static_cast<double>(std::max(first.x, second.x)),
                       static_cast<double>(std::max(first.y, second.y))}};
}

IndexBox makeBox(const GridRing &ring)
{
    auto minimum_x = ring.front().x;
    auto maximum_x = ring.front().x;
    auto minimum_y = ring.front().y;
    auto maximum_y = ring.front().y;
    for (const auto point : ring)
    {
        minimum_x = std::min(minimum_x, point.x);
        maximum_x = std::max(maximum_x, point.x);
        minimum_y = std::min(minimum_y, point.y);
        maximum_y = std::max(maximum_y, point.y);
    }
    return {IndexPoint{static_cast<double>(minimum_x), static_cast<double>(minimum_y)},
            IndexPoint{static_cast<double>(maximum_x), static_cast<double>(maximum_y)}};
}

IndexBox combinedBox(const GridRing &first, const GridRing &second)
{
    const auto first_box = makeBox(first);
    const auto second_box = makeBox(second);
    return {IndexPoint{std::min(bg::get<bg::min_corner, 0>(first_box),
                                bg::get<bg::min_corner, 0>(second_box)),
                       std::min(bg::get<bg::min_corner, 1>(first_box),
                                bg::get<bg::min_corner, 1>(second_box))},
            IndexPoint{std::max(bg::get<bg::max_corner, 0>(first_box),
                                bg::get<bg::max_corner, 0>(second_box)),
                       std::max(bg::get<bg::max_corner, 1>(first_box),
                                bg::get<bg::max_corner, 1>(second_box))}};
}

bool pointOnSegment(const GridPoint point, const GridPoint start, const GridPoint end)
{
    return cross(start, end, point) == 0 && point.x >= std::min(start.x, end.x) &&
           point.x <= std::max(start.x, end.x) && point.y >= std::min(start.y, end.y) &&
           point.y <= std::max(start.y, end.y);
}

enum class IntersectionKind
{
    None,
    SharedEndpoint,
    Other
};

struct Intersection
{
    IntersectionKind kind = IntersectionKind::None;
    GridPoint point{};
};

Intersection
segmentIntersection(const GridPoint a, const GridPoint b, const GridPoint c, const GridPoint d)
{
    const auto ab_c = sign(cross(a, b, c));
    const auto ab_d = sign(cross(a, b, d));
    const auto cd_a = sign(cross(c, d, a));
    const auto cd_b = sign(cross(c, d, b));
    const auto proper_intersection = ab_c * ab_d < 0 && cd_a * cd_b < 0;
    if (proper_intersection)
        return {IntersectionKind::Other, {}};

    std::array<GridPoint, 4> contacts{};
    std::size_t contact_count = 0;
    const auto add_contact = [&](const GridPoint point)
    {
        if (std::find(contacts.begin(), contacts.begin() + contact_count, point) ==
            contacts.begin() + contact_count)
        {
            contacts[contact_count++] = point;
        }
    };
    if (pointOnSegment(a, c, d))
        add_contact(a);
    if (pointOnSegment(b, c, d))
        add_contact(b);
    if (pointOnSegment(c, a, b))
        add_contact(c);
    if (pointOnSegment(d, a, b))
        add_contact(d);

    if (contact_count == 0)
        return {};
    if (contact_count != 1)
        return {IntersectionKind::Other, {}};

    const auto point = contacts.front();
    const auto endpoint_of_first = point == a || point == b;
    const auto endpoint_of_second = point == c || point == d;
    if (endpoint_of_first && endpoint_of_second)
        return {IntersectionKind::SharedEndpoint, point};
    return {IntersectionKind::Other, point};
}

enum class PointLocation
{
    Outside,
    Inside,
    Boundary
};

std::optional<PointLocation>
locatePoint(const GridPoint point, const GridRing &ring, WorkBudget &topology_budget)
{
    int winding = 0;
    for (std::size_t index = 0; index + 1 < ring.size(); ++index)
    {
        if (!topology_budget.consume(1))
            return std::nullopt;
        const auto from = ring[index];
        const auto to = ring[index + 1];
        if (pointOnSegment(point, from, to))
            return PointLocation::Boundary;

        if (from.y <= point.y)
        {
            if (to.y > point.y && cross(from, to, point) > 0)
                ++winding;
        }
        else if (to.y <= point.y && cross(from, to, point) < 0)
            --winding;
    }
    return winding == 0 ? PointLocation::Outside : PointLocation::Inside;
}

std::optional<PointLocation>
locateRing(const GridRing &subject, const GridRing &container, WorkBudget &topology_budget)
{
    for (std::size_t index = 0; index + 1 < subject.size(); ++index)
    {
        const auto location = locatePoint(subject[index], container, topology_budget);
        if (!location)
            return std::nullopt;
        if (*location != PointLocation::Boundary)
            return location;
    }
    return PointLocation::Boundary;
}

struct RingPair
{
    const GridRing *original;
    const GridRing *candidate;
};

std::vector<RingPair> pairRings(const std::vector<GridPolygon> &original,
                                const std::vector<GridPolygon> &candidate)
{
    std::vector<RingPair> rings;
    for (std::size_t polygon = 0; polygon < original.size(); ++polygon)
    {
        rings.push_back({&original[polygon].outer, &candidate[polygon].outer});
        for (std::size_t hole = 0; hole < original[polygon].holes.size(); ++hole)
            rings.push_back({&original[polygon].holes[hole], &candidate[polygon].holes[hole]});
    }
    return rings;
}

struct Segment
{
    GridPoint from;
    GridPoint to;
    std::size_t ring;
    std::size_t edge;
    std::size_t edge_count;
    IndexBox box;
};

bool hasValidBoundaryInteractions(
    const std::vector<RingPair> &rings,
    const std::unordered_set<GridPoint, GridPointHash> &permitted_contacts,
    WorkBudget &topology_budget)
{
    std::vector<Segment> segments;
    for (std::size_t ring_index = 0; ring_index < rings.size(); ++ring_index)
    {
        const auto &ring = *rings[ring_index].candidate;
        const auto edge_count = ring.size() - 1;
        for (std::size_t edge = 0; edge < edge_count; ++edge)
        {
            if (ring[edge] == ring[edge + 1])
                return false;
            segments.push_back({ring[edge],
                                ring[edge + 1],
                                ring_index,
                                edge,
                                edge_count,
                                makeBox(ring[edge], ring[edge + 1])});
        }
    }

    std::vector<IndexValue> indexed_segments;
    indexed_segments.reserve(segments.size());
    for (std::size_t index = 0; index < segments.size(); ++index)
        indexed_segments.emplace_back(segments[index].box, index);
    const bgi::rtree<IndexValue, bgi::quadratic<16>> index(indexed_segments);

    std::vector<IndexValue> candidates;
    for (std::size_t left_index = 0; left_index < segments.size(); ++left_index)
    {
        candidates.clear();
        index.query(bgi::intersects(segments[left_index].box), std::back_inserter(candidates));
        if (!topology_budget.consume(candidates.size()))
            return false;
        const auto &left = segments[left_index];
        for (const auto &[box, right_index] : candidates)
        {
            static_cast<void>(box);
            if (right_index <= left_index)
                continue;
            const auto &right = segments[right_index];
            const auto intersection = segmentIntersection(left.from, left.to, right.from, right.to);
            if (intersection.kind == IntersectionKind::None)
                continue;

            if (left.ring == right.ring)
            {
                const auto adjacent = (left.edge + 1) % left.edge_count == right.edge ||
                                      (right.edge + 1) % right.edge_count == left.edge;
                if (adjacent && intersection.kind == IntersectionKind::SharedEndpoint)
                    continue;
                return false;
            }

            if (intersection.kind != IntersectionKind::SharedEndpoint ||
                !permitted_contacts.contains(intersection.point))
            {
                return false;
            }
        }
    }
    return true;
}

bool preservesRingRelationships(const std::vector<RingPair> &rings, WorkBudget &topology_budget)
{
    std::vector<IndexValue> boxes;
    boxes.reserve(rings.size());
    for (std::size_t index = 0; index < rings.size(); ++index)
        boxes.emplace_back(combinedBox(*rings[index].original, *rings[index].candidate), index);
    const bgi::rtree<IndexValue, bgi::quadratic<16>> box_index(boxes);

    std::vector<IndexValue> candidates;
    for (std::size_t left = 0; left < rings.size(); ++left)
    {
        candidates.clear();
        box_index.query(bgi::intersects(boxes[left].first), std::back_inserter(candidates));
        if (!topology_budget.consume(candidates.size()))
            return false;
        for (const auto &[box, right] : candidates)
        {
            static_cast<void>(box);
            if (right <= left)
                continue;

            const auto original_left_in_right =
                locateRing(*rings[left].original, *rings[right].original, topology_budget);
            const auto candidate_left_in_right =
                locateRing(*rings[left].candidate, *rings[right].candidate, topology_budget);
            if (!original_left_in_right || !candidate_left_in_right ||
                *original_left_in_right != *candidate_left_in_right)
                return false;

            const auto original_right_in_left =
                locateRing(*rings[right].original, *rings[left].original, topology_budget);
            const auto candidate_right_in_left =
                locateRing(*rings[right].candidate, *rings[left].candidate, topology_budget);
            if (!original_right_in_left || !candidate_right_in_left ||
                *original_right_in_left != *candidate_right_in_left)
                return false;
        }
    }
    return true;
}

std::unordered_set<GridPoint, GridPointHash>
findSharedVertices(const std::vector<GridPolygon> &polygons)
{
    std::vector<GridPoint> points;
    for (const auto &polygon : polygons)
    {
        points.insert(points.end(), polygon.outer.begin(), polygon.outer.end() - 1);
        for (const auto &hole : polygon.holes)
            points.insert(points.end(), hole.begin(), hole.end() - 1);
    }
    std::sort(points.begin(), points.end(), pointLess);

    std::unordered_set<GridPoint, GridPointHash> shared;
    for (std::size_t index = 1; index < points.size(); ++index)
    {
        if (points[index] == points[index - 1])
            shared.insert(points[index]);
    }
    return shared;
}

bool isValidCandidate(const std::vector<GridPolygon> &original,
                      const std::vector<GridPolygon> &candidate,
                      const std::unordered_set<GridPoint, GridPointHash> &permitted_contacts,
                      WorkBudget &topology_budget)
{
    if (candidate.size() != original.size())
        return false;
    for (std::size_t polygon = 0; polygon < original.size(); ++polygon)
    {
        if (candidate[polygon].holes.size() != original[polygon].holes.size())
            return false;
    }

    const auto rings = pairRings(original, candidate);
    for (const auto &[original_ring, candidate_ring] : rings)
    {
        if (candidate_ring->size() < 4 || candidate_ring->front() != candidate_ring->back())
            return false;
        const auto original_area = twiceSignedArea(*original_ring);
        const auto candidate_area = twiceSignedArea(*candidate_ring);
        if (candidate_area == 0 || sign(original_area) != sign(candidate_area))
            return false;
    }

    return hasValidBoundaryInteractions(rings, permitted_contacts, topology_budget) &&
           preservesRingRelationships(rings, topology_budget);
}

bool isClosedRing(const GridRing &ring) { return ring.size() >= 4 && ring.front() == ring.back(); }

} // namespace

bool generalizeGridPolygons(std::vector<GridPolygon> &polygons, const double tolerance)
{
    if (!(tolerance > 0.) || !std::isfinite(tolerance) || polygons.empty())
        return false;

    // Generalization is optional. Keep its auxiliary allocations bounded independently of the
    // larger response-coordinate budget so a highly fragmented contour falls back to raw output.
    constexpr std::size_t MAXIMUM_GENERALIZATION_COORDINATES = 100'000;
    std::size_t coordinate_count = 0;
    for (const auto &polygon : polygons)
    {
        if (!isClosedRing(polygon.outer))
            return false;
        if (!hasWorkCapacity(
                coordinate_count, polygon.outer.size(), std::numeric_limits<std::size_t>::max()))
            return false;
        coordinate_count += polygon.outer.size();
        if (coordinate_count > MAXIMUM_GENERALIZATION_COORDINATES)
            return false;
        for (const auto &hole : polygon.holes)
        {
            if (!isClosedRing(hole))
                return false;
            if (!hasWorkCapacity(
                    coordinate_count, hole.size(), std::numeric_limits<std::size_t>::max()))
                return false;
            coordinate_count += hole.size();
            if (coordinate_count > MAXIMUM_GENERALIZATION_COORDINATES)
                return false;
        }
    }

    // A requested tolerance can make independently simplified rings intersect even though a
    // smaller tolerance is safe. Keep a single bounded work budget across all retries so
    // topology-rich inputs can back off without making adversarial inputs unbounded.
    constexpr std::size_t MAXIMUM_ATTEMPTS = 16;
    constexpr std::size_t DISTANCE_CHECK_FACTOR = 80;
    constexpr std::size_t MAXIMUM_DISTANCE_CHECKS = 2'000'000;
    WorkBudget distance_budget{
        std::min(coordinate_count * DISTANCE_CHECK_FACTOR, MAXIMUM_DISTANCE_CHECKS)};
    constexpr std::size_t TOPOLOGY_PROBE_FACTOR = 512;
    constexpr std::size_t MAXIMUM_TOPOLOGY_PROBES = 1'000'000;
    WorkBudget topology_budget{
        std::min(coordinate_count * TOPOLOGY_PROBE_FACTOR, MAXIMUM_TOPOLOGY_PROBES)};
    const auto shared_vertices = findSharedVertices(polygons);

    auto candidate_tolerance = static_cast<long double>(tolerance);
    for (std::size_t attempt = 0; attempt < MAXIMUM_ATTEMPTS; ++attempt)
    {
        auto candidate = polygons;
        auto changed = false;
        for (auto &polygon : candidate)
        {
            auto outer =
                simplifyRing(polygon.outer, candidate_tolerance, shared_vertices, distance_budget);
            if (!outer)
                return false;
            changed = changed || outer->size() != polygon.outer.size();
            polygon.outer = std::move(*outer);
            for (auto &hole : polygon.holes)
            {
                auto simplified =
                    simplifyRing(hole, candidate_tolerance, shared_vertices, distance_budget);
                if (!simplified)
                    return false;
                changed = changed || simplified->size() != hole.size();
                hole = std::move(*simplified);
            }
        }

        if (!changed)
            return false;

        if (isValidCandidate(polygons, candidate, shared_vertices, topology_budget))
        {
            polygons = std::move(candidate);
            return true;
        }

        candidate_tolerance /= 2.;
        if (!(candidate_tolerance > 0.))
            break;
    }
    return false;
}

} // namespace osrm::engine::isochrone
