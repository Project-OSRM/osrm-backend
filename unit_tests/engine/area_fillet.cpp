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

util::Coordinate at_metres_(const double x, const double y)
{
    constexpr double METRES_PER_DEGREE_ = 111194.9;
    return {util::FloatLongitude{x / METRES_PER_DEGREE_},
            util::FloatLatitude{y / METRES_PER_DEGREE_}};
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
    BOOST_CHECK(out.legal);
}

BOOST_AUTO_TEST_CASE(a_corner_becomes_an_arc_and_the_runs_stay_straight)
{
    const Blocked plaza;
    // Round the block's south-east corner, from the low corner of the square to the high.
    const std::vector<Point> taut{{20, 20}, {60, 40}, {80, 80}};
    const auto out = round_corners(taut, plaza.rings, 5.0);
    BOOST_REQUIRE(out.legal);
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
    BOOST_REQUIRE(out.legal);
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


// ---- the legality test the construction relies on ------------------------------------

// An anchor on a corner of the block, with the segment leaving it straight through the
// block.  A proper-crossing test cannot see this: the anchor is a vertex of the ring and
// shares an endpoint with the two edges there, which such a test exempts.
BOOST_AUTO_TEST_CASE(a_segment_from_a_corner_into_the_obstacle_is_not_legal)
{
    const Blocked plaza;
    const std::vector<Point> diagonal{{40, 40}, {60, 60}};
    BOOST_CHECK(!path_in_closed_area(diagonal, plaza.rings));

    // Leaving the same corner into the open is legal.
    const std::vector<Point> away{{40, 40}, {20, 20}};
    BOOST_CHECK(path_in_closed_area(away, plaza.rings));

    // Along an edge of the block: legal ground.  The free space is closed, so a path may
    // run against an obstacle, which is what a shortest path does.
    const std::vector<Point> grazing{{40, 40}, {60, 40}};
    BOOST_CHECK(path_in_closed_area(grazing, plaza.rings));

    // The whole way round the block, touching it for the entire length: still legal.
    const std::vector<Point> around{{40, 40}, {60, 40}, {60, 60}, {40, 60}, {40, 40}};
    BOOST_CHECK(path_in_closed_area(around, plaza.rings));

    // Along the outer wall of the square, which is boundary too.
    const std::vector<Point> wall{{0, 0}, {100, 0}};
    BOOST_CHECK(path_in_closed_area(wall, plaza.rings));

    // Leaving the plaza altogether is not legal, however much room is on the far side.
    const std::vector<Point> outside{{50, 10}, {50, -10}};
    BOOST_CHECK(!path_in_closed_area(outside, plaza.rings));

    // Cutting the corner of the block: legal at both ends, through the obstacle between.
    const std::vector<Point> corner{{50, 40}, {40, 50}};
    BOOST_CHECK(!path_in_closed_area(corner, plaza.rings));

    // What round_corners() does with a path through the block: hands it back, saying so.
    const std::vector<Point> through{{40, 40}, {50, 50}, {60, 60}};
    const auto out = round_corners(through, plaza.rings, 5.0);
    BOOST_CHECK(!out.legal);
    BOOST_CHECK_EQUAL(out.points.size(), through.size());

    // A taut path grazes the geometry, which is legal, and its corners get rounded.  This
    // one is genuinely taut, the shortest way from one side of the block to the other
    // below it: the geometry is on the inside of both turns, which is what a shortest
    // path looks like and what the corner offset assumes.
    const std::vector<Point> taut{{20, 35}, {40, 40}, {60, 40}, {70, 55}};
    const auto rounded = round_corners(taut, plaza.rings, 5.0);
    BOOST_CHECK(rounded.legal);
    BOOST_CHECK_GT(rounded.points.size(), taut.size());
}

// ---- pulling taut: what a path from anything but the visibility graph needs first --------

BOOST_AUTO_TEST_CASE(a_wobble_in_the_open_is_pulled_to_a_straight_line)
{
    const Blocked plaza;
    const std::vector<Point> wobble{{10, 10}, {20, 12}, {30, 8}, {40, 12}, {50, 10}};
    const auto pulled = pull_taut(wobble, plaza.rings);
    BOOST_REQUIRE_EQUAL(pulled.size(), 2u);
    BOOST_CHECK(pulled.front().x == 10 && pulled.back().x == 50);

    // and rounding it draws nothing new: a straight line is handed back as it is
    const auto out = round_corners(wobble, plaza.rings, 5.0);
    BOOST_CHECK(out.legal);
    BOOST_CHECK_EQUAL(out.points.size(), 2u);
}

BOOST_AUTO_TEST_CASE(a_staircase_round_the_block_is_pulled_to_the_taut_path)
{
    const Blocked plaza;
    // A grid search's way past the block: steps that a straight line could replace, up
    // to the corner it has to turn at, then steps again.
    const std::vector<Point> staircase{
        {20, 20}, {30, 25}, {40, 30}, {50, 35}, {60, 40}, {65, 50}, {70, 60}, {80, 80}};
    const std::vector<Point> taut{{20, 20}, {60, 40}, {80, 80}};

    const auto pulled = pull_taut(staircase, plaza.rings);
    BOOST_REQUIRE_EQUAL(pulled.size(), 3u);
    BOOST_CHECK(pulled[1].x == 60 && pulled[1].y == 40);

    // So the staircase is drawn exactly as the taut path is.
    const auto from_stairs = round_corners(staircase, plaza.rings, 5.0);
    const auto from_taut = round_corners(taut, plaza.rings, 5.0);
    BOOST_REQUIRE(from_stairs.legal && from_taut.legal);
    BOOST_REQUIRE_EQUAL(from_stairs.points.size(), from_taut.points.size());
    for (std::size_t i = 0; i < from_taut.points.size(); ++i)
    {
        BOOST_CHECK_SMALL(from_stairs.points[i].x - from_taut.points[i].x, 1e-9);
        BOOST_CHECK_SMALL(from_stairs.points[i].y - from_taut.points[i].y, 1e-9);
    }
}

BOOST_AUTO_TEST_CASE(a_taut_path_is_left_alone_by_pulling)
{
    const Blocked plaza;
    // Every vertex forced: the chord across either corner cuts into the block.
    const std::vector<Point> taut{{20, 45}, {40, 40}, {60, 40}, {70, 55}};
    const auto pulled = pull_taut(taut, plaza.rings);
    BOOST_REQUIRE_EQUAL(pulled.size(), taut.size());
    // and a path through the obstacle is kept as it is, since no shortcut is legal
    const std::vector<Point> through{{40, 40}, {50, 50}, {60, 60}};
    BOOST_CHECK_EQUAL(pull_taut(through, plaza.rings).size(), 3u);
    BOOST_CHECK(!round_corners(through, plaza.rings, 5.0).legal);
}

BOOST_AUTO_TEST_CASE(pulling_tightens_the_string_onto_the_corner)
{
    // A legal path past the block's south-east corner that never touches it: the
    // vertex at (63,41) is wide of the corner at (60,40), and dropping vertices alone
    // would leave it there.  Tightening puts the string on the corner: the result is
    // the geodesic round the block, three vertices, the middle one exactly the corner.
    const Blocked plaza;
    const std::vector<Point> loose{{20, 20}, {45, 33}, {63, 41}, {80, 80}};
    const auto pulled = pull_taut(loose, plaza.rings);
    BOOST_REQUIRE_EQUAL(pulled.size(), 3u);
    BOOST_CHECK_SMALL(pulled[1].x - 60.0, 1e-9);
    BOOST_CHECK_SMALL(pulled[1].y - 40.0, 1e-9);

    // and it does not take the shortcut on the far side: a wander round the block's
    // north stays north, since sweeping across the block is not a clean move
    const std::vector<Point> north{{20, 20}, {30, 62}, {50, 66}, {70, 62}, {80, 20}};
    const auto round_the_top = pull_taut(north, plaza.rings);
    BOOST_REQUIRE_GE(round_the_top.size(), 3u);
    for (std::size_t i = 1; i + 1 < round_the_top.size(); ++i)
    {
        BOOST_CHECK_GE(round_the_top[i].y, 60.0 - 1e-9);
    }
    BOOST_CHECK(path_in_closed_area(round_the_top, plaza.rings));
}

BOOST_AUTO_TEST_CASE(pulling_hops_a_small_obstacle_but_not_a_large_one)
{
    // A tree pit two units across in the middle of the square's south, and a path that
    // goes round its north.  The chord along y = 15 passes it on the south and is legal.
    std::vector<Point> outer{{0, 0}, {100, 0}, {100, 100}, {0, 100}};
    std::vector<Point> pit{{49, 19}, {51, 19}, {51, 21}, {49, 21}};
    std::vector<Point> block{{40, 40}, {60, 40}, {60, 60}, {40, 60}};
    std::vector<Ring> rings{Ring{outer}, Ring{pit}, Ring{block}};
    const std::vector<Point> round_the_pit{{10, 15}, {50, 21.5}, {90, 15}};

    // With no hop the side is kept: the vertex stays, tightened onto the pit's corner.
    const auto kept = pull_taut(round_the_pit, rings, 0.0);
    BOOST_REQUIRE_EQUAL(kept.size(), 3u);
    BOOST_CHECK_GE(kept[1].y, 21.0 - 1e-9);
    // With a hop at least the pit's size across, the pit is passed on the shorter side.
    const auto hopped = pull_taut(round_the_pit, rings, 3.0);
    BOOST_CHECK_EQUAL(hopped.size(), 2u);

    // The block is twenty across and is never hopped at that size: a path over its north
    // stays north, even though the chord along its south is legal.
    const std::vector<Point> over_the_block{{20, 35}, {50, 65}, {80, 35}};
    const auto north = pull_taut(over_the_block, rings, 3.0);
    BOOST_REQUIRE_EQUAL(north.size(), 3u);
    BOOST_CHECK_GE(north[1].y, 60.0 - 1e-9);
    // and round_corners() with the same hop agrees, and without one keeps the side
    BOOST_CHECK_EQUAL(round_corners(round_the_pit, rings, 3.0, 3.0).points.size(), 2u);
    BOOST_CHECK_GT(round_corners(round_the_pit, rings, 3.0).points.size(), 2u);
}

BOOST_AUTO_TEST_CASE(pulling_hops_an_obstacle_it_wraps_with_several_vertices)
{
    // The path stands on two corners of the pit, so the chord across either one of them
    // cuts the pit itself; only a chord from before the pit to after it can pass it on
    // the other side, and that is the one the pass has to find.
    std::vector<Point> outer{{0, 0}, {100, 0}, {100, 100}, {0, 100}};
    std::vector<Point> pit{{49, 19}, {51, 19}, {51, 21}, {49, 21}};
    std::vector<Ring> rings{Ring{outer}, Ring{pit}};
    const std::vector<Point> wrapped{{10, 15}, {49, 21}, {51, 21}, {90, 15}};
    BOOST_CHECK_EQUAL(pull_taut(wrapped, rings, 0.0).size(), 4u);
    BOOST_CHECK_EQUAL(pull_taut(wrapped, rings, 3.0).size(), 2u);
}

BOOST_AUTO_TEST_CASE(pulling_keeps_the_anchors_and_copes_with_nothing)
{
    const Blocked plaza;
    BOOST_CHECK(pull_taut(std::vector<Point>{}, plaza.rings).empty());
    const std::vector<Point> one{{50, 20}};
    BOOST_CHECK_EQUAL(pull_taut(one, plaza.rings).size(), 1u);
    const std::vector<Point> two{{10, 10}, {90, 10}};
    BOOST_CHECK_EQUAL(pull_taut(two, plaza.rings).size(), 2u);
    // the anchors are the input's own, whatever happens between them
    const std::vector<Point> loop{{10, 10}, {30, 30}, {50, 10}, {30, 5}, {90, 10}};
    const auto pulled = pull_taut(loop, plaza.rings);
    BOOST_CHECK(pulled.front().x == 10 && pulled.front().y == 10);
    BOOST_CHECK(pulled.back().x == 90 && pulled.back().y == 10);
    BOOST_CHECK_EQUAL(pulled.size(), 2u);
}

BOOST_AUTO_TEST_CASE(pulling_drops_collinear_vertices_along_a_wall)
{
    // Along the south edge of the block, vertex by vertex: every segment grazes the
    // block, which is legal, and the chord across all of them grazes it just the same.
    const Blocked plaza;
    const std::vector<Point> along{{20, 45}, {40, 40}, {45, 40}, {50, 40}, {55, 40}, {60, 40}, {70, 55}};
    const auto pulled = pull_taut(along, plaza.rings);
    BOOST_REQUIRE_EQUAL(pulled.size(), 4u);
    BOOST_CHECK(pulled[1].x == 40 && pulled[2].x == 60);
}

BOOST_AUTO_TEST_CASE(pulling_drops_vertices_that_a_clean_chord_replaces)
{
    // (30,30) and (35,35) lie on the way to the corner and go; (60,40) is the corner
    // and stays; the doubling back at the end is pulled straight since the block is
    // not between the chord and the path there.
    const Blocked plaza;
    const std::vector<Point> path{{20, 20}, {30, 30}, {35, 35}, {60, 40}, {80, 80}, {65, 65}};
    const auto pulled = pull_taut(path, plaza.rings);
    BOOST_REQUIRE_EQUAL(pulled.size(), 3u);
    BOOST_CHECK(pulled[1].x == 60 && pulled[1].y == 40);
}

BOOST_AUTO_TEST_CASE(a_wobbly_path_given_as_coordinates_comes_back_pulled)
{
    // The engine's entry point returns the pulled path even where no arc is drawn,
    // since a straight line is not what it was given.
    const std::vector<util::Coordinate> outer{
        at_metres_(0, 0), at_metres_(100, 0), at_metres_(100, 100), at_metres_(0, 100)};
    const std::vector<std::span<const util::Coordinate>> rings{outer};
    const std::vector<util::Coordinate> wobble{
        at_metres_(10, 10), at_metres_(20, 12), at_metres_(30, 8), at_metres_(40, 12), at_metres_(50, 10)};
    const auto drawn = round_corners(rings, wobble, 2.0);
    BOOST_REQUIRE(drawn);
    BOOST_CHECK_EQUAL(drawn->size(), 2u);
    BOOST_CHECK(drawn->front() == wobble.front() && drawn->back() == wobble.back());
}

BOOST_AUTO_TEST_CASE(two_corners_close_together_are_backed_off_rather_than_folded)
{
    // Two corners of the block ten units apart, and a margin that would move each of
    // them further than that: the run between them would swing round, so the offsets
    // are backed off until it does not, and the result is legal and no sharper than
    // the taut path.
    const Blocked plaza;
    const std::vector<Point> taut{{10, 20}, {40, 40}, {60, 40}, {90, 20}};
    // not taut in the shortest-path sense, but every vertex is forced: the chord across
    // (40,40) from (10,20) to (60,40) grazes the block's edge and is legal -- so pull it
    // first, and what is left is what gets rounded
    const auto out = round_corners(taut, plaza.rings, 20.0);
    BOOST_REQUIRE(out.legal);
    for (const auto &p : out.points)
    {
        BOOST_CHECK(in_closed_area(p, plaza.rings, ON_GEOMETRY));
    }
    BOOST_CHECK_LE(sharpest_turn(out.points), sharpest_turn(pull_taut(taut, plaza.rings)) + 1e-9);
}

BOOST_AUTO_TEST_CASE(degenerate_input_is_handed_back)
{
    const Blocked plaza;
    const std::vector<Point> single{{50, 20}};
    BOOST_CHECK_EQUAL(round_corners(single, plaza.rings, 5.0).points.size(), 1u);
    const std::vector<Point> pair{{20, 20}, {80, 20}};
    BOOST_CHECK_EQUAL(round_corners(pair, plaza.rings, 5.0).points.size(), 2u);
    // Two coincident points: nothing to round.
    const std::vector<Point> same{{50, 20}, {50, 20}};
    const auto out = round_corners(same, plaza.rings, 5.0);
    BOOST_CHECK_EQUAL(out.points.size(), 2u);
    BOOST_CHECK(out.legal);
    // No margin: as given.
    const std::vector<Point> taut{{20, 20}, {60, 40}, {80, 80}};
    BOOST_CHECK_EQUAL(round_corners(taut, plaza.rings, 0.0).points.size(), 3u);
}

// ---- round_corners(), for coordinates: what the engine calls ---------------------------

namespace
{

//! Metres per degree at the equator, so a fixture can be laid out in metres.
constexpr double METRES_PER_DEGREE = 111194.9;

util::Coordinate at_metres(const double x, const double y)
{
    return {util::FloatLongitude{x / METRES_PER_DEGREE},
            util::FloatLatitude{y / METRES_PER_DEGREE}};
}

// The Blocked fixture, laid out in coordinates: a 100 m square with a 20 m block in the
// middle, and the taut path from the low corner to the high one, which turns on the
// block's south-east corner.
struct BlockedInCoordinates
{
    std::vector<util::Coordinate> outer{
        at_metres(0, 0), at_metres(100, 0), at_metres(100, 100), at_metres(0, 100)};
    std::vector<util::Coordinate> block{
        at_metres(40, 40), at_metres(60, 40), at_metres(60, 60), at_metres(40, 60)};
    std::vector<std::span<const util::Coordinate>> rings;
    std::vector<util::Coordinate> taut{at_metres(20, 20), at_metres(60, 40), at_metres(80, 80)};
    BlockedInCoordinates()
    {
        rings.emplace_back(outer);
        rings.emplace_back(block);
    }
    std::vector<Point> projected_outer = projected(outer);
    std::vector<Point> projected_block = projected(block);
    std::vector<Ring> projected_rings() const
    {
        return {Ring{projected_outer}, Ring{projected_block}};
    }
    static std::vector<Point> projected(const std::vector<util::Coordinate> &coordinates)
    {
        std::vector<Point> points;
        for (const auto c : coordinates)
        {
            points.push_back(project(c));
        }
        return points;
    }
};

double path_length(const std::vector<Point> &points)
{
    auto total = 0.0;
    for (std::size_t i = 1; i < points.size(); ++i)
        total += std::hypot(points[i].x - points[i - 1].x, points[i].y - points[i - 1].y);
    return total;
}

} // namespace

BOOST_AUTO_TEST_CASE(no_margin_draws_the_path_as_given)
{
    const BlockedInCoordinates plaza;
    BOOST_CHECK(!round_corners(plaza.rings, plaza.taut, 0.0));
    BOOST_CHECK(!round_corners(plaza.rings, plaza.taut, -1.0));
}

BOOST_AUTO_TEST_CASE(a_straight_line_across_the_open_is_not_redrawn)
{
    // Nothing to round on a straight line, and the answer says so rather than hand back
    // a copy to be carried as computed.
    const BlockedInCoordinates plaza;
    const std::vector<util::Coordinate> line{at_metres(5, 5), at_metres(30, 5)};
    BOOST_CHECK(!round_corners(plaza.rings, line, 2.0));
}

BOOST_AUTO_TEST_CASE(a_corner_is_rounded_and_the_anchors_are_kept_exactly)
{
    const BlockedInCoordinates plaza;
    const auto drawn = round_corners(plaza.rings, plaza.taut, 2.0);
    BOOST_REQUIRE(drawn);
    BOOST_CHECK_GT(drawn->size(), plaza.taut.size());

    // Anchors are the traveller's own coordinates and do not drift by a projection.
    BOOST_CHECK(drawn->front() == plaza.taut.front());
    BOOST_CHECK(drawn->back() == plaza.taut.back());

    // Every point is on legal ground, and the corner it rounds is rounded: the sharpest
    // turn is gentler than the 37 degrees the taut path turns at the block's corner.
    const auto rings = plaza.projected_rings();
    std::vector<Point> points;
    for (const auto c : *drawn)
    {
        points.push_back(project(c));
    }
    for (const auto &p : points)
    {
        BOOST_CHECK(in_closed_area(p, rings, ON_GEOMETRY));
    }
    std::vector<Point> taut;
    for (const auto c : plaza.taut)
    {
        taut.push_back(project(c));
    }
    BOOST_CHECK_LT(sharpest_turn(points), sharpest_turn(taut));
    // and a little longer, which is what the margin costs
    BOOST_CHECK_GE(path_length(points), path_length(taut));
}

BOOST_AUTO_TEST_CASE(rounding_coordinates_is_deterministic)
{
    const BlockedInCoordinates plaza;
    const auto once = round_corners(plaza.rings, plaza.taut, 2.0);
    const auto twice = round_corners(plaza.rings, plaza.taut, 2.0);
    BOOST_REQUIRE(once && twice);
    BOOST_REQUIRE_EQUAL(once->size(), twice->size());
    for (std::size_t i = 0; i < once->size(); ++i)
    {
        BOOST_CHECK((*once)[i] == (*twice)[i]);
    }
}

BOOST_AUTO_TEST_SUITE_END()
