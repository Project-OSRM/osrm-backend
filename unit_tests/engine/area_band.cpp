#include "engine/area_band.hpp"

#include "engine/area_clearance.hpp"

#include <boost/test/unit_test.hpp>

#include <cmath>
#include <limits>
#include <span>
#include <vector>

BOOST_AUTO_TEST_SUITE(area_band_test)

using namespace osrm;
using namespace osrm::engine::area;

namespace
{

// A 100x100 square, big enough that a comfort margin of 5 leaves plenty of open ground.
struct Square
{
    std::vector<Point> outer{{0, 0}, {100, 0}, {100, 100}, {0, 100}};
    std::vector<Ring> rings;
    Square() { rings.emplace_back(outer); }
};

// The same square with a 20x20 block in the middle, so a path across it has to bend.
//
// The taut paths below clear the block by half a unit rather than grazing it. That is
// what planning on eroded geometry gives and what the band is specified to receive: the
// shortest way past a rectangle runs along its edges, so an unmodified taut path has
// interior nodes at zero clearance where a bubble has no radius at all.
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


//! How much of a polyline's turning doubles back on itself, in degrees.
double doubling_back(const std::vector<Point> &points)
{
    auto total = 0.0;
    auto previous = 0.0;
    for (std::size_t i = 1; i + 1 < points.size(); ++i)
    {
        const auto ax = points[i].x - points[i - 1].x, ay = points[i].y - points[i - 1].y;
        const auto bx = points[i + 1].x - points[i].x, by = points[i + 1].y - points[i].y;
        const auto la = std::hypot(ax, ay), lb = std::hypot(bx, by);
        if (!(la > 0.0) || !(lb > 0.0))
        {
            continue;
        }
        const auto angle =
            std::acos(std::clamp((ax * bx + ay * by) / (la * lb), -1.0, 1.0)) * 180.0 / M_PI;
        const auto sign = ax * by - ay * bx;
        if (previous * sign < 0.0)
        {
            total += angle;
        }
        if (sign != 0.0)
        {
            previous = sign;
        }
    }
    return total;
}

BandParameters defaults()
{
    BandParameters p;
    p.comfort = 5.0;
    p.contraction = 1.0;
    p.repulsion = 1.0;
    p.sweeps = 60;
    // Stated rather than left to the default, because these fixtures are in the regime
    // where the hard margin dominates: a comfort margin of 5 on a 100 wide square is
    // enormous relative to the geometry, and there twice the margin is the right spacing.
    // The default is a quarter of it, which suits the realistic regime where the comfort
    // margin is metres and the plaza is hundreds. See BandParameters::spacing.
    p.spacing = p.comfort * 2.0;
    return p;
}

double path_length(const std::vector<Point> &points)
{
    auto total = 0.0;
    for (std::size_t i = 0; i + 1 < points.size(); ++i)
    {
        total += std::hypot(points[i + 1].x - points[i].x, points[i + 1].y - points[i].y);
    }
    return total;
}

//! The sharpest turn at any interior node, in degrees. Zero for a straight line.
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
        const auto cosine = std::clamp((ax * bx + ay * by) / (la * lb), -1.0, 1.0);
        worst = std::max(worst, std::acos(cosine) * 180.0 / M_PI);
    }
    return worst;
}

} // namespace

BOOST_AUTO_TEST_CASE(an_open_square_leaves_a_straight_line_straight)
{
    const Square square;
    // The taut path across an empty square is the straight line, so this is the input the
    // band is actually given there.
    const std::vector<Point> path{{20, 50}, {80, 50}};

    const auto band = smooth(path, square.rings, defaults());

    BOOST_REQUIRE_GE(band.points.size(), 2u);
    // Nothing is near enough to push, so tension alone survives and it stays straight.
    BOOST_CHECK_LT(sharpest_turn(band.points), 1.0);
    const auto direct = std::hypot(80.0 - 20.0, 50.0 - 50.0);
    BOOST_CHECK_LT(path_length(band.points), direct * 1.01);
}

