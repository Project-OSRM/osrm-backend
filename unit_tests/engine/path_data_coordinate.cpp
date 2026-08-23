#include "engine/internal_route_result.hpp"

#include "engine/guidance/assemble_geometry.hpp"
#include "engine/guidance/assemble_leg.hpp"

#include "mocks/mock_datafacade.hpp"

#include <boost/test/unit_test.hpp>

#include <vector>

BOOST_AUTO_TEST_SUITE(path_data_coordinate)

using namespace osrm;
using namespace osrm::engine;
using namespace osrm::util;

namespace
{

// The mock puts every node at the origin, which is what makes this testable: a location
// away from the origin can only have come from the field.
const Coordinate ORIGIN{FixedLongitude{0}, FixedLatitude{0}};
const Coordinate ELSEWHERE{FixedLongitude{1000}, FixedLatitude{2000}};

/**
 * The shared mock hands back an empty datasource range, which assembly indexes into.  It
 * is enough for everything else in here, so this adds the one thing that is missing
 * rather than growing the mock for one caller.
 */
struct Facade final : test::MockBaseDataFacade
{
    using Base = test::MockBaseDataFacade;

    Base::DatasourceForwardRange GetUncompressedForwardDatasources(const EdgeID) const override
    {
        static const DatasourceID datasources[] = {0, 0, 0, 0};
        return {datasources, 4};
    }
};

PathData at_node(const NodeID node)
{
    PathData point{};
    point.turn_via_node = node;
    return point;
}

PhantomNode endpoint(const Coordinate location)
{
    PhantomNode node;
    node.location = location;
    node.forward_segment_id = {0, true};
    node.reverse_segment_id = {0, true};
    node.forward_weight = {0};
    node.reverse_weight = {0};
    node.forward_duration = {0};
    node.reverse_duration = {0};
    node.fwd_segment_position = 0;
    return node;
}

} // namespace

// Nothing sets the field, so nothing changes: the position still comes from the node.
BOOST_AUTO_TEST_CASE(a_point_without_a_coordinate_is_placed_by_its_node)
{
    const Facade facade;
    const std::vector<PathData> leg{at_node(1), at_node(2)};

    BOOST_CHECK(!leg.front().coordinate.IsValid());
    BOOST_CHECK_EQUAL(coordinateOf(facade, leg.front()).lon, ORIGIN.lon);
    BOOST_CHECK_EQUAL(coordinateOf(facade, leg.front()).lat, ORIGIN.lat);
}

// And when it is set, it wins.
BOOST_AUTO_TEST_CASE(a_coordinate_overrides_the_node_lookup)
{
    const Facade facade;
    auto point = at_node(1);
    point.coordinate = ELSEWHERE;

    BOOST_CHECK_EQUAL(coordinateOf(facade, point).lon, ELSEWHERE.lon);
    BOOST_CHECK_EQUAL(coordinateOf(facade, point).lat, ELSEWHERE.lat);
}

// The drawn line follows the field.
//
// This is the property the field exists for.  A point that is computed rather than
// traversed has no node to be looked up, and without this it would be drawn at whatever
// the lookup happened to return.
BOOST_AUTO_TEST_CASE(the_geometry_follows_the_coordinate)
{
    const Facade facade;
    const auto source = endpoint(ORIGIN);
    const auto target = endpoint(ORIGIN);

    std::vector<PathData> leg{at_node(1)};
    const auto plain =
        engine::guidance::assembleGeometry(facade, leg, source, target, false, false);

    leg.front().coordinate = ELSEWHERE;
    const auto moved =
        engine::guidance::assembleGeometry(facade, leg, source, target, false, false);

    // Same shape, one point in a different place.
    BOOST_REQUIRE_EQUAL(plain.locations.size(), moved.locations.size());
    BOOST_CHECK_EQUAL(plain.locations[1].lon, ORIGIN.lon);
    BOOST_CHECK_EQUAL(moved.locations[1].lon, ELSEWHERE.lon);
    BOOST_CHECK_EQUAL(moved.locations[1].lat, ELSEWHERE.lat);

    // And the node id is untouched, because the field does not claim to answer that.
    BOOST_CHECK(plain.node_ids == moved.node_ids);
}

// The reported distance follows it too, which is the whole reason the field lives on
// PathData rather than being applied to the geometry after assembly.  A line that is
// drawn one way and measured another is worse than either.
BOOST_AUTO_TEST_CASE(the_distance_follows_the_coordinate)
{
    const Facade facade;
    const auto source = endpoint(ORIGIN);
    const auto target = endpoint(ORIGIN);

    std::vector<PathData> leg{at_node(1)};
    const auto plain = engine::guidance::assembleLeg(facade, leg, source, target, false);

    leg.front().coordinate = ELSEWHERE;
    const auto moved = engine::guidance::assembleLeg(facade, leg, source, target, false);

    // Everything is at the origin in the first case, so the leg has no length at all.
    BOOST_CHECK_SMALL(plain.distance, 1e-9);
    BOOST_CHECK_GT(moved.distance, 0.0);

    // Out to the moved point and back, so the geometry above and this agree.  The leg
    // reports its distance rounded to a tenth of a metre, which is the tolerance here.
    const auto out = coordinate_calculation::greatCircleDistance(ORIGIN, ELSEWHERE);
    BOOST_CHECK_SMALL(moved.distance - 2.0 * out, 0.1);
}

BOOST_AUTO_TEST_SUITE_END()
