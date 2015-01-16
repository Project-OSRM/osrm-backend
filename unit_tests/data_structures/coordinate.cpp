#include <osrm/coordinate.hpp>

#include <boost/test/unit_test.hpp>

// Regression test for bug captured in #1347
BOOST_AUTO_TEST_CASE(regression_test_1347)
{
    FixedPointCoordinate u(10 * COORDINATE_PRECISION, -100 * COORDINATE_PRECISION);
    FixedPointCoordinate v(10.001 * COORDINATE_PRECISION, -100.002 * COORDINATE_PRECISION);
    FixedPointCoordinate q(10.002 * COORDINATE_PRECISION, -100.001 * COORDINATE_PRECISION);

    float d1 = FixedPointCoordinate::ComputePerpendicularDistance(u, v, q);

    float ratio;
    FixedPointCoordinate nearest_location;
    float d2 = FixedPointCoordinate::ComputePerpendicularDistance(u, v, q, nearest_location, ratio);

    BOOST_CHECK_LE(std::abs(d1 - d2), 0.01f);
}