BOOST_AUTO_TEST_CASE(a_detour_in_the_open_is_pulled_straight)
{
    const Square square;
    // A twenty unit detour through an empty square, which no planner would produce.
    const std::vector<Point> detour{{20, 50}, {50, 70}, {80, 50}};

    const auto band = smooth(detour, square.rings, defaults());

    // With nothing in the way, tension has no opposition and pulls it to the chord.
    //
    // An earlier version of this test asserted the opposite, on the grounds that the band
    // is a local deformation and tension propagates one node per sweep. That was a
    // description of an implementation that had not converged rather than of a contract:
    // with the node spacing following the comfort margin the band is short enough to
    // reach equilibrium inside the sweep budget, and it straightens.
    //
    // What does still hold, and is the property that matters, is that it cannot straighten
    // through anything: crossing an obstacle is refused move by move, so the side the
    // planner chose is the side the result keeps. That is checked over the corpus.
    const auto direct = std::hypot(80.0 - 20.0, 50.0 - 50.0);
    BOOST_CHECK_LT(path_length(band.points), direct * 1.02);

    // What is left at the default sweep budget is the corner itself. Resampling keeps
    // every vertex of the input, so there is a node sitting exactly on the apex of the
    // detour, and that node is the last thing to flatten: tension reaches it from both
    // sides and has the furthest to move it. It is a question of budget and not of where
    // the band settles, which the next check shows.
    BOOST_CHECK_LT(sharpest_turn(band.points), 7.0);

    auto patient = defaults();
    patient.sweeps = 480;
    BOOST_CHECK_LT(sharpest_turn(smooth(detour, square.rings, patient).points), 0.5);
}

BOOST_AUTO_TEST_CASE(the_anchors_do_not_move)
{
    const Blocked blocked;
    const std::vector<Point> path{{10, 50}, {39.5, 60.5}, {60.5, 60.5}, {90, 50}};

    const auto band = smooth(path, blocked.rings, defaults());

    BOOST_REQUIRE_GE(band.points.size(), 2u);
    BOOST_CHECK_CLOSE(band.points.front().x, 10.0, 1e-9);
    BOOST_CHECK_CLOSE(band.points.front().y, 50.0, 1e-9);
    BOOST_CHECK_CLOSE(band.points.back().x, 90.0, 1e-9);
    BOOST_CHECK_CLOSE(band.points.back().y, 50.0, 1e-9);
}

// A taut path running along a wall stands on geometry at every interior node, and the
// repulsion has no direction to push a node on a wall along: the distance is zero.  The
// band supplies that push from the wall's normal, so the path lifts off the wall by the
// comfort margin where it has room to, and rounds back onto the anchors at either end.
BOOST_AUTO_TEST_CASE(a_path_along_a_wall_lifts_off_it)
{
    const Square square;
    const std::vector<Point> along_the_wall{{10, 0}, {50, 0}, {90, 0}};
    auto parameters = defaults();
    parameters.comfort = 5.0;
    const auto band = smooth(along_the_wall, square.rings, parameters);
    BOOST_REQUIRE(band.certified);

    auto highest = 0.0;
    for (const auto &p : band.points)
    {
        BOOST_CHECK_GE(p.y, 0.0);
        highest = std::max(highest, p.y);
    }
    // off the wall by most of the margin somewhere along the way, and never through it
    BOOST_CHECK_GT(highest, 0.5 * parameters.comfort);
    BOOST_CHECK_LE(highest, 2.0 * parameters.comfort);
}

BOOST_AUTO_TEST_CASE(the_certificate_holds)
{
    const Blocked blocked;
    const std::vector<Point> path{{10, 50}, {39.5, 60.5}, {60.5, 60.5}, {90, 50}};

    const auto band = smooth(path, blocked.rings, defaults());
    BOOST_CHECK(certificate_holds(band, blocked.rings));
    BOOST_CHECK(band.certified);

    // And what it certifies: every interior node has room around it.
    for (std::size_t i = 1; i + 1 < band.points.size(); ++i)
    {
        BOOST_CHECK_GT(clearance(band.points[i], blocked.rings).distance, 0.0);
    }
}

BOOST_AUTO_TEST_CASE(the_kink_comes_out)
{
    const Blocked blocked;
    // Round the block's corner and away: the taut shape, and the one this work exists to
    // improve. The turn is concentrated in a single node.
    const std::vector<Point> path{{10, 61}, {39.5, 60.5}, {60.5, 60.5}, {61, 95}};

    const auto band = smooth(path, blocked.rings, defaults());

    const auto before = sharpest_turn(path);
    const auto after = sharpest_turn(band.points);
    BOOST_TEST_MESSAGE("sharpest turn " << before << " -> " << after);
    BOOST_CHECK_LT(after, before * 0.75);
}

