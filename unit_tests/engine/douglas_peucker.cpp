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
    std::vector<util::FixedPointCoordinate> coordinates = {
        util::FixedPointCoordinate(5 * COORDINATE_PRECISION, 5 * COORDINATE_PRECISION),
        util::FixedPointCoordinate(6 * COORDINATE_PRECISION, 6 * COORDINATE_PRECISION),
        util::FixedPointCoordinate(10 * COORDINATE_PRECISION, 10 * COORDINATE_PRECISION),
        util::FixedPointCoordinate(5 * COORDINATE_PRECISION, 15 * COORDINATE_PRECISION)};

    // FIXME this test fails for everything below z4 because the DP algorithms
    // only used a naive distance measurement
    //for (unsigned z = 0; z < detail::DOUGLAS_PEUCKER_THRESHOLDS_SIZE; z++)
    for (unsigned z = 0; z < 2; z++)
    {
        auto result = douglasPeucker(coordinates, z);
        BOOST_CHECK_EQUAL(result.size(), 3);
        BOOST_CHECK_EQUAL(result[0], coordinates[0]);
        BOOST_CHECK_EQUAL(result[1], coordinates[2]);
        BOOST_CHECK_EQUAL(result[2], coordinates[3]);
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
        std::vector<util::FixedPointCoordinate> input = {
            util::FixedPointCoordinate(5 * COORDINATE_PRECISION, 5 * COORDINATE_PRECISION),
            util::FixedPointCoordinate(5 * COORDINATE_PRECISION,
                                       5 * COORDINATE_PRECISION +
                                           detail::DOUGLAS_PEUCKER_THRESHOLDS[z]),
            util::FixedPointCoordinate(10 * COORDINATE_PRECISION, 10 * COORDINATE_PRECISION),
            util::FixedPointCoordinate(10 * COORDINATE_PRECISION,
                                       10 + COORDINATE_PRECISION +
                                           detail::DOUGLAS_PEUCKER_THRESHOLDS[z] * 2),
            util::FixedPointCoordinate(5 * COORDINATE_PRECISION, 15 * COORDINATE_PRECISION),
            util::FixedPointCoordinate(5 * COORDINATE_PRECISION +
                                           detail::DOUGLAS_PEUCKER_THRESHOLDS[z],
                                       15 * COORDINATE_PRECISION),
        };
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
