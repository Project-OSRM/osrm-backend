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
    RectangleInt2D nw{
        FloatLongitude(10), FloatLongitude(100), FloatLatitude(10), FloatLatitude(80)};
    // RectangleInt2D ne {FloatLongitude(-100), FloatLongitude(-10), FloatLatitude(10),
    // FloatLatitude(80)};
    // RectangleInt2D sw {FloatLongitude(10), FloatLongitude(100), FloatLatitude(-80),
    // FloatLatitude(-10)};
    RectangleInt2D se{
        FloatLongitude(-100), FloatLongitude(-10), FloatLatitude(-80), FloatLatitude(-10)};

    Coordinate nw_sw{FloatLongitude(9.9), FloatLatitude(9.9)};
    Coordinate nw_se{FloatLongitude(100.1), FloatLatitude(9.9)};
    Coordinate nw_ne{FloatLongitude(100.1), FloatLatitude(80.1)};
    Coordinate nw_nw{FloatLongitude(9.9), FloatLatitude(80.1)};
    Coordinate nw_s{FloatLongitude(55), FloatLatitude(9.9)};
    Coordinate nw_e{FloatLongitude(100.1), FloatLatitude(45.0)};
    Coordinate nw_w{FloatLongitude(9.9), FloatLatitude(45.0)};
    Coordinate nw_n{FloatLongitude(55), FloatLatitude(80.1)};
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_sw), 15611.9, 0.1);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_se), 15611.9, 0.1);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_ne), 11287.4, 0.1);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_nw), 11287.4, 0.1);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_s), 11122.6, 0.1);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_e), 7864.89, 0.1);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_w), 7864.89, 0.1);
    BOOST_CHECK_CLOSE(nw.GetMinDist(nw_n), 11122.6, 0.1);

    Coordinate se_ne{FloatLongitude(-9.9), FloatLatitude(-9.9)};
    Coordinate se_nw{FloatLongitude(-100.1), FloatLatitude(-9.9)};
    Coordinate se_sw{FloatLongitude(-100.1), FloatLatitude(-80.1)};
    Coordinate se_se{FloatLongitude(-9.9), FloatLatitude(-80.1)};
    Coordinate se_n{FloatLongitude(-55), FloatLatitude(-9.9)};
    Coordinate se_w{FloatLongitude(-100.1), FloatLatitude(-45.0)};
    Coordinate se_e{FloatLongitude(-9.9), FloatLatitude(-45.0)};
    Coordinate se_s{FloatLongitude(-55), FloatLatitude(-80.1)};
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