BOOST_AUTO_TEST_CASE(it_holds_the_comfort_margin_where_there_is_room)
{
    const Blocked blocked;
    const std::vector<Point> path{{10, 50}, {39.5, 60.5}, {60.5, 60.5}, {90, 50}};

    auto parameters = defaults();
    const auto band = smooth(path, blocked.rings, parameters);

    // Past the block there is room on both sides, so the band should not be hugging it.
    // Not the full margin: at a corner the node settles at the weighted average of the
    // chord and the obstacle's target, so with tension and repulsion at equal weight it
    // is about halfway out.  What matters is that it is off the geometry by a visible
    // amount, and that weighting the repulsion more moves it further out.
    const auto tightest_of = [&](const Band &b)
    {
        auto tightest = 1e18;
        for (std::size_t i = 1; i + 1 < b.points.size(); ++i)
        {
            tightest = std::min(tightest, clearance(b.points[i], blocked.rings).distance);
        }
        return tightest;
    };
    const auto tightest = tightest_of(band);
    BOOST_TEST_MESSAGE("tightest clearance " << tightest << " against comfort "
                                             << parameters.comfort);
    BOOST_CHECK_GT(tightest, parameters.comfort * 0.5);

    parameters.repulsion = 3.0;
    const auto firmer = tightest_of(smooth(path, blocked.rings, parameters));
    BOOST_CHECK_GT(firmer, tightest);
    BOOST_CHECK_GT(firmer, parameters.comfort * 0.7);
}

BOOST_AUTO_TEST_CASE(running_it_again_changes_almost_nothing)
{
    const Blocked blocked;
    const std::vector<Point> path{{10, 50}, {39.5, 60.5}, {60.5, 60.5}, {90, 50}};

    const auto once = smooth(path, blocked.rings, defaults());
    const auto twice = smooth(once.points, blocked.rings, defaults());

    // Not point-for-point equality: the second run resamples the first one's output, so
    // the nodes are in different places. The shape is what has to be stable.
    BOOST_CHECK_CLOSE(path_length(twice.points), path_length(once.points), 2.0);
    BOOST_CHECK_LT(sharpest_turn(twice.points), sharpest_turn(once.points) + 5.0);
}

BOOST_AUTO_TEST_CASE(the_same_input_gives_the_same_output)
{
    const Blocked blocked;
    const std::vector<Point> path{{10, 50}, {39.5, 60.5}, {60.5, 60.5}, {90, 50}};

    const auto first = smooth(path, blocked.rings, defaults());
    const auto second = smooth(path, blocked.rings, defaults());

    // Bitwise, not approximately. A path that differs in the last bit between two runs
    // encodes to a different polyline and fails a test on one platform only.
    BOOST_REQUIRE_EQUAL(first.points.size(), second.points.size());
    for (std::size_t i = 0; i < first.points.size(); ++i)
    {
        BOOST_CHECK_EQUAL(first.points[i].x, second.points[i].x);
        BOOST_CHECK_EQUAL(first.points[i].y, second.points[i].y);
    }
}

BOOST_AUTO_TEST_CASE(the_band_is_longer_than_the_taut_path_but_not_much)
{
    const Blocked blocked;
    const std::vector<Point> path{{10, 50}, {39.5, 60.5}, {60.5, 60.5}, {90, 50}};

    const auto band = smooth(path, blocked.rings, defaults());

    // Repulsion buys clearance with length, so the band cannot be shorter than the taut
    // path by much; and it must not be wildly longer, which would mean it has wandered.
    const auto taut = path_length(path);
    const auto smoothed = path_length(band.points);
    BOOST_TEST_MESSAGE("taut " << taut << " smoothed " << smoothed);
    BOOST_CHECK_GT(smoothed, taut * 0.9);
    BOOST_CHECK_LT(smoothed, taut * 1.5);
}

BOOST_AUTO_TEST_CASE(degenerate_input)
{
    const Square square;

    const std::vector<Point> single{{50, 50}};
    BOOST_CHECK_EQUAL(smooth(single, square.rings, defaults()).points.size(), 1u);

    const std::vector<Point> pair{{20, 50}, {80, 50}};
    BOOST_CHECK_GE(smooth(pair, square.rings, defaults()).points.size(), 2u);

    // Two coincident points: nothing to resample and nothing to smooth.
    const std::vector<Point> same{{50, 50}, {50, 50}};
    const auto band = smooth(same, square.rings, defaults());
    BOOST_CHECK_GE(band.points.size(), 2u);
    BOOST_CHECK(certificate_holds(band, square.rings));
}

