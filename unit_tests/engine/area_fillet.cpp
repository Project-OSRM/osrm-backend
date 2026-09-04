#include "engine/area_fillet.hpp"

#include "engine/area_clearance.hpp"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

BOOST_AUTO_TEST_SUITE(area_fillet_test)

using namespace osrm;
using namespace osrm::engine::area;

namespace
{

// A 100x100 square with a 20x20 block in the middle.
struct Blocked
{
    std::vector<Point> outer{{0, 0}, {100, 0}, {100, 100}, {0, 100}};
    std::vector<Point> block{{40, 40}, {60, 40}, {60, 60}, {40, 60}};
    std::vector<Ring> rings;
    Blocked()
    {
        rings.emplace_back(outer);
        rings.emplace_back(block);
    }
};

double sharpest_turn(const std::vector<Point> &points)
{
    auto worst = 0.0;
    for (std::size_t i = 1; i + 1 < points.size(); ++i)
    {
        const auto ax = points[i].x - points[i - 1].x, ay = points[i].y - points[i - 1].y;
        const auto bx = points[i + 1].x - points[i].x, by = points[i + 1].y - points[i].y;
        const auto la = std::hypot(ax, ay), lb = std::hypot(bx, by);
        if (la == 0.0 || lb == 0.0)
            continue;
        worst = std::max(worst, std::acos(std::clamp((ax * bx + ay * by) / (la * lb), -1.0, 1.0)) *
                                    180.0 / M_PI);
    }
    return worst;
}

double tightest_clearance(const std::vector<Point> &points, std::span<const Ring> rings)
{
    auto tightest = 1e18;
    for (std::size_t i = 1; i + 1 < points.size(); ++i)
        tightest = std::min(tightest, clearance(points[i], rings).distance);
    return tightest;
}

} // namespace

BOOST_AUTO_TEST_CASE(a_straight_path_is_handed_back_as_it_is)
{
    const Blocked plaza;
    const std::vector<Point> line{{10, 10}, {30, 10}};
    const auto out = round_corners(line, plaza.rings, 5.0);
    BOOST_REQUIRE_EQUAL(out.points.size(), 2u);
    BOOST_CHECK(out.certified);
}

BOOST_AUTO_TEST_CASE(a_corner_becomes_an_arc_and_the_runs_stay_straight)
{
    const Blocked plaza;
    // Round the block's south-east corner, from the low corner of the square to the high.
    const std::vector<Point> taut{{20, 20}, {60, 40}, {80, 80}};
    const auto out = round_corners(taut, plaza.rings, 5.0);
    BOOST_REQUIRE(out.certified);
    BOOST_CHECK_GT(out.points.size(), 3u);
    BOOST_CHECK(out.points.front().x == 20 && out.points.front().y == 20);
    BOOST_CHECK(out.points.back().x == 80 && out.points.back().y == 80);

    // The corner is held off the block by about the margin, and turned gently.
    BOOST_CHECK_GT(tightest_clearance(out.points, plaza.rings), 4.0);
    BOOST_CHECK_LT(sharpest_turn(out.points), 15.0);
    BOOST_CHECK_LT(sharpest_turn(out.points), sharpest_turn(taut));

    // Straight lines and arcs and nothing else: every interior point is either on the
    // arc, at the margin's radius from one centre, or the tangent point either side.
    // Checked the simple way: no three consecutive interior points are collinear
    // except where the two runs meet the arc, so the turning never changes sign.
    auto previous = 0.0;
    for (std::size_t i = 1; i + 1 < out.points.size(); ++i)
    {
        const auto ax = out.points[i].x - out.points[i - 1].x,
                   ay = out.points[i].y - out.points[i - 1].y;
        const auto bx = out.points[i + 1].x - out.points[i].x,
                   by = out.points[i + 1].y - out.points[i].y;
        const auto sign = ax * by - ay * bx;
        BOOST_CHECK(!(previous * sign < 0.0));
        if (std::abs(sign) > 1e-9)
            previous = sign;
    }
}

BOOST_AUTO_TEST_CASE(a_passage_narrower_than_the_margin_gets_what_fits)
{
    // A block 6 units from the wall, and a path through the gap turning on the block's
    // corners: the full offset would leave the square, so it backs off to what is legal.
    std::vector<Point> outer{{0, 0}, {100, 0}, {100, 100}, {0, 100}};
    std::vector<Point> block{{30, 6}, {70, 6}, {70, 40}, {30, 40}};
    std::vector<Ring> rings{Ring{outer}, Ring{block}};
    const std::vector<Point> taut{{10, 20}, {30, 6}, {70, 6}, {90, 20}};
    const auto out = round_corners(taut, rings, 5.0);
    BOOST_REQUIRE(out.certified);
    for (const auto &p : out.points)
    {
        BOOST_CHECK(in_closed_area(p, rings, ON_GEOMETRY));
    }
    BOOST_CHECK_GT(out.points.size(), 4u);
}

BOOST_AUTO_TEST_CASE(round_corners_is_deterministic)
{
    const Blocked plaza;
    const std::vector<Point> taut{{20, 20}, {60, 40}, {80, 80}};
    const auto once = round_corners(taut, plaza.rings, 5.0);
    const auto twice = round_corners(taut, plaza.rings, 5.0);
    BOOST_REQUIRE_EQUAL(once.points.size(), twice.points.size());
    for (std::size_t i = 0; i < once.points.size(); ++i)
    {
        BOOST_CHECK(once.points[i].x == twice.points[i].x && once.points[i].y == twice.points[i].y);
    }
}

BOOST_AUTO_TEST_SUITE_END()
