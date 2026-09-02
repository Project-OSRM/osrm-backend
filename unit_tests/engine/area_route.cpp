#include "engine/area_route.hpp"

#include "mocks/mock_area_facade.hpp"
#include "engine/area_geodesic.hpp"

#include "util/coordinate_calculation.hpp"

#include <boost/test/unit_test.hpp>

#include <vector>

BOOST_AUTO_TEST_SUITE(area_route_test)

using namespace osrm;
using namespace osrm::engine;
using namespace osrm::engine::area;
using osrm::test::at;
using osrm::test::mock_square;
using osrm::test::mock_triangle;
using osrm::test::MockAreaFacade;

namespace
{

// Both call sites go through the same helper, and a table is the one that can be driven
// with plain vectors, so it is what these exercise.

constexpr EdgeDuration UNTOUCHED{99999};

/** A one-cell table between two coordinates, returning what the cell became. */
EdgeDuration
cell_between(const MockAreaFacade &facade, const util::Coordinate from, const util::Coordinate to)
{
    // the cache is keyed on (checksum, area offset), and every fixture here has checksum
    // zero, so one test could otherwise be answered from another's graph
    forget_cached_geodesics();

    const std::vector<util::Coordinate> coordinates{from, to};
    const std::vector<std::size_t> sources{0}, destinations{1};
    std::vector<EdgeDuration> durations{UNTOUCHED};
    std::vector<EdgeDistance> distances;

    useGeodesicInTable(facade, coordinates, sources, destinations, durations, distances);
    return durations.front();
}

/** What the cell should hold: the straight line at the area's walking speed. */
EdgeDuration straight_line(const util::Coordinate from, const util::Coordinate to, double speed)
{
    const auto metres = util::coordinate_calculation::greatCircleDistance(from, to);
    return to_alias<EdgeDuration>(std::lround(metres / speed * 10.));
}

} // namespace

// The ordinary case, so the fixture is known to be capable of answering at all.
BOOST_AUTO_TEST_CASE(rewrites_a_cell_with_both_ends_on_one_plaza)
{
    MockAreaFacade facade;
    facade.areas.push_back(mock_square(0, 1000, 0));

    const auto from = at(300, 300), to = at(700, 700);
    BOOST_CHECK_EQUAL(cell_between(facade, from, to), straight_line(from, to, 1.4));
}

// Two coordinates with no area between them are left to the search.
BOOST_AUTO_TEST_CASE(leaves_a_cell_that_touches_no_area)
{
    MockAreaFacade facade;
    facade.areas.push_back(mock_square(0, 1000, 0));

    BOOST_CHECK_EQUAL(cell_between(facade, at(2000, 2000), at(2100, 2100)), UNTOUCHED);
}

/**
 * The regression this file was written for.
 *
 * The r-tree answers with bounding boxes, so a coordinate is filed under areas it is not
 * inside; geodesic_between settles containment properly and declines. Taking only the
 * first shared area threw the journey away whenever that one was a near miss, and the leg
 * stayed as the search left it: out to a corner of the plaza and back.
 *
 * The triangle sorts first here, shares its bounding box with the square, and holds
 * neither coordinate.
 */
BOOST_AUTO_TEST_CASE(looks_past_an_area_that_only_the_bounding_box_claimed)
{
    MockAreaFacade facade;
    facade.areas.push_back(mock_triangle(0, 1000, 0)); // bounding box only
    facade.areas.push_back(mock_square(200, 800, 3));  // genuinely holds both

    // x + y > 1000, so both are outside the triangle and inside the square
    const auto from = at(600, 600), to = at(700, 700);
    BOOST_CHECK_EQUAL(cell_between(facade, from, to), straight_line(from, to, 1.4));
}

// And when the only candidate really is a near miss, there is nothing to rewrite.
BOOST_AUTO_TEST_CASE(leaves_a_cell_whose_only_area_does_not_hold_it)
{
    MockAreaFacade facade;
    facade.areas.push_back(mock_triangle(0, 1000, 0));

    BOOST_CHECK_EQUAL(cell_between(facade, at(600, 600), at(700, 700)), UNTOUCHED);
}

BOOST_AUTO_TEST_SUITE_END()
