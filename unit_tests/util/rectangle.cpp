#include "util/rectangle.hpp"
#include "util/typedefs.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

BOOST_AUTO_TEST_SUITE(rectangle_test)

using namespace osrm;
using namespace osrm::util;

// Verify that the bearing-bounds checking function behaves as expected
BOOST_AUTO_TEST_CASE(get_min_dist_test)
{
    //           ^
    //           |
    //           +- 80
    //           |
    //           |
    //           +- 10
    //           |
    //--|-----|--+--|-----|-->
    // -100  -10 |  10    100
    //           +- -10
    //           |
    //           |
    //           +- -80
    //           |
    RectangleInt2D nw(10 * COORDINATE_PRECISION, 80 * COORDINATE_PRECISION,
                      10 * COORDINATE_PRECISION, 100 * COORDINATE_PRECISION);
    RectangleInt2D ne(10 * COORDINATE_PRECISION, 80 * COORDINATE_PRECISION,
                      -100 * COORDINATE_PRECISION, -10 * COORDINATE_PRECISION);
    RectangleInt2D sw(-80 * COORDINATE_PRECISION, -10 * COORDINATE_PRECISION,
                      10 * COORDINATE_PRECISION, 100 * COORDINATE_PRECISION);
    RectangleInt2D se(-80 * COORDINATE_PRECISION, -10 * COORDINATE_PRECISION,
                      -100 * COORDINATE_PRECISION, -10 * COORDINATE_PRECISION);

    FixedPointCoordinate nw_sw(9.9 * COORDINATE_PRECISION, 9.9 * COORDINATE_PRECISION);
    FixedPointCoordinate nw_se(9.9 * COORDINATE_PRECISION, 100.1 * COORDINATE_PRECISION);
    FixedPointCoordinate nw_ne(80.1 * COORDINATE_PRECISION, 100.1 * COORDINATE_PRECISION);
    FixedPointCoordinate nw_nw(80.1 * COORDINATE_PRECISION, 9.9 * COORDINATE_PRECISION);
    FixedPointCoordinate nw_s(9.9 * COORDINATE_PRECISION, 55 * COORDINATE_PRECISION);
    FixedPointCoordinate nw_e(45.0 * COORDINATE_PRECISION, 100.1 * COORDINATE_PRECISION);
    FixedPointCoordinate nw_w(45.0 * COORDINATE_PRECISION, 9.9 * COORDINATE_PRECISION);
    FixedPointCoordinate nw_n(80.1 * COORDINATE_PRECISION, 55 * COORDINATE_PRECISION);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_sw), 15611.9, 0.1);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_se), 15611.9, 0.1);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_ne), 11287.4, 0.1);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_nw), 11287.4, 0.1);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_s), 11122.6, 0.1);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_e), 7864.89, 0.1);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_w), 7864.89, 0.1);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_n), 11122.6, 0.1);

    FixedPointCoordinate se_ne(-9.9 * COORDINATE_PRECISION, -9.9 * COORDINATE_PRECISION);
    FixedPointCoordinate se_nw(-9.9 * COORDINATE_PRECISION, -100.1 * COORDINATE_PRECISION);
    FixedPointCoordinate se_sw(-80.1 * COORDINATE_PRECISION, -100.1 * COORDINATE_PRECISION);
    FixedPointCoordinate se_se(-80.1 * COORDINATE_PRECISION, -9.9 * COORDINATE_PRECISION);
    FixedPointCoordinate se_n(-9.9 * COORDINATE_PRECISION, -55 * COORDINATE_PRECISION);
    FixedPointCoordinate se_w(-45.0 * COORDINATE_PRECISION, -100.1 * COORDINATE_PRECISION);
    FixedPointCoordinate se_e(-45.0 * COORDINATE_PRECISION, -9.9 * COORDINATE_PRECISION);
    FixedPointCoordinate se_s(-80.1 * COORDINATE_PRECISION, -55 * COORDINATE_PRECISION);
    BOOST_CHECK_CLOSE(se.GetMinDist(se_sw), 11287.4, 0.1);
    BOOST_CHECK_CLOSE(se.GetMinDist(se_se), 11287.4, 0.1);
    BOOST_CHECK_CLOSE(se.GetMinDist(se_ne), 15611.9, 0.1);
    BOOST_CHECK_CLOSE(se.GetMinDist(se_nw), 15611.9, 0.1);
    BOOST_CHECK_CLOSE(se.GetMinDist(se_s), 11122.6, 0.1);
    BOOST_CHECK_CLOSE(se.GetMinDist(se_e), 7864.89, 0.1);
    BOOST_CHECK_CLOSE(se.GetMinDist(se_w), 7864.89, 0.1);
    BOOST_CHECK_CLOSE(se.GetMinDist(se_n), 11122.6, 0.1);
}

BOOST_AUTO_TEST_SUITE_END()
