#include "engine/area_clearance.hpp"

#include <boost/test/unit_test.hpp>

#include <cmath>
#include <span>
#include <vector>

BOOST_AUTO_TEST_SUITE(area_clearance_test)

using namespace osrm;
using namespace osrm::engine::area;

namespace
{

// A 10x10 square with a 2x2 block in the middle, so every answer below can be worked out
// on paper.
struct Fixture
{
    std::vector<Point> outer{{0, 0}, {10, 0}, {10, 10}, {0, 10}};
    std::vector<Point> block{{4, 4}, {6, 4}, {6, 6}, {4, 6}};
    std::vector<Ring> rings;

    explicit Fixture(bool with_block = true)
    {
        rings.emplace_back(outer);
        if (with_block)
            rings.emplace_back(block);
    }
};

constexpr double TOLERANCE = 1e-9;

} // namespace

BOOST_AUTO_TEST_CASE(distance_to_a_wall)
{
    const Fixture f{false};

    // Nearest wall is the one 1 away, not the three that are further.
    const auto near_left = clearance({1, 5}, f.rings);
    BOOST_CHECK_CLOSE(near_left.distance, 1.0, TOLERANCE);
    BOOST_CHECK_CLOSE(near_left.nearest.x, 0.0, TOLERANCE);
    BOOST_CHECK_CLOSE(near_left.nearest.y, 5.0, TOLERANCE);
    // The gradient points away from the wall, into the square.
    BOOST_CHECK_CLOSE(near_left.gradient.x, 1.0, TOLERANCE);
    BOOST_CHECK_SMALL(near_left.gradient.y, 1e-12);

    // Dead centre of an empty square: 5 from all four walls.
    BOOST_CHECK_CLOSE(clearance({5, 5}, f.rings).distance, 5.0, TOLERANCE);
}

BOOST_AUTO_TEST_CASE(distance_to_a_corner)
{
    const Fixture f{false};

    // Diagonally off the corner, so the point projects beyond the end of both edges and
    // the nearest point is the corner itself.  This is the case an unclamped projection
    // onto the infinite line gets wrong, and it answers with a distance that is too small.
    const auto at = clearance({-1, -1}, f.rings);
    BOOST_CHECK_CLOSE(at.distance, std::sqrt(2.0), TOLERANCE);
    BOOST_CHECK_SMALL(at.nearest.x, 1e-12);
    BOOST_CHECK_SMALL(at.nearest.y, 1e-12);

    // Inside, near the same corner, both edges are in span and the answer is the wall at
    // 1 rather than the corner at sqrt(2).  Worth pinning: it is the mistake this test
    // was written with.
    BOOST_CHECK_CLOSE(clearance({1, 1}, f.rings).distance, 1.0, TOLERANCE);
}

BOOST_AUTO_TEST_CASE(the_block_counts_as_much_as_the_walls)
{
    const Fixture f;

    // 5,2 is 2 from the wall below and 2 from the block above: a tie, and the first
    // segment scanned wins it.  What matters is that it is one of the two and that the
    // distance is right.
    const auto tie = clearance({5, 2}, f.rings);
    BOOST_CHECK_CLOSE(tie.distance, 2.0, TOLERANCE);

    // Just inside the block's shadow the block wins outright.
    const auto blocked = clearance({5, 3}, f.rings);
    BOOST_CHECK_CLOSE(blocked.distance, 1.0, TOLERANCE);
    BOOST_CHECK_EQUAL(blocked.ring, 1u);
    BOOST_CHECK_CLOSE(blocked.gradient.y, -1.0, TOLERANCE);
}

BOOST_AUTO_TEST_CASE(a_point_on_the_geometry_has_no_gradient)
{
    const Fixture f;

    const auto on_wall = clearance({0, 5}, f.rings);
    BOOST_CHECK_SMALL(on_wall.distance, 1e-12);
    BOOST_CHECK_SMALL(on_wall.gradient.x, 1e-12);
    BOOST_CHECK_SMALL(on_wall.gradient.y, 1e-12);

    const auto on_corner = clearance({4, 4}, f.rings);
    BOOST_CHECK_SMALL(on_corner.distance, 1e-12);
}

