#include "engine/area_geodesic.hpp"

#include "engine/area_visibility.hpp"

#include "util/coordinate_calculation.hpp"

#include <algorithm>
#include <limits>
#include <list>
#include <queue>
#include <unordered_map>
#include <utility>

namespace osrm::engine::area
{

namespace
{

/**
 * @brief One area, ready to be asked questions about.
 *
 * Holds the vertices twice over: projected, because the visibility predicates work in
 * that space, and as they came, because distances have to be measured on the sphere.
 * Neither can stand in for the other -- the projection preserves what crosses what but
 * not how long anything is.
 */
struct SolvedArea
{
    std::vector<util::Coordinate> coordinates; // every ring flattened, outer first
    std::vector<std::vector<Point>> projected; // the same, projected, per ring
    std::vector<Ring> rings;                   // views onto `projected`
    //! Mutually visible pairs among the vertices, weighted in metres.
    std::vector<std::vector<std::pair<std::size_t, double>>> adjacency;

    std::size_t size() const { return coordinates.size(); }

    /** Is this the area those rings describe, and not merely one filed under its name? */
    bool describes(const std::vector<std::span<const util::Coordinate>> &rings) const
    {
        if (rings.size() != projected.size())
        {
            return false;
        }
        std::size_t at = 0;
        for (std::size_t ring = 0; ring < rings.size(); ++ring)
        {
            if (rings[ring].size() != projected[ring].size())
            {
                return false;
            }
            for (const auto coordinate : rings[ring])
            {
                if (coordinate != coordinates[at++])
                {
                    return false;
                }
            }
        }
        return true;
    }
};

double metres(const util::Coordinate a, const util::Coordinate b)
{ return util::coordinate_calculation::greatCircleDistance(a, b); }

/** Project the rings and index every vertex once, in ring order. */
void flatten(const std::vector<std::span<const util::Coordinate>> &rings, SolvedArea &area)
{
    area.projected.reserve(rings.size());
    for (const auto &ring : rings)
    {
        std::vector<Point> points;
        points.reserve(ring.size());
        for (const auto coordinate : ring)
        {
            area.coordinates.push_back(coordinate);
            points.push_back(project(coordinate));
        }
        area.projected.push_back(std::move(points));
    }
    // only once `projected` has stopped growing, or the views would dangle
    area.rings.reserve(area.projected.size());
    for (const auto &points : area.projected)
    {
        area.rings.emplace_back(points);
    }
}

/** The point of the area a flat index refers to. */
Point projected_vertex(const SolvedArea &area, std::size_t index)
{
    for (const auto &ring : area.rings)
    {
        if (index < ring.size())
        {
            return ring[index];
        }
        index -= ring.size();
    }
    return Point{};
}

/** The mutually visible pairs, which is the expensive half and the reason to cache. */
void build_adjacency(SolvedArea &area)
{
    area.adjacency.resize(area.size());
    for (std::size_t u = 0; u < area.size(); ++u)
    {
        // visible_vertices() reports every vertex this one can see, so running it from
        // each vertex in turn builds the whole graph.  It is symmetric, so keep the
        // upper half and mirror it.
        for (const auto v : visible_vertices(projected_vertex(area, u), area.rings))
        {
            if (v > u)
            {
                const auto weight = metres(area.coordinates[u], area.coordinates[v]);
                area.adjacency[u].emplace_back(v, weight);
                area.adjacency[v].emplace_back(u, weight);
            }
        }
    }
}

/**
 * @brief A few areas' graphs, most recently used first.
 *
 * Per thread, in the manner of SearchEngineData: a plaza asked about once is likely to be
 * asked about again, and a thread that never touches an area pays nothing.
 */
struct Cache
{
    using Key = std::pair<std::uint32_t, std::uint64_t>;

    struct Entry
    {
        Key key;
        SolvedArea area;
    };

    std::list<Entry> entries;
    std::unordered_map<std::uint64_t, std::list<Entry>::iterator> index;

    static std::uint64_t hash(const Key &key)
    { return (static_cast<std::uint64_t>(key.first) << 32) ^ (key.second * 0x9e3779b97f4a7c15ULL); }

    const SolvedArea *find(const Key &key)
    {
        const auto found = index.find(hash(key));
        if (found == index.end() || found->second->key != key)
        {
            return nullptr;
        }
        entries.splice(entries.begin(), entries, found->second);
        return &entries.front().area;
    }

