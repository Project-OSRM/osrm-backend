#include "engine/area_visibility.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
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

BOOST_AUTO_TEST_SUITE_END()
