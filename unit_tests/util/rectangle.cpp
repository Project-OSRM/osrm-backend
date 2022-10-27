#include "util/rectangle.hpp"
#include "util/typedefs.hpp"

#include <boost/test/unit_test.hpp>

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
    RectangleInt2D ne{
        FloatLongitude{10}, FloatLongitude{100}, FloatLatitude{10}, FloatLatitude{80}};
    RectangleInt2D nw{
        FloatLongitude{-100}, FloatLongitude{-10}, FloatLatitude{10}, FloatLatitude{80}};
    RectangleInt2D se{
        FloatLongitude{10}, FloatLongitude{100}, FloatLatitude{-80}, FloatLatitude{-10}};
    RectangleInt2D sw{
        FloatLongitude{-100}, FloatLongitude{-10}, FloatLatitude{-80}, FloatLatitude{-10}};

    Coordinate nw_sw{FloatLongitude{-100.1}, FloatLatitude{9.9}};
    Coordinate nw_se{FloatLongitude{-9.9}, FloatLatitude{9.9}};
    Coordinate nw_ne{FloatLongitude{-9.9}, FloatLatitude{80.1}};
    Coordinate nw_nw{FloatLongitude{-100.1}, FloatLatitude{80.1}};
    Coordinate nw_s{FloatLongitude{-55}, FloatLatitude{9.9}};
    Coordinate nw_e{FloatLongitude{-9.9}, FloatLatitude{45.0}};
    Coordinate nw_w{FloatLongitude{-100.1}, FloatLatitude{45.0}};
    Coordinate nw_n{FloatLongitude{-55}, FloatLatitude{80.1}};
    BOOST_CHECK_CLOSE(
        nw.GetMinSquaredDist(nw_sw), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        nw.GetMinSquaredDist(nw_se), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        nw.GetMinSquaredDist(nw_ne), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        nw.GetMinSquaredDist(nw_nw), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        nw.GetMinSquaredDist(nw_s), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        nw.GetMinSquaredDist(nw_e), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        nw.GetMinSquaredDist(nw_w), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        nw.GetMinSquaredDist(nw_n), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);

    Coordinate ne_sw{FloatLongitude{9.9}, FloatLatitude{9.9}};
    Coordinate ne_se{FloatLongitude{100.1}, FloatLatitude{9.9}};
    Coordinate ne_ne{FloatLongitude{100.1}, FloatLatitude{80.1}};
    Coordinate ne_nw{FloatLongitude{9.9}, FloatLatitude{80.1}};
    Coordinate ne_s{FloatLongitude{55}, FloatLatitude{9.9}};
    Coordinate ne_e{FloatLongitude{100.1}, FloatLatitude{45.0}};
    Coordinate ne_w{FloatLongitude{9.9}, FloatLatitude{45.0}};
    Coordinate ne_n{FloatLongitude{55}, FloatLatitude{80.1}};
    BOOST_CHECK_CLOSE(
        ne.GetMinSquaredDist(ne_sw), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        ne.GetMinSquaredDist(ne_se), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        ne.GetMinSquaredDist(ne_ne), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        ne.GetMinSquaredDist(ne_nw), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        ne.GetMinSquaredDist(ne_s), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        ne.GetMinSquaredDist(ne_e), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        ne.GetMinSquaredDist(ne_w), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        ne.GetMinSquaredDist(ne_n), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);

    Coordinate se_ne{FloatLongitude{100.1}, FloatLatitude{-9.9}};
    Coordinate se_nw{FloatLongitude{9.9}, FloatLatitude{-9.9}};
    Coordinate se_sw{FloatLongitude{9.9}, FloatLatitude{-80.1}};
    Coordinate se_se{FloatLongitude{100.1}, FloatLatitude{-80.1}};
    Coordinate se_n{FloatLongitude{55}, FloatLatitude{-9.9}};
    Coordinate se_w{FloatLongitude{9.9}, FloatLatitude{-45.0}};
    Coordinate se_e{FloatLongitude{100.1}, FloatLatitude{-45.0}};
    Coordinate se_s{FloatLongitude{55}, FloatLatitude{-80.1}};
    BOOST_CHECK_CLOSE(
        se.GetMinSquaredDist(se_sw), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        se.GetMinSquaredDist(se_se), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        se.GetMinSquaredDist(se_ne), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        se.GetMinSquaredDist(se_nw), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        se.GetMinSquaredDist(se_s), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        se.GetMinSquaredDist(se_e), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        se.GetMinSquaredDist(se_w), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        se.GetMinSquaredDist(se_n), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);

    Coordinate sw_ne{FloatLongitude{-9.9}, FloatLatitude{-9.9}};
    Coordinate sw_nw{FloatLongitude{-100.1}, FloatLatitude{-9.9}};
    Coordinate sw_sw{FloatLongitude{-100.1}, FloatLatitude{-80.1}};
    Coordinate sw_se{FloatLongitude{-9.9}, FloatLatitude{-80.1}};
    Coordinate sw_n{FloatLongitude{-55}, FloatLatitude{-9.9}};
    Coordinate sw_w{FloatLongitude{-100.1}, FloatLatitude{-45.0}};
    Coordinate sw_e{FloatLongitude{-9.9}, FloatLatitude{-45.0}};
    Coordinate sw_s{FloatLongitude{-55}, FloatLatitude{-80.1}};
    BOOST_CHECK_CLOSE(
        sw.GetMinSquaredDist(sw_sw), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        sw.GetMinSquaredDist(sw_se), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        sw.GetMinSquaredDist(sw_ne), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        sw.GetMinSquaredDist(sw_nw), 0.02 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        sw.GetMinSquaredDist(sw_s), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        sw.GetMinSquaredDist(sw_e), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        sw.GetMinSquaredDist(sw_w), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
    BOOST_CHECK_CLOSE(
        sw.GetMinSquaredDist(sw_n), 0.01 * COORDINATE_PRECISION * COORDINATE_PRECISION, 0.1);
}

BOOST_AUTO_TEST_SUITE_END()