    const SolvedArea &put(const Key &key, SolvedArea area)
    {
        entries.push_front(Entry{key, std::move(area)});
        index[hash(key)] = entries.begin();
        while (entries.size() > GEODESIC_CACHE_SIZE)
        {
            index.erase(hash(entries.back().key));
            entries.pop_back();
        }
        return entries.front().area;
    }
};

Cache &cache()
{
    static thread_local Cache instance;
    return instance;
}

/** Dijkstra from the source over the area's graph, with both query points attached. */
std::optional<Geodesic> shortest(const SolvedArea &area,
                                 const util::Coordinate from,
                                 const util::Coordinate to,
                                 const Point projected_from,
                                 const Point projected_to)
{
    const auto count = area.size();
    const auto source = count, target = count + 1;

    std::vector<std::vector<std::pair<std::size_t, double>>> extra(2);
    std::vector<std::vector<std::pair<std::size_t, double>>> incident(count);
    const auto attach = [&](std::size_t which, const util::Coordinate at, const Point projected)
    {
        for (const auto v : visible_vertices(projected, area.rings))
        {
            const auto weight = metres(at, area.coordinates[v]);
            extra[which].emplace_back(v, weight);
            incident[v].emplace_back(count + which, weight);
        }
    };
    attach(0, from, projected_from);
    attach(1, to, projected_to);

    // The straight line, when nothing stands in the way.  It can never be beaten, but
    // going through the search anyway keeps one code path instead of two.
    const auto blocked = std::any_of(area.rings.begin(),
                                     area.rings.end(),
                                     [&](const Ring &ring)
                                     { return crosses_ring(projected_from, projected_to, ring); });
    if (!blocked)
    {
        const auto weight = metres(from, to);
        extra[0].emplace_back(target, weight);
        extra[1].emplace_back(source, weight);
    }

    const auto neighbours = [&](std::size_t node) -> const auto &
    {
        if (node >= count)
        {
            return extra[node - count];
        }
        return area.adjacency[node];
    };

    constexpr auto INF = std::numeric_limits<double>::infinity();
    std::vector<double> best(count + 2, INF);
    std::vector<std::size_t> came_from(count + 2, count + 2);
    std::priority_queue<std::pair<double, std::size_t>,
                        std::vector<std::pair<double, std::size_t>>,
                        std::greater<>>
        queue;

    best[source] = 0.0;
    queue.emplace(0.0, source);
    while (!queue.empty())
    {
        const auto [distance, here] = queue.top();
        queue.pop();
        if (distance > best[here])
        {
            continue;
        }
        if (here == target)
        {
            break;
        }
        const auto relax = [&](std::size_t other, double weight)
        {
            if (distance + weight < best[other])
            {
                best[other] = distance + weight;
                came_from[other] = here;
                queue.emplace(best[other], other);
            }
        };
        for (const auto &[other, weight] : neighbours(here))
        {
            relax(other, weight);
        }
        if (here < count)
        {
            for (const auto &[other, weight] : incident[here])
            {
                relax(other, weight);
            }
        }
    }

    if (best[target] == INF)
    {
        // the two points are in the same area but no path runs between them, which an
        // obstacle touching the boundary can do
        return std::nullopt;
    }

    Geodesic result;
    result.length = best[target];
    for (auto node = came_from[target]; node < count; node = came_from[node])
    {
        result.bends.push_back(area.coordinates[node]);
    }
    std::reverse(result.bends.begin(), result.bends.end());
    return result;
}

} // namespace

/**
 * The graph the mesher wrote down, if it wrote one.
 *
 * Every edge appears under both of its vertices, so only the direction that goes up is
 * taken, and each edge is weighted once.  A polygon the mesher declined has no entries at
 * all, which is how the caller knows to build the graph itself.
 */
bool adopt_stored_visibility(SolvedArea &area, const StoredVisibility &stored)
{
    area.adjacency.assign(area.size(), {});
    bool any = false;
    for (std::uint32_t u = 0; u < area.size(); ++u)
    {
        for (const auto v : stored(u))
        {
            if (v >= area.size() || v <= u)
                continue;
            any = true;
            const auto weight = metres(area.coordinates[u], area.coordinates[v]);
            area.adjacency[u].emplace_back(v, weight);
            area.adjacency[v].emplace_back(u, weight);
        }
    }
    return any;
}

std::optional<Geodesic>
geodesic_between(std::uint32_t dataset,
                 std::uint64_t area_key,
                 const std::vector<std::span<const util::Coordinate>> &rings,
                 const util::Coordinate from,
                 const util::Coordinate to,
                 std::optional<StoredVisibility> stored_visibility)
{
    // an area needs an outer ring with area to it, and the request has to be for two
    // points that are really inside
    if (rings.empty() || rings.front().size() < 3)
    {
        return std::nullopt;
    }
    std::size_t vertices = 0;
    for (const auto &ring : rings)
    {
        vertices += ring.size();
    }
    if (vertices == 0)
    {
        return std::nullopt;
    }

    const auto projected_from = project(from), projected_to = project(to);

    const Cache::Key key{dataset, area_key};
    const SolvedArea *area = cache().find(key);
    if (area != nullptr && !area->describes(rings))
    {
        // The key is not proof of identity.  A checksum says what a dataset contains, not
        // which dataset it is, and one process can serve several in turn -- osrm-datastore
        // swaps them under a running server -- so two areas from two datasets can arrive
        // wearing the same key.  Comparing the vertices settles it, and costs a walk over
        // at most GEODESIC_MAX_VERTICES coordinates against building the graph again.
        area = nullptr;
    }
    if (area == nullptr)
    {
        // Containment is decided before the graph is built, not after.  Projecting the
        // rings is linear and building the visibility graph is cubic, and a coordinate
        // arrives here once for every area whose *bounding box* claimed it, most of
        // which do not hold it at all.  Building first would charge every one of those
        // near misses the full price of an answer that is about to be thrown away.
        SolvedArea candidate;
        flatten(rings, candidate);
        if (!inside_area(projected_from, candidate.rings) ||
            !inside_area(projected_to, candidate.rings))
        {
            return std::nullopt;
        }
        // The graph, from extraction where it wrote one; built here otherwise, and then
        // only for an area small enough that a cubic build fits a request.
        if (!(stored_visibility && adopt_stored_visibility(candidate, *stored_visibility)))
        {
            if (vertices > GEODESIC_MAX_VERTICES)
            {
                return std::nullopt;
            }
            build_adjacency(candidate);
        }
        area = &cache().put(key, std::move(candidate));
    }
    else if (!inside_area(projected_from, area->rings) || !inside_area(projected_to, area->rings))
    {
        return std::nullopt;
    }
    if (from == to)
    {
        return Geodesic{};
    }

    return shortest(*area, from, to, projected_from, projected_to);
}

void forget_cached_geodesics()
{
    cache().entries.clear();
    cache().index.clear();
}

std::size_t cached_geodesic_count() { return cache().entries.size(); }

} // namespace osrm::engine::area
