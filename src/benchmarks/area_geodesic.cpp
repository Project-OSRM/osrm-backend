/*
 * What would it cost to solve a plaza at query time?
 *
 * When both ends of a request lie inside one open area, the answer is the geodesic
 * between them: the straight line if they can see each other, otherwise a path bending
 * round the obstacles.  Rather than bake a graph that can answer that, the engine could
 * work it out when asked -- it already carries the polygon, for snapping.
 *
 * This times the three pieces of doing so, over areas of the sizes that actually occur.
 * Monaco's meshed areas have a median of seven visibility-graph vertices and a maximum of
 * twenty-four; AreaMesher::max_vertices caps an area at a hundred.
 */

#include "engine/area_visibility.hpp"

#include "util/timing_util.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <limits>
#include <span>
#include <vector>

namespace
{
using osrm::engine::area::Point;
using osrm::engine::area::Ring;

/** A square plaza with a grid of square blocks in it, giving 4 + 4b vertices. */
struct Plaza
{
    std::vector<Point> outer;
    std::vector<std::vector<Point>> blocks;
    std::vector<Ring> rings;

    explicit Plaza(std::size_t per_side)
    {
        const double side = 1000.0;
        outer = {{0, 0}, {side, 0}, {side, side}, {0, side}};
        const double step = per_side > 1 ? 760.0 / static_cast<double>(per_side - 1) : 0.0;
        for (std::size_t i = 0; i < per_side; ++i)
        {
            for (std::size_t j = 0; j < per_side; ++j)
            {
                const double x = 120.0 + static_cast<double>(i) * step;
                const double y = 120.0 + static_cast<double>(j) * step;
                blocks.push_back({{x, y}, {x + 45, y}, {x + 45, y + 45}, {x, y + 45}});
            }
        }
        rings.emplace_back(outer);
        for (const auto &block : blocks)
        {
            rings.emplace_back(block);
        }
    }

    std::size_t vertices() const { return 4 + 4 * blocks.size(); }

    Point vertex(std::size_t index) const
    {
        if (index < outer.size())
        {
            return outer[index];
        }
        index -= outer.size();
        return blocks[index / 4][index % 4];
    }
};

/**
 * Every mutually visible pair among the polygon's own vertices.
 *
 * This is the expensive half: it is what the mesher builds at extraction time, and what
 * a query would have to build for itself if the two ends cannot see each other.
 */
std::vector<std::pair<std::size_t, std::size_t>> visibility_graph(const Plaza &plaza)
{
    std::vector<std::pair<std::size_t, std::size_t>> edges;
    const auto count = plaza.vertices();
    for (std::size_t u = 0; u < count; ++u)
    {
        // visible_vertices() answers for one point against every vertex, so n calls
        // build the whole graph
        for (const auto v : visible_vertices(plaza.vertex(u), plaza.rings))
        {
            if (v > u)
            {
                edges.emplace_back(u, v);
            }
        }
    }
    return edges;
}

bool crosses_any(const Plaza &plaza, const Point &from, const Point &to);

/** Dijkstra over that graph, with the two query points joined to what they can see. */
double geodesic(const Plaza &plaza,
                const std::vector<std::pair<std::size_t, std::size_t>> &edges,
                const Point &from,
                const Point &to)
{
    const auto count = plaza.vertices();
    const auto source = count, target = count + 1;
    std::vector<std::vector<std::pair<std::size_t, double>>> adjacency(count + 2);

    const auto join = [&](std::size_t node, std::size_t other, Point a, Point b)
    {
        const auto weight = std::hypot(a.x - b.x, a.y - b.y);
        adjacency[node].emplace_back(other, weight);
        adjacency[other].emplace_back(node, weight);
    };

    for (const auto &[u, v] : edges)
    {
        join(u, v, plaza.vertex(u), plaza.vertex(v));
    }
    for (const auto v : visible_vertices(from, plaza.rings))
    {
        join(source, v, from, plaza.vertex(v));
    }
    for (const auto v : visible_vertices(to, plaza.rings))
    {
        join(target, v, to, plaza.vertex(v));
    }
    if (!crosses_any(plaza, from, to))
    {
        join(source, target, from, to);
    }

    std::vector<double> best(adjacency.size(), std::numeric_limits<double>::infinity());
    std::vector<bool> done(adjacency.size(), false);
    best[source] = 0.0;
    for (;;)
    {
        std::size_t here = adjacency.size();
        double smallest = std::numeric_limits<double>::infinity();
        for (std::size_t i = 0; i < adjacency.size(); ++i)
        {
            if (!done[i] && best[i] < smallest)
            {
                smallest = best[i];
                here = i;
            }
        }
        if (here == adjacency.size())
        {
            break;
        }
        done[here] = true;
        for (const auto &[other, weight] : adjacency[here])
        {
            best[other] = std::min(best[other], best[here] + weight);
        }
    }
    return best[target];
}

bool crosses_any(const Plaza &plaza, const Point &from, const Point &to)
{
    return std::any_of(plaza.rings.begin(),
                       plaza.rings.end(),
                       [&](const Ring &ring) { return crosses_ring(from, to, ring); });
}
} // namespace

int main()
{
    std::cout << "  vertices   see-from-one-point   whole graph   graph + dijkstra\n";
    for (const std::size_t per_side : {1u, 2u, 3u, 4u, 5u})
    {
        const Plaza plaza{per_side};
        const Point from{60, 60}, to{940, 940};
        const std::size_t rounds = 2000;

        TIMER_START(one_point);
        for (std::size_t i = 0; i < rounds; ++i)
        {
            auto seen = visible_vertices(from, plaza.rings);
            (void)seen.size();
        }
        TIMER_STOP(one_point);

        TIMER_START(graph);
        for (std::size_t i = 0; i < rounds; ++i)
        {
            auto edges = visibility_graph(plaza);
            (void)edges.size();
        }
        TIMER_STOP(graph);

        TIMER_START(full);
        double checksum = 0.0;
        for (std::size_t i = 0; i < rounds; ++i)
        {
            auto edges = visibility_graph(plaza);
            checksum += geodesic(plaza, edges, from, to);
        }
        TIMER_STOP(full);

        const auto us = [](double seconds) { return seconds * 1e6 / static_cast<double>(rounds); };
        std::cout << std::fixed << std::setprecision(1) << std::setw(10) << plaza.vertices()
                  << std::setw(20) << us(TIMER_SEC(one_point)) << " us" << std::setw(11)
                  << us(TIMER_SEC(graph)) << " us" << std::setw(15) << us(TIMER_SEC(full))
                  << " us   (" << checksum / static_cast<double>(rounds) << " m)\n";
    }
    return 0;
}
