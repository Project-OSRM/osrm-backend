#include "engine/area_visibility.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cmath>
#include <span>
#include <vector>

BOOST_AUTO_TEST_SUITE(area_visibility_test)

using namespace osrm;
using namespace osrm::engine::area;

namespace
{

/** A 10x10 square with an optional 4x4 block in the middle. */
struct Fixture
{
    std::vector<Point> outer{{0, 0}, {10, 0}, {10, 10}, {0, 10}};
    std::vector<Point> obstacle{{3, 3}, {7, 3}, {7, 7}, {3, 7}};
    std::vector<Ring> rings;

    explicit Fixture(bool with_obstacle)
    {
        rings.emplace_back(outer);
        if (with_obstacle)
            rings.emplace_back(obstacle);
    }
};

bool sees(const Fixture &f, Point from, std::size_t vertex)
{
    const auto visible = visible_vertices(from, f.rings);
    return std::find(visible.begin(), visible.end(), vertex) != visible.end();
}

} // namespace

BOOST_AUTO_TEST_CASE(area_visibility_inside_and_outside)
{
    Fixture f{true};

    BOOST_CHECK(inside_area(Point{1, 1}, f.rings));
    BOOST_CHECK(inside_area(Point{9, 9}, f.rings));
    // outside the outer ring
    BOOST_CHECK(!inside_area(Point{-1, 5}, f.rings));
    BOOST_CHECK(!inside_area(Point{11, 5}, f.rings));
    // inside the obstacle is not inside the area
    BOOST_CHECK(!inside_area(Point{5, 5}, f.rings));
}

// With nothing in the way every corner is visible.
BOOST_AUTO_TEST_CASE(area_visibility_open_square_sees_every_corner)
{
    Fixture f{false};
    const auto visible = visible_vertices(Point{5, 5}, f.rings);

    const std::vector<std::size_t> expected{0, 1, 2, 3};
    BOOST_CHECK(visible == expected);
}

// The obstacle hides the corners on its far side, and only those.
BOOST_AUTO_TEST_CASE(area_visibility_obstacle_hides_what_is_behind_it)
{
    Fixture f{true};
    const Point from{1, 1}; // south-west, with the block to its north-east

    // the two outer corners the block does not stand in front of
    BOOST_CHECK(sees(f, from, 0)); // (0,0)
    BOOST_CHECK(sees(f, from, 1)); // (10,0)
    BOOST_CHECK(sees(f, from, 3)); // (0,10)
    // the far outer corner is straight through the block
    BOOST_CHECK(!sees(f, from, 2)); // (10,10)

    // of the block's own corners, the near one and its two neighbours are visible,
    // the far one is not
    BOOST_CHECK(sees(f, from, 4));  // (3,3), the near corner
    BOOST_CHECK(sees(f, from, 5));  // (7,3)
    BOOST_CHECK(sees(f, from, 7));  // (3,7)
    BOOST_CHECK(!sees(f, from, 6)); // (7,7), diagonally opposite
}

// The corners of the obstacle are exactly what makes going round the back possible, so
// they must be reported even though they belong to a hole rather than the boundary.
BOOST_AUTO_TEST_CASE(area_visibility_reports_obstacle_corners)
{
    Fixture f{true};
    const auto visible = visible_vertices(Point{1, 5}, f.rings);

    // indices 4..7 are the obstacle's corners
    const bool any_obstacle_corner =
        std::any_of(visible.begin(), visible.end(), [](std::size_t i) { return i >= 4; });
    BOOST_CHECK(any_obstacle_corner);
}

// A vertex of the ring the segment ends on must not hide that vertex from itself.
BOOST_AUTO_TEST_CASE(area_visibility_target_vertex_is_not_hidden_by_its_own_edges)
{
    Fixture f{true};
    // sitting right next to a corner of the obstacle
    BOOST_CHECK(sees(f, Point{2.5, 2.5}, 4)); // (3,3)
}

// Standing on a wall, the next vertex along that wall is visible.
//
// Every test above asks from a point in the interior. This one asks from a vertex, which
// is where the callers ask from: a portal sits on the boundary, and so does every vertex
// the planner expands. It is also the case a midpoint test gets wrong, because the
// midpoint of two adjacent vertices lies exactly on the ring between them and a strict
// containment test answers arbitrarily there.
BOOST_AUTO_TEST_CASE(area_visibility_a_wall_is_visible_along_its_length)
{
    Fixture f{true};

    // The outer ring, in both directions, from a corner.
    BOOST_CHECK(sees(f, f.outer[0], 1)); // (0,0) -> (10,0), along the bottom wall
    BOOST_CHECK(sees(f, f.outer[0], 3)); // (0,0) -> (0,10), along the left wall
    BOOST_CHECK(sees(f, f.outer[2], 1)); // (10,10) -> (10,0), along the right wall
    BOOST_CHECK(sees(f, f.outer[2], 3)); // (10,10) -> (0,10), along the top wall

    // And an obstacle's own wall, which is how a path gets round it. Indices 4..7.
    BOOST_CHECK(sees(f, f.obstacle[0], 5)); // (3,3) -> (7,3)
    BOOST_CHECK(sees(f, f.obstacle[0], 7)); // (3,3) -> (3,7)
}

