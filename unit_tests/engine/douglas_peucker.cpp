#include "engine/douglas_peucker.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include <osrm/coordinate.hpp>

#include <vector>

BOOST_AUTO_TEST_SUITE(douglas_peucker_simplification)

using namespace osrm;
using namespace osrm::engine;

BOOST_AUTO_TEST_CASE(removed_middle_test)
{
    /*
            x
           / \
          x   \
         /     \
        x       x
    */
    std::vector<util::Coordinate> coordinates = {
        util::Coordinate(util::FloatLongitude(5), util::FloatLatitude(5)),
        util::Coordinate(util::FloatLongitude(5.995715), util::FloatLatitude(6)),
        util::Coordinate(util::FloatLongitude(10), util::FloatLatitude(10)),
        util::Coordinate(util::FloatLongitude(15), util::FloatLatitude(5))};

    for (unsigned z = 0; z < detail::DOUGLAS_PEUCKER_THRESHOLDS_SIZE; z++)
    {
        auto result = douglasPeucker(coordinates, z);
        BOOST_CHECK_EQUAL(result.size(), 3);
        BOOST_CHECK_EQUAL(result[0], coordinates[0]);
        BOOST_CHECK_EQUAL(result[1], coordinates[2]);
        BOOST_CHECK_EQUAL(result[2], coordinates[3]);
    }
}

BOOST_AUTO_TEST_CASE(removed_middle_test_zoom_sensitive)
{
    /*
            x
           / \
          x   \
         /     \
        x       x
    */
    std::vector<util::Coordinate> coordinates = {
        util::Coordinate(util::FloatLongitude(5), util::FloatLatitude(5)),
        util::Coordinate(util::FloatLongitude(6), util::FloatLatitude(6)),
        util::Coordinate(util::FloatLongitude(10), util::FloatLatitude(10)),
        util::Coordinate(util::FloatLongitude(15), util::FloatLatitude(5))};

    // Coordinate 6,6 should start getting included at Z12 and higher
    // Note that 5,5->6,6->10,10 is *not* a straight line on the surface
    // of the earth
    for (unsigned z = 0; z < 11; z++)
    {
        auto result = douglasPeucker(coordinates, z);
        BOOST_CHECK_EQUAL(result.size(), 3);
    }
    // From 12 to max zoom, we should get all coordinates
    for (unsigned z = 12; z < detail::DOUGLAS_PEUCKER_THRESHOLDS_SIZE; z++)
    {
        auto result = douglasPeucker(coordinates, z);
        BOOST_CHECK_EQUAL(result.size(), 4);
    }
}


BOOST_AUTO_TEST_CASE(remove_second_node_test)
{
    for (unsigned z = 0; z < detail::DOUGLAS_PEUCKER_THRESHOLDS_SIZE; z++)
    {
        /*
             x--x
             |   \
           x-x    x
                  |
                  x
        */
        std::vector<util::Coordinate> input = {
            util::Coordinate(util::FixedLongitude(5 * COORDINATE_PRECISION),
                             util::FixedLatitude(5 * COORDINATE_PRECISION)),
            util::Coordinate(util::FixedLongitude(5 * COORDINATE_PRECISION),
                             util::FixedLatitude(5 * COORDINATE_PRECISION +
                                                 detail::DOUGLAS_PEUCKER_THRESHOLDS[z])),
            util::Coordinate(util::FixedLongitude(10 * COORDINATE_PRECISION),
                             util::FixedLatitude(10 * COORDINATE_PRECISION)),
            util::Coordinate(util::FixedLongitude(10 * COORDINATE_PRECISION),
                             util::FixedLatitude(10 + COORDINATE_PRECISION +
                                                 detail::DOUGLAS_PEUCKER_THRESHOLDS[z] * 2)),
            util::Coordinate(util::FixedLongitude(5 * COORDINATE_PRECISION),
                             util::FixedLatitude(15 * COORDINATE_PRECISION)),
            util::Coordinate(util::FixedLongitude(5 * COORDINATE_PRECISION +
                                                  detail::DOUGLAS_PEUCKER_THRESHOLDS[z]),
                             util::FixedLatitude(15 * COORDINATE_PRECISION))};
        BOOST_TEST_MESSAGE("Threshold (" << z << "): " << detail::DOUGLAS_PEUCKER_THRESHOLDS[z]);
        auto result = douglasPeucker(input, z);
        BOOST_CHECK_EQUAL(result.size(), 4);
        BOOST_CHECK_EQUAL(input[0], result[0]);
        BOOST_CHECK_EQUAL(input[2], result[1]);
        BOOST_CHECK_EQUAL(input[3], result[2]);
        BOOST_CHECK_EQUAL(input[5], result[3]);
    }
}

BOOST_AUTO_TEST_SUITE_END()