// The defect this pins: an anchor on a corner of the block, with the segment leaving it
// straight through the block.
//
// `crosses_ring` cannot see this. It asks whether the segment properly crosses a ring
// edge, and an anchor that is a vertex of the ring shares an endpoint with the two edges
// there, which the test exempts. So the segment was called clear, and because the rest of
// the certificate speaks only for the interior nodes, the whole path certified while
// running 20 units inside an obstacle.
BOOST_AUTO_TEST_CASE(an_anchor_segment_into_an_obstacle_is_not_certified)
{
    const Blocked blocked;

    // (40,40) and (60,60) are opposite corners of the block, so the straight line between
    // them is its diagonal and lies entirely inside it.
    Band diagonal;
    diagonal.points = {{40, 40}, {60, 60}};
    diagonal.radii = {0.0, 0.0};
    BOOST_CHECK(!certificate_holds(diagonal, blocked.rings));

    // Leaving the same corner into the open: certified, which is what stops the fix
    // from being "reject every anchor segment".
    Band away;
    away.points = {{40, 40}, {20, 20}};
    away.radii = {0.0, 0.0};
    BOOST_CHECK(certificate_holds(away, blocked.rings));

    // And along an edge of the block: legal ground, and certified. The free space is
    // closed, so a path may run against an obstacle -- which is exactly what a shortest
    // path does, since the way past a rectangle runs along its sides. There is no disc
    // anywhere on this segment to prove it with, so it is the geometry that answers.
    Band grazing;
    grazing.points = {{40, 40}, {60, 40}};
    grazing.radii = {0.0, 0.0};
    BOOST_CHECK(certificate_holds(grazing, blocked.rings));

    // The whole way round the block, touching it for the entire length: still legal.
    Band around;
    around.points = {{40, 40}, {60, 40}, {60, 60}, {40, 60}, {40, 40}};
    around.radii.assign(around.points.size(), 0.0);
    BOOST_CHECK(certificate_holds(around, blocked.rings));

    // Along the outer wall of the square, which is boundary too.
    Band wall;
    wall.points = {{0, 0}, {100, 0}};
    wall.radii = {0.0, 0.0};
    BOOST_CHECK(certificate_holds(wall, blocked.rings));

    // Leaving the plaza altogether is not legal, however much room is on the far side.
    Band outside;
    outside.points = {{50, 10}, {50, -10}};
    outside.radii = {0.0, 0.0};
    BOOST_CHECK(!certificate_holds(outside, blocked.rings));

    // Cutting the corner of the block: legal at both ends, through the obstacle between.
    Band corner;
    corner.points = {{50, 40}, {40, 50}};
    corner.radii = {0.0, 0.0};
    BOOST_CHECK(!certificate_holds(corner, blocked.rings));

    // And what smooth() does with an input through the block: hands it back as it came,
    // saying so.
    const std::vector<Point> path{{40, 40}, {50, 50}, {60, 60}};
    const auto band = smooth(path, blocked.rings, defaults());
    BOOST_CHECK(!band.certified);
    BOOST_CHECK_EQUAL(band.points.size(), path.size());

    // A taut path is the case this is all for: it grazes the geometry, so it has no discs
    // to speak with, and it must still certify.
    const std::vector<Point> taut{{10, 40}, {40, 40}, {60, 40}, {90, 40}};
    BOOST_CHECK(smooth(taut, blocked.rings, defaults()).certified);
}


// Legal is not the same as better. A band that stays in free space the whole time can
// still hand back something worse than it was given, and over the corpus that was the
// rule rather than the exception: every taut path the band managed to move came out
// worse. So a result has to earn its place.
//
// Length is not what it earns it on. A smoothed path is longer than the taut one it came
// from, and that is the trade the band exists to make.
BOOST_AUTO_TEST_CASE(it_returns_nothing_that_doubles_back_more)
{
    const Blocked blocked;

    // Cranked hard enough to wobble: strong repulsion against a large comfort margin on
    // geometry that is small relative to it.
    auto wild = defaults();
    wild.repulsion = 8.0;
    wild.contraction = 4.0;
    wild.sweeps = 200;

    const std::vector<std::vector<Point>> paths{
        {{10, 50}, {39.5, 60.5}, {60.5, 60.5}, {90, 50}},
        {{10, 40}, {40, 40}, {60, 40}, {90, 40}},
        {{10, 50}, {50, 65}, {90, 50}},
    };

    for (const auto &path : paths)
    {
        for (const auto &parameters : {defaults(), wild})
        {
            const auto band = smooth(path, blocked.rings, parameters);

            // Never doubling back more than the two inflections a bulge needs.
            BOOST_CHECK_LE(doubling_back(band.points), doubling_back(path) + 20.0 + 1e-9);

            // Whatever comes back is legal ground either way.
            BOOST_CHECK(certificate_holds(band, blocked.rings));
        }
    }
}


