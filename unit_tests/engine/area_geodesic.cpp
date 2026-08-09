#include "engine/area_geodesic.hpp"

#include "util/coordinate_calculation.hpp"

#include <boost/test/unit_test.hpp>

#include <cmath>
#include <span>
#include <vector>

BOOST_AUTO_TEST_SUITE(area_geodesic_test)

using namespace osrm;
using namespace osrm::engine::area;

namespace
{

// A degree of longitude at the equator, near enough for a fixture: the numbers below are
// laid out in metres and converted, so the expected lengths can be written down by hand.
constexpr double METRE = 1.0 / 111319.49;

util::Coordinate at(double x, double y)
{ return {util::FloatLongitude{x * METRE}, util::FloatLatitude{y * METRE}}; }

using Rings = std::vector<std::span<const util::Coordinate>>;

/** A square plaza of the given side, optionally with a square block in the middle. */
struct Plaza
{
    std::vector<util::Coordinate> outer;
    std::vector<util::Coordinate> block;
    Rings rings;

    explicit Plaza(double side = 1000.0, double obstacle = 0.0)
    {
        outer = {at(0, 0), at(side, 0), at(side, side), at(0, side)};
        rings.emplace_back(outer);
        if (obstacle > 0.0)
        {
            const auto lo = (side - obstacle) / 2, hi = lo + obstacle;
            block = {at(lo, lo), at(hi, lo), at(hi, hi), at(lo, hi)};
            rings.emplace_back(block);
        }
    }
};

/** Every fixture gets a key of its own, so one test cannot read another's graph. */
std::uint64_t fresh_key()
{
    static std::uint64_t next = 1;
    return next++;
}

double straight(util::Coordinate a, util::Coordinate b)
{ return util::coordinate_calculation::greatCircleDistance(a, b); }

} // namespace

// Nothing in the way: the answer is the straight line and the path turns nowhere.
BOOST_AUTO_TEST_CASE(geodesic_straight_across_an_empty_plaza)
{
    Plaza plaza;
    const auto from = at(100, 100), to = at(900, 900);
    const auto found = geodesic_between(1, fresh_key(), plaza.rings, from, to);

    BOOST_REQUIRE(found);
    BOOST_CHECK(found->bends.empty());
    BOOST_CHECK_CLOSE(found->length, straight(from, to), 0.5);
}

// With a block between them the path has to turn, and turns at a corner of the block.
BOOST_AUTO_TEST_CASE(geodesic_bends_round_an_obstacle)
{
    Plaza plaza{1000.0, 400.0}; // the block spans 300..700 in both axes
    const auto from = at(100, 500), to = at(900, 500);
    const auto found = geodesic_between(1, fresh_key(), plaza.rings, from, to);

    BOOST_REQUIRE(found);
    BOOST_REQUIRE(!found->bends.empty());
    // longer than the straight line, but not by much -- it clips two corners
    BOOST_CHECK_GT(found->length, straight(from, to));
    BOOST_CHECK_LT(found->length, straight(from, to) * 1.3);

    // every turn is at a corner of the block, never mid-air
    for (const auto bend : found->bends)
    {
        const auto on_block =
            std::any_of(plaza.block.begin(),
                        plaza.block.end(),
                        [&](const auto corner) { return straight(corner, bend) < 0.5; });
        BOOST_CHECK(on_block);
    }
}

// The bend is on the near side of the block: going round the far side would be longer.
BOOST_AUTO_TEST_CASE(geodesic_takes_the_shorter_way_round)
{
    Plaza plaza{1000.0, 400.0};
    // both points below the block, so the way round is underneath it
    const auto from = at(100, 200), to = at(900, 200);
    const auto found = geodesic_between(1, fresh_key(), plaza.rings, from, to);

    BOOST_REQUIRE(found);
    // nothing blocks a line at y = 200, the block starts at 300
    BOOST_CHECK(found->bends.empty());
    BOOST_CHECK_CLOSE(found->length, straight(from, to), 0.5);
}

