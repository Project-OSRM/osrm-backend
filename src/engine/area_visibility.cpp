#include "engine/area_visibility.hpp"

#include "extractor/area/util.hpp"

namespace osrm::engine::area
{

namespace
{
/** Call `function` for every edge of the ring, closing it. */
template <typename Fun> void for_each_edge(Ring ring, Fun function)
{
    for (std::size_t i = 0; i < ring.size(); ++i)
        function(ring[i], ring[(i + 1) % ring.size()]);
}
} // namespace

bool crosses_ring(const Point &from, const Point &to, Ring ring)
{
    bool crosses = false;
    for_each_edge(ring,
                  [&](const Point &a, const Point &b)
                  {
                      // an edge sharing an endpoint with from..to meets it only there,
                      // and cannot obstruct it -- the same reasoning as in the sweep
                      const auto same = [](const Point &p, const Point &q)
                      { return p.x == q.x && p.y == q.y; };
                      if (same(a, from) || same(a, to) || same(b, from) || same(b, to))
                          return;
                      if (extractor::area::intersect(&from, &to, &a, &b))
                          crosses = true;
                  });
    return crosses;
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
            const Point midpoint{(point.x + vertex.x) / 2, (point.y + vertex.y) / 2};
            bool blocked = !inside_area(midpoint, rings);
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