// A path that runs along a wall and turns its corners must survive resampling.
//
// The bug this pins: resampling sampled the polyline at an even stride and kept only the
// two anchors, so a corner the path turned was dropped unless a sample happened to land
// on it. The chord between the samples either side then cut straight through the
// obstacle, the certificate refused the whole band, and it was discarded. Every level of
// every wall-hugging band on the corpus was thrown away that way, which is why a taut
// path could never be smoothed however much room its nodes were given.
BOOST_AUTO_TEST_CASE(resampling_keeps_the_corners_it_turns)
{
    const Blocked blocked;

    // Up the left side of the block and along its top: legal, and it turns a right angle
    // at the corner. The two legs are 27 long against a sampling of 10, so the corner
    // falls between samples rather than on one, which is the ordinary case and the one
    // that broke.
    const std::vector<Point> around{{40, 33}, {40, 60}, {67, 60}};

    for (const auto &parameters : {defaults(), BandParameters{}})
    {
        auto p = parameters;
        p.comfort = 5.0;
        const auto band = smooth(around, blocked.rings, p);

        BOOST_CHECK(certificate_holds(band, blocked.rings));

        // And the band survived, which is the point. Before the fix it did not: the
        // resampled band cut the corner at (40,60), the certificate rightly refused it,
        // and smooth() handed the input straight back -- legal, and untouched. So the
        // symptom was not an illegal path but a band that never did anything.
        BOOST_CHECK_GT(band.points.size(), around.size());

        // And every vertex of the input is still represented, to within the distance the
        // relaxation is allowed to move it.
        for (const auto &vertex : around)
        {
            auto nearest = std::numeric_limits<double>::infinity();
            for (const auto &point : band.points)
            {
                nearest = std::min(nearest, std::hypot(point.x - vertex.x, point.y - vertex.y));
            }
            BOOST_CHECK_LT(nearest, p.comfort);
        }
    }
}

// ---- smooth_coordinates(): the band as the engine calls it -------------------------

namespace
{

//! Metres per degree at the equator, so a fixture can be laid out in metres.
constexpr double METRES_PER_DEGREE = 111194.9;

util::Coordinate at_metres(const double x, const double y)
{
    return {util::FloatLongitude{x / METRES_PER_DEGREE},
            util::FloatLatitude{y / METRES_PER_DEGREE}};
}

// The Blocked fixture above, laid out in coordinates: a 100 m square with a 20 m block
// in the middle, and the taut path from the low corner to the high one, which turns on
// the block's south-east corner.
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
    std::vector<Ring> projected_rings() const
    {
        return {Ring{projected_outer}, Ring{projected_block}};
    }
    std::vector<Point> projected_outer = projected(outer);
    std::vector<Point> projected_block = projected(block);
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

} // namespace

BOOST_AUTO_TEST_CASE(no_margin_draws_the_path_as_given)
{
    const BlockedInCoordinates plaza;
    BOOST_CHECK(!smooth_coordinates(plaza.rings, plaza.taut, 0.0));
    BOOST_CHECK(!smooth_coordinates(plaza.rings, plaza.taut, -1.0));
}

BOOST_AUTO_TEST_CASE(a_straight_line_across_the_open_is_not_redrawn)
{
    // Nothing within the margin of the line, so the band is a straight line sampled
    // every quarter margin.  That is the same drawing, and the answer says so rather
    // than hand back forty collinear points to be carried as computed.
    const BlockedInCoordinates plaza;
    const std::vector<util::Coordinate> line{at_metres(5, 5), at_metres(30, 5)};
    BOOST_CHECK(!smooth_coordinates(plaza.rings, line, 2.0));
}

BOOST_AUTO_TEST_CASE(a_corner_is_rounded_and_the_anchors_are_kept_exactly)
{
    const BlockedInCoordinates plaza;
    const auto drawn = smooth_coordinates(plaza.rings, plaza.taut, 2.0);
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
    // and longer, which is the trade
    BOOST_CHECK_GE(path_length(points), path_length(taut));
}

BOOST_AUTO_TEST_CASE(smooth_coordinates_is_deterministic)
{
    const BlockedInCoordinates plaza;
    const auto once = smooth_coordinates(plaza.rings, plaza.taut, 2.0);
    const auto twice = smooth_coordinates(plaza.rings, plaza.taut, 2.0);
    BOOST_REQUIRE(once && twice);
    BOOST_REQUIRE_EQUAL(once->size(), twice->size());
    for (std::size_t i = 0; i < once->size(); ++i)
    {
        BOOST_CHECK((*once)[i] == (*twice)[i]);
    }
}

BOOST_AUTO_TEST_SUITE_END()