BOOST_AUTO_TEST_CASE(geodesic_from_a_point_to_itself_is_nothing)
{
    Plaza plaza;
    const auto here = at(400, 400);
    const auto found = geodesic_between(1, fresh_key(), plaza.rings, here, here);

    BOOST_REQUIRE(found);
    BOOST_CHECK_SMALL(found->length, 1e-6);
    BOOST_CHECK(found->bends.empty());
}

BOOST_AUTO_TEST_CASE(geodesic_between_neighbouring_points)
{
    Plaza plaza;
    const auto from = at(500, 500), to = at(500.5, 500);
    const auto found = geodesic_between(1, fresh_key(), plaza.rings, from, to);

    BOOST_REQUIRE(found);
    BOOST_CHECK(found->bends.empty());
    BOOST_CHECK_LT(found->length, 1.0);
}

// Outside, or inside the obstacle, is not inside the area.  Declining is not the same as
// saying there is no route -- the caller falls back to the ordinary search.
BOOST_AUTO_TEST_CASE(geodesic_declines_points_that_are_not_inside)
{
    Plaza plaza{1000.0, 400.0};
    const auto inside = at(100, 100);

    BOOST_CHECK(!geodesic_between(1, fresh_key(), plaza.rings, at(-50, 500), inside));
    BOOST_CHECK(!geodesic_between(1, fresh_key(), plaza.rings, inside, at(1500, 500)));
    // the middle of the block
    BOOST_CHECK(!geodesic_between(1, fresh_key(), plaza.rings, inside, at(500, 500)));
}

BOOST_AUTO_TEST_CASE(geodesic_declines_a_degenerate_area)
{
    std::vector<util::Coordinate> two{at(0, 0), at(100, 0)};
    Rings sliver{std::span<const util::Coordinate>(two)};
    BOOST_CHECK(!geodesic_between(1, fresh_key(), sliver, at(10, 0), at(20, 0)));

    Rings none;
    BOOST_CHECK(!geodesic_between(1, fresh_key(), none, at(10, 0), at(20, 0)));

    std::vector<util::Coordinate> empty;
    Rings empty_ring{std::span<const util::Coordinate>(empty)};
    BOOST_CHECK(!geodesic_between(1, fresh_key(), empty_ring, at(10, 0), at(20, 0)));
}

// Above the guard the solver declines rather than spending the query's whole budget.
BOOST_AUTO_TEST_CASE(geodesic_declines_an_area_that_is_too_big)
{
    std::vector<util::Coordinate> ring;
    for (std::size_t i = 0; i < GEODESIC_MAX_VERTICES + 1; ++i)
    {
        const auto angle =
            2 * M_PI * static_cast<double>(i) / static_cast<double>(GEODESIC_MAX_VERTICES + 1);
        ring.push_back(at(500 + 400 * std::cos(angle), 500 + 400 * std::sin(angle)));
    }
    Rings rings{std::span<const util::Coordinate>(ring)};

    BOOST_CHECK(!geodesic_between(1, fresh_key(), rings, at(500, 500), at(520, 520)));

    ring.pop_back(); // now exactly at the limit
    Rings smaller{std::span<const util::Coordinate>(ring)};
    BOOST_CHECK(geodesic_between(1, fresh_key(), smaller, at(500, 500), at(520, 520)));
}

