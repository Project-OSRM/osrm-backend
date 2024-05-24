#include "util/viewport.hpp"

using namespace osrm::util;

#include <boost/test/unit_test.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(viewport_test)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(zoom_level_test)
{
    BOOST_CHECK_EQUAL(
        viewport::getFittedZoom(
            Coordinate(FloatLongitude{5.668343999999995}, FloatLatitude{45.111511000000014}),
            Coordinate(FloatLongitude{5.852471999999996}, FloatLatitude{45.26800200000002})),
        12);
}

BOOST_AUTO_TEST_SUITE_END()