// A diagonal of the square is not blocked by the walls it ends on.
BOOST_AUTO_TEST_CASE(area_visibility_a_diagonal_from_a_corner_is_not_blocked)
{
    Fixture f{false};

    BOOST_CHECK(sees(f, f.outer[0], 2)); // (0,0) -> (10,10)
    BOOST_CHECK(sees(f, f.outer[1], 3)); // (10,0) -> (0,10)
}

// Every line that stays inside is reported, over every pair of vertices of several shapes.
//
// The converse of the tests above it, and the one that was missing. Checking only that no
// reported line crosses an obstacle bounds the graph from one side: an oracle that reports
// nothing at all passes every such test. The defect that costs a path its shape is the
// opposite one, a pair that can see each other and is not reported, which deletes a
// shortcut from the graph and sends the planner the long way round.
//
// The oracle here shares no code with visible_vertices(): a ring edge blocks only if it
// properly crosses, meaning each segment strictly separates the other's endpoints, so
// shared endpoints and grazes come out as not blocking. Touching a corner in passing is
// allowed, cutting it is not.
BOOST_AUTO_TEST_CASE(area_visibility_reports_every_line_that_stays_inside)
{
    const auto side = [](const Point &a, const Point &b, const Point &c)
    {
        const auto cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        return cross > 0.0 ? 1 : (cross < 0.0 ? -1 : 0);
    };

    // A square, a square with a block, an L with a reflex corner, and two blocks.
    std::vector<std::vector<std::vector<Point>>> shapes{
        {{{0, 0}, {10, 0}, {10, 10}, {0, 10}}},
        {{{0, 0}, {10, 0}, {10, 10}, {0, 10}}, {{3, 3}, {7, 3}, {7, 7}, {3, 7}}},
        {{{0, 0}, {10, 0}, {10, 4}, {4, 4}, {4, 10}, {0, 10}}},
        {{{0, 0}, {12, 0}, {12, 12}, {0, 12}},
         {{2, 2}, {5, 2}, {5, 5}, {2, 5}},
         {{7, 7}, {10, 7}, {10, 10}, {7, 10}}}};

    std::size_t checked = 0;
    for (const auto &shape : shapes)
    {
        std::vector<Ring> rings;
        std::vector<Point> vertices;
        for (const auto &ring : shape)
        {
            rings.emplace_back(ring);
            vertices.insert(vertices.end(), ring.begin(), ring.end());
        }

        for (std::size_t v = 0; v < vertices.size(); ++v)
        {
            const auto reported = visible_vertices(vertices[v], rings);
            for (std::size_t w = 0; w < vertices.size(); ++w)
            {
                if (w == v)
                    continue;
                const auto &from = vertices[v];
                const auto &to = vertices[w];

                auto blocked = false;
                for (const auto &ring : rings)
                {
                    for (std::size_t i = 0; i < ring.size() && !blocked; ++i)
                    {
                        const auto &a = ring[i];
                        const auto &b = ring[(i + 1) % ring.size()];
                        blocked = side(from, to, a) * side(from, to, b) < 0 &&
                                  side(a, b, from) * side(a, b, to) < 0;
                    }
                }
                if (blocked)
                    continue;

                // Not blocked by a crossing, but it may still run outside the area, for
                // instance across the notch of the L. Sample the interior to rule it out.
                auto leaves = false;
                for (int k = 1; k < 16 && !leaves; ++k)
                {
                    const auto t = static_cast<double>(k) / 16.0;
                    const Point at{from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t};
                    // A sample on the boundary has not left: a segment running along a
                    // wall is inside the area for every purpose visibility cares about.
                    auto on_a_ring = false;
                    for (const auto &ring : rings)
                    {
                        for (std::size_t i = 0; i < ring.size() && !on_a_ring; ++i)
                        {
                            const auto &a = ring[i];
                            const auto &b = ring[(i + 1) % ring.size()];
                            const auto dx = b.x - a.x, dy = b.y - a.y;
                            const auto len = dx * dx + dy * dy;
                            auto u = 0.0;
                            if (len > 0.0)
                                u = std::clamp(
                                    ((at.x - a.x) * dx + (at.y - a.y) * dy) / len, 0.0, 1.0);
                            const auto ex = at.x - (a.x + u * dx), ey = at.y - (a.y + u * dy);
                            on_a_ring = ex * ex + ey * ey < 1e-18;
                        }
                    }
                    leaves = !inside_area(at, rings) && !on_a_ring;
                }
                if (leaves)
                    continue;

                ++checked;
                BOOST_CHECK_MESSAGE(std::find(reported.begin(), reported.end(), w) !=
                                        reported.end(),
                                    "vertex " << v << " should see vertex " << w);
            }
        }
    }

    BOOST_CHECK_GT(checked, 100u);
}

BOOST_AUTO_TEST_SUITE_END()
