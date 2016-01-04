#include <boost/test/unit_test.hpp>

#include "util/coordinate_calculation.hpp"

#include <osrm/coordinate.hpp>

#include <cmath>

// Regression test for bug captured in #1347
BOOST_AUTO_TEST_CASE(regression_test_1347)
{
    FixedPointCoordinate u(10 * COORDINATE_PRECISION, -100 * COORDINATE_PRECISION);
    FixedPointCoordinate v(10.001 * COORDINATE_PRECISION, -100.002 * COORDINATE_PRECISION);
    FixedPointCoordinate q(10.002 * COORDINATE_PRECISION, -100.001 * COORDINATE_PRECISION);

    double d1 = coordinate_calculation::perpendicularDistance(u, v, q);

    double ratio;
    FixedPointCoordinate nearest_location;
    double d2 = coordinate_calculation::perpendicularDistance(u, v, q, nearest_location, ratio);

    BOOST_CHECK_LE(std::abs(d1 - d2), 0.01);
}
