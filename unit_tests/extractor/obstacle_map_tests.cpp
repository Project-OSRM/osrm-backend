#include "extractor/obstacles.hpp"
#include <boost/test/unit_test.hpp>

using namespace osrm::extractor;

BOOST_AUTO_TEST_SUITE(obstacle_map_tests)

BOOST_AUTO_TEST_CASE(unsorted_emplace_query)
{
    ObstacleMap map;
    // insert obstacles with 'to' keys out of ascending order
    map.emplace(0, 5, Obstacle(Obstacle::Type::Barrier));
    map.emplace(0, 3, Obstacle(Obstacle::Type::Gate));
    map.emplace(0, 4, Obstacle(Obstacle::Type::TrafficSignals));

    // Ensure internal storage is sorted before querying
    map.sort();

    const auto r3 = map.get(3);
    BOOST_REQUIRE_EQUAL(r3.size(), 1);
    BOOST_CHECK(r3[0].type == Obstacle::Type::Gate);

    const auto r4 = map.get(4);
    BOOST_REQUIRE_EQUAL(r4.size(), 1);
    BOOST_CHECK(r4[0].type == Obstacle::Type::TrafficSignals);

    const auto r5 = map.get(5);
    BOOST_REQUIRE_EQUAL(r5.size(), 1);
    BOOST_CHECK(r5[0].type == Obstacle::Type::Barrier);
}

BOOST_AUTO_TEST_CASE(any_obstacle_unsorted_insert)
{
    ObstacleMap map;
    map.emplace(0, 2, Obstacle(Obstacle::Type::Stop));
    map.emplace(0, 1, Obstacle(Obstacle::Type::Stop));

    // sort internal storage before querying
    map.sort();

    BOOST_CHECK(map.any(2));
    BOOST_CHECK(map.any(1));
    BOOST_CHECK(!map.any(3));
}

BOOST_AUTO_TEST_SUITE_END()
