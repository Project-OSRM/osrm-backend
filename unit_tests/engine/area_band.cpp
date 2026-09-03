#include "engine/area_band.hpp"

#include "engine/area_clearance.hpp"

#include <boost/test/unit_test.hpp>

#include <cmath>
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

BandParameters defaults()
{
    BandParameters p;
    p.comfort = 5.0;
    p.contraction = 1.0;
    p.repulsion = 1.0;
    p.step = 0.3;
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

BOOST_AUTO_TEST_CASE(the_soft_floor_vanishes_only_at_contact)
{
    constexpr double DELTA = 1.0;

    // The property the whole two-tier scheme rests on: zero only where the clearance is
    // zero, so its zero set is the obstacle set and no passage is closed by it.
    BOOST_CHECK_SMALL(soft_floor(0.0, DELTA), 1e-12);
    BOOST_CHECK_GT(soft_floor(0.001, DELTA), 0.0);
    BOOST_CHECK_GT(soft_floor(0.5, DELTA), 0.0);

    // Strictly increasing, so it never reorders two clearances.
    auto previous = 0.0;
    for (double rho = 0.0; rho < 10.0; rho += 0.05)
    {
        const auto value = soft_floor(rho, DELTA);
        BOOST_CHECK_GE(value, previous);
        previous = value;
    }

    // Converges to rho - delta once there is room to spare, which is the full comfort
    // margin applying in the open.
    BOOST_CHECK_CLOSE(soft_floor(10.0, DELTA), 10.0 - DELTA, 0.1);

    // And never exceeds the true clearance, which is what keeps the certificate honest.
    for (double rho = 0.0; rho < 10.0; rho += 0.1)
    {
        BOOST_CHECK_LE(soft_floor(rho, DELTA), rho);
    }

    // A comfort margin of zero leaves the clearance alone.
    BOOST_CHECK_CLOSE(soft_floor(3.0, 0.0), 3.0, 1e-9);
}

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
    BOOST_CHECK_LT(sharpest_turn(band.points), 5.0);
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

BOOST_AUTO_TEST_CASE(the_certificate_holds)
{
    const Blocked blocked;
    const std::vector<Point> path{{10, 50}, {39.5, 60.5}, {60.5, 60.5}, {90, 50}};

    const auto band = smooth(path, blocked.rings, defaults());
    BOOST_CHECK(certificate_holds(band));

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
    // Not the full margin: tension is pulling the other way and the equilibrium is
    // between them. What matters is that it is off the geometry by a visible amount.
    auto tightest = 1e18;
    for (std::size_t i = 1; i + 1 < band.points.size(); ++i)
    {
        tightest = std::min(tightest, clearance(band.points[i], blocked.rings).distance);
    }
    BOOST_TEST_MESSAGE("tightest clearance " << tightest << " against comfort "
                                             << parameters.comfort);
    BOOST_CHECK_GT(tightest, parameters.comfort * 0.7);
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
    BOOST_CHECK(certificate_holds(band));
}

BOOST_AUTO_TEST_SUITE_END()