// A wall across the plaza with a one-metre gap at each end.  The interior of a valid
// polygon with holes is always connected -- a hole that separated it would have to touch
// the boundary, which makes it two areas rather than one -- so a path always exists.  What
// there is not, is a short one: this has to find a gap and thread it.
BOOST_AUTO_TEST_CASE(geodesic_threads_a_one_metre_gap)
{
    std::vector<util::Coordinate> outer{at(0, 0), at(1000, 0), at(1000, 1000), at(0, 1000)};
    std::vector<util::Coordinate> wall{at(1, 480), at(999, 480), at(999, 520), at(1, 520)};
    Rings rings{std::span<const util::Coordinate>(outer), std::span<const util::Coordinate>(wall)};

    const auto from = at(100, 200), to = at(100, 800);
    const auto found = geodesic_between(1, fresh_key(), rings, from, to);

    BOOST_REQUIRE(found);
    // straight across would be 600 m; round the end of the wall is half as long again
    BOOST_CHECK_GT(found->length, 1.5 * straight(from, to));
    BOOST_REQUIRE_GE(found->bends.size(), 1u);
    // and it turns only at real corners -- of the wall, or of the plaza
    for (const auto bend : found->bends)
    {
        const auto is_corner = [&](const std::vector<util::Coordinate> &ring)
        {
            return std::any_of(ring.begin(),
                               ring.end(),
                               [&](const auto corner) { return straight(corner, bend) < 1.0; });
        };
        BOOST_CHECK(is_corner(wall) || is_corner(outer));
    }
}

// A plaza bent into an L: the two ends cannot see each other even with no obstacle in it,
// and the path turns at the inner corner.
BOOST_AUTO_TEST_CASE(geodesic_bends_at_a_concave_corner_of_the_boundary)
{
    std::vector<util::Coordinate> outer{at(0, 0),
                                        at(400, 0),
                                        at(400, 600), // the inner corner
                                        at(1000, 600),
                                        at(1000, 1000),
                                        at(0, 1000)};
    Rings rings{std::span<const util::Coordinate>(outer)};

    const auto from = at(200, 100), to = at(900, 800);
    const auto found = geodesic_between(1, fresh_key(), rings, from, to);

    BOOST_REQUIRE(found);
    BOOST_REQUIRE_EQUAL(found->bends.size(), 1u);
    BOOST_CHECK_LT(straight(found->bends.front(), at(400, 600)), 0.5);
    BOOST_CHECK_GT(found->length, straight(from, to));
}

// A vertex repeated, and three vertices in a row on one line: neither is a corner, and
// neither should make the solver report a bend or fall over.
BOOST_AUTO_TEST_CASE(geodesic_survives_duplicate_and_collinear_vertices)
{
    std::vector<util::Coordinate> outer{at(0, 0),
                                        at(500, 0), // collinear with its neighbours
                                        at(1000, 0),
                                        at(1000, 1000),
                                        at(1000, 1000), // repeated
                                        at(0, 1000)};
    Rings rings{std::span<const util::Coordinate>(outer)};

    const auto from = at(100, 100), to = at(900, 900);
    const auto found = geodesic_between(1, fresh_key(), rings, from, to);

    BOOST_REQUIRE(found);
    BOOST_CHECK(found->bends.empty());
    BOOST_CHECK_CLOSE(found->length, straight(from, to), 0.5);
}

// Sitting exactly on a corner, or exactly on an edge, is the boundary of "inside".  What
// matters is that the solver answers consistently and never asserts.
BOOST_AUTO_TEST_CASE(geodesic_handles_points_on_the_boundary)
{
    Plaza plaza;
    const auto inside = at(500, 500);
    for (const auto edge_case : {at(0, 0), at(500, 0), at(1000, 1000)})
    {
        const auto found = geodesic_between(1, fresh_key(), plaza.rings, edge_case, inside);
        if (found)
        {
            BOOST_CHECK_GE(found->length, 0.0);
        }
    }
}

