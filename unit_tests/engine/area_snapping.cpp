#include "engine/area_snapping.hpp"

#include "mocks/mock_area_facade.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(area_snapping_test)

using namespace osrm;
using namespace osrm::engine;
using namespace osrm::engine::area;
using osrm::test::at;
using osrm::test::mock_square;
using osrm::test::MockAreaFacade;

namespace
{
std::optional<PhantomNodeCandidates> departing_from(const MockAreaFacade &facade,
                                                    const util::Coordinate coordinate)
{ return SnapInsideOpenArea(facade, coordinate, Approach::UNRESTRICTED, ApproachRole::Departure); }
} // namespace

// The ordinary case, so the fixture is known to be capable of answering at all.
BOOST_AUTO_TEST_CASE(snaps_to_the_vertices_of_a_meshed_area)
{
    MockAreaFacade facade;
    facade.areas.push_back(mock_square(0, 1000, 0));

    const auto candidates = departing_from(facade, at(500, 500));

    BOOST_REQUIRE(candidates);
    // every corner of an empty square is visible from the middle
    BOOST_CHECK_EQUAL(candidates->size(), 4u);
}

// A coordinate outside every area is not this function's business.
BOOST_AUTO_TEST_CASE(declines_a_coordinate_that_is_in_no_area)
{
    MockAreaFacade facade;
    facade.areas.push_back(mock_square(0, 1000, 0));

    BOOST_CHECK(!departing_from(facade, at(2000, 2000)));
}

/**
 * The regression this file was written for.
 *
 * Areas overlap, and one holding the coordinate can have nothing on it: the extractor
 * records a polygon before it applies AreaMesher::max_vertices, so a plaza too large to
 * mesh is stored with no way on any of its vertices.  Snapping returned on the first area
 * that held the coordinate, so an unmeshed one sorting first swallowed the request and
 * the route fell back to ordinary snapping -- onto the perimeter, out by a corner and
 * back again.
 *
 * The unmeshed area sorts first here because snapping orders by `vertices_offset`.
 */
BOOST_AUTO_TEST_CASE(looks_past_an_area_that_has_nothing_to_offer)
{
    MockAreaFacade facade;
    facade.areas.push_back(mock_square(0, 1000, 0, false)); // recorded, never meshed
    facade.areas.push_back(mock_square(200, 800, 4, true)); // the plaza with the ways on it

    const auto candidates = departing_from(facade, at(500, 500));

    BOOST_REQUIRE(candidates);
    BOOST_CHECK_EQUAL(candidates->size(), 4u);
}

// And when none of them has anything, there is genuinely nothing to offer.
BOOST_AUTO_TEST_CASE(declines_when_no_area_has_a_reachable_vertex)
{
    MockAreaFacade facade;
    facade.areas.push_back(mock_square(0, 1000, 0, false));
    facade.areas.push_back(mock_square(200, 800, 4, false));

    BOOST_CHECK(!departing_from(facade, at(500, 500)));
}

BOOST_AUTO_TEST_SUITE_END()
