#include <boost/test/unit_test.hpp>

#include "util/coordinate_calculation.hpp"

#include <osrm/coordinate.hpp>

#include <cmath>

using namespace osrm;
using namespace osrm::util;

// Regression test for bug captured in #1347
BOOST_AUTO_TEST_CASE(regression_test_1347)
{
    Coordinate u(FloatLongitude(-100), FloatLatitude(10));
    Coordinate v(FloatLongitude(-100.002), FloatLatitude(10.001));
    Coordinate q(FloatLongitude(-100.001), FloatLatitude(10.002));

    double d1 = coordinate_calculation::perpendicularDistance(u, v, q);

    double ratio;
    Coordinate nearest_location;
    double d2 = coordinate_calculation::perpendicularDistance(u, v, q, nearest_location, ratio);

    BOOST_CHECK_LE(std::abs(d1 - d2), 0.01);
}