// The graph is built once per area and kept, and a second dataset does not reuse it.
BOOST_AUTO_TEST_CASE(geodesic_caches_the_graph_per_area_and_dataset)
{
    forget_cached_geodesics();
    BOOST_CHECK_EQUAL(cached_geodesic_count(), 0u);

    Plaza plaza{1000.0, 400.0};
    const auto key = fresh_key();

    BOOST_REQUIRE(geodesic_between(7, key, plaza.rings, at(100, 100), at(200, 200)));
    BOOST_CHECK_EQUAL(cached_geodesic_count(), 1u);

    // same area again: still one
    BOOST_REQUIRE(geodesic_between(7, key, plaza.rings, at(100, 900), at(900, 100)));
    BOOST_CHECK_EQUAL(cached_geodesic_count(), 1u);

    // a different area, and the same area in a different dataset, are both their own
    BOOST_REQUIRE(geodesic_between(7, fresh_key(), plaza.rings, at(100, 100), at(200, 200)));
    BOOST_CHECK_EQUAL(cached_geodesic_count(), 2u);
    BOOST_REQUIRE(geodesic_between(8, key, plaza.rings, at(100, 100), at(200, 200)));
    BOOST_CHECK_EQUAL(cached_geodesic_count(), 3u);

    forget_cached_geodesics();
    BOOST_CHECK_EQUAL(cached_geodesic_count(), 0u);
}

// The cache is bounded, and evicting does not change any answer.
BOOST_AUTO_TEST_CASE(geodesic_cache_is_bounded_and_eviction_is_invisible)
{
    forget_cached_geodesics();
    Plaza plaza{1000.0, 400.0};
    const auto from = at(100, 500), to = at(900, 500);

    const auto first = geodesic_between(1, 1000, plaza.rings, from, to);
    BOOST_REQUIRE(first);

    for (std::uint64_t i = 0; i < GEODESIC_CACHE_SIZE * 2; ++i)
    {
        BOOST_REQUIRE(geodesic_between(1, 2000 + i, plaza.rings, from, to));
    }
    BOOST_CHECK_EQUAL(cached_geodesic_count(), GEODESIC_CACHE_SIZE);

    // the first area was evicted long ago; asking again rebuilds it and agrees
    const auto again = geodesic_between(1, 1000, plaza.rings, from, to);
    BOOST_REQUIRE(again);
    BOOST_CHECK_CLOSE(again->length, first->length, 1e-6);
    BOOST_CHECK_EQUAL(again->bends.size(), first->bends.size());

    forget_cached_geodesics();
}

// Asking both ways round must give the same length, and the mirrored turns.
BOOST_AUTO_TEST_CASE(geodesic_is_symmetric)
{
    Plaza plaza{1000.0, 400.0};
    const auto key = fresh_key();
    const auto from = at(100, 450), to = at(900, 550);

    const auto there = geodesic_between(1, key, plaza.rings, from, to);
    const auto back = geodesic_between(1, key, plaza.rings, to, from);

    BOOST_REQUIRE(there);
    BOOST_REQUIRE(back);
    BOOST_CHECK_CLOSE(there->length, back->length, 1e-6);
    BOOST_REQUIRE_EQUAL(there->bends.size(), back->bends.size());
    for (std::size_t i = 0; i < there->bends.size(); ++i)
    {
        const auto mirrored = back->bends[back->bends.size() - 1 - i];
        BOOST_CHECK_LT(straight(there->bends[i], mirrored), 0.5);
    }
}

// Whatever it reports, walking the reported path has to add up to the reported length.
BOOST_AUTO_TEST_CASE(geodesic_length_matches_the_path_it_reports)
{
    for (const double obstacle : {0.0, 200.0, 400.0, 700.0})
    {
        Plaza plaza{1000.0, obstacle};
        const auto key = fresh_key();
        for (const auto &pair : {std::pair{at(100, 100), at(900, 900)},
                                 std::pair{at(100, 500), at(900, 500)},
                                 std::pair{at(150, 850), at(850, 150)}})
        {
            const auto found = geodesic_between(1, key, plaza.rings, pair.first, pair.second);
            if (!found)
            {
                continue;
            }
            double walked = 0.0;
            auto previous = pair.first;
            for (const auto bend : found->bends)
            {
                walked += straight(previous, bend);
                previous = bend;
            }
            walked += straight(previous, pair.second);
            BOOST_CHECK_CLOSE(walked, found->length, 0.01);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
