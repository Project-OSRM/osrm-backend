#include <boost/test/unit_test.hpp>

#include "util/coordinate_calculation.hpp"

#include <osrm/coordinate.hpp>

#include <cmath>

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_SUITE(coordinate_calculation_tests)

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

BOOST_AUTO_TEST_CASE(regression_point_on_segment)
{
    //  ^
    //  |               t
    //  |
    //  |                 i
    //  |
    //  |---|---|---|---|---|---|---|--->
    //  |
    //  |
    //  |
    //  |
    //  |
    //  |
    //  |
    //  |
    //  |                           s
    FloatCoordinate input{FloatLongitude{55.995715}, FloatLatitude{48.332711}};
    FloatCoordinate start{FloatLongitude{74.140427}, FloatLatitude{-180}};
    FloatCoordinate target{FloatLongitude{53.041084}, FloatLatitude{77.21011}};

    FloatCoordinate nearest;
    double ratio;
    std::tie(ratio, nearest) = coordinate_calculation::projectPointOnSegment(start, target, input);

    FloatCoordinate diff{target.lon - start.lon, target.lat - start.lat};

    BOOST_CHECK_CLOSE(static_cast<double>(start.lon + FloatLongitude(ratio) * diff.lon), static_cast<double>(nearest.lon), 0.1);
    BOOST_CHECK_CLOSE(static_cast<double>(start.lat + FloatLatitude(ratio) * diff.lat), static_cast<double>(nearest.lat), 0.1);
}

BOOST_AUTO_TEST_CASE(point_on_segment)
{
    //  t
    //  |
    //  |---- i
    //  |
    //  s
    auto result_1 = coordinate_calculation::projectPointOnSegment(
        {FloatLongitude{0}, FloatLatitude{0}}, {FloatLongitude{0}, FloatLatitude{2}},
        {FloatLongitude{2}, FloatLatitude{1}});
    auto reference_ratio_1 = 0.5;
    auto reference_point_1 = FloatCoordinate{FloatLongitude{0}, FloatLatitude{1}};
    BOOST_CHECK_EQUAL(result_1.first, reference_ratio_1);
    BOOST_CHECK_EQUAL(result_1.second.lon, reference_point_1.lon);
    BOOST_CHECK_EQUAL(result_1.second.lat, reference_point_1.lat);

    //  i
    //  :
    //  t
    //  |
    //  |
    //  |
    //  s
    auto result_2 = coordinate_calculation::projectPointOnSegment(
        {FloatLongitude{0.}, FloatLatitude{0.}}, {FloatLongitude{0}, FloatLatitude{2}},
        {FloatLongitude{0}, FloatLatitude{3}});
    auto reference_ratio_2 = 1.;
    auto reference_point_2 = FloatCoordinate{FloatLongitude{0}, FloatLatitude{2}};
    BOOST_CHECK_EQUAL(result_2.first, reference_ratio_2);
    BOOST_CHECK_EQUAL(result_2.second.lon, reference_point_2.lon);
    BOOST_CHECK_EQUAL(result_2.second.lat, reference_point_2.lat);

    //  t
    //  |
    //  |
    //  |
    //  s
    //  :
    //  i
    auto result_3 = coordinate_calculation::projectPointOnSegment(
        {FloatLongitude{0.}, FloatLatitude{0.}}, {FloatLongitude{0}, FloatLatitude{2}},
        {FloatLongitude{0}, FloatLatitude{-1}});
    auto reference_ratio_3 = 0.;
    auto reference_point_3 = FloatCoordinate{FloatLongitude{0}, FloatLatitude{0}};
    BOOST_CHECK_EQUAL(result_3.first, reference_ratio_3);
    BOOST_CHECK_EQUAL(result_3.second.lon, reference_point_3.lon);
    BOOST_CHECK_EQUAL(result_3.second.lat, reference_point_3.lat);

    //     t
    //    /
    //   /.
    //  /  i
    // s
    //
    auto result_4 = coordinate_calculation::projectPointOnSegment(
        {FloatLongitude{0}, FloatLatitude{0}}, {FloatLongitude{1}, FloatLatitude{1}},
        {FloatLongitude{0.5 + 0.1}, FloatLatitude{0.5 - 0.1}});
    auto reference_ratio_4 = 0.5;
    auto reference_point_4 = FloatCoordinate{FloatLongitude{0.5}, FloatLatitude{0.5}};
    BOOST_CHECK_EQUAL(result_4.first, reference_ratio_4);
    BOOST_CHECK_EQUAL(result_4.second.lon, reference_point_4.lon);
    BOOST_CHECK_EQUAL(result_4.second.lat, reference_point_4.lat);
}

BOOST_AUTO_TEST_SUITE_END()