BOOST_AUTO_TEST_CASE(a_point_outside_gets_its_distance_to_the_boundary)
{
    const Fixture f{false};

    // Unsigned on purpose.  A coordinate a person asked about can be just outside the
    // plaza, and answering with a negative number would make every comparison against a
    // margin do the wrong thing silently.
    const auto outside = clearance({-3, 5}, f.rings);
    BOOST_CHECK_CLOSE(outside.distance, 3.0, TOLERANCE);
    BOOST_CHECK_CLOSE(outside.gradient.x, -1.0, TOLERANCE);
}

BOOST_AUTO_TEST_CASE(inside_the_block_measures_to_the_block)
{
    const Fixture f;

    // The block's own centre is 1 from its walls.  Nothing here knows about inside and
    // outside, which is what makes the answer usable for a point that has strayed.
    BOOST_CHECK_CLOSE(clearance({5, 5}, f.rings).distance, 1.0, TOLERANCE);
}

BOOST_AUTO_TEST_CASE(degenerate_rings_do_not_divide_by_zero)
{
    std::vector<Point> repeated{{0, 0}, {0, 0}, {10, 0}, {10, 10}, {0, 10}};
    std::vector<Ring> rings{repeated};

    const auto at = clearance({5, 5}, rings);
    BOOST_CHECK_CLOSE(at.distance, 5.0, TOLERANCE);
    BOOST_CHECK(std::isfinite(at.gradient.x));
    BOOST_CHECK(std::isfinite(at.gradient.y));

    // A ring with fewer than two points contributes nothing rather than crashing.
    std::vector<Point> single{{1, 1}};
    std::vector<Ring> with_single{repeated, single};
    BOOST_CHECK_CLOSE(clearance({5, 5}, with_single).distance, 5.0, TOLERANCE);
}

BOOST_AUTO_TEST_CASE(no_rings_at_all)
{
    const auto nothing = clearance({5, 5}, {});
    BOOST_CHECK_SMALL(nothing.distance, 1e-12);
}

BOOST_AUTO_TEST_CASE(the_gradient_is_the_direction_clearance_grows_in)
{
    const Fixture f;

    // Checked by finite difference rather than by construction: step along the reported
    // gradient and the clearance must increase by the length of the step, because the
    // gradient of a distance function is a unit vector.
    const Point at{2.5, 5.0};
    const auto here = clearance(at, f.rings);
    constexpr double STEP = 1e-6;

    const Point forward{at.x + here.gradient.x * STEP, at.y + here.gradient.y * STEP};
    const auto there = clearance(forward, f.rings);
    BOOST_CHECK_CLOSE(there.distance - here.distance, STEP, 1e-4);

    // And the other way, which is the direction the repulsion force will push against.
    const Point back{at.x - here.gradient.x * STEP, at.y - here.gradient.y * STEP};
    BOOST_CHECK_CLOSE(here.distance - clearance(back, f.rings).distance, STEP, 1e-4);
}

BOOST_AUTO_TEST_CASE(projected_units_convert_to_metres)
{
    // At the equator a projected unit is a degree of longitude.
    BOOST_CHECK_CLOSE(metres_per_projected_unit(0.0), 111194.9, 0.1);

    // And it shrinks with the cosine of the latitude, which is what makes the conversion
    // a property of the area rather than a constant.
    BOOST_CHECK_CLOSE(metres_per_projected_unit(60.0), metres_per_projected_unit(0.0) / 2, 0.1);

    // Symmetric, and defined at the poles rather than zero or NaN.
    BOOST_CHECK_CLOSE(metres_per_projected_unit(-45.0), metres_per_projected_unit(45.0), 1e-9);
    BOOST_CHECK_GT(metres_per_projected_unit(90.0), 0.0);
}

BOOST_AUTO_TEST_SUITE_END()
