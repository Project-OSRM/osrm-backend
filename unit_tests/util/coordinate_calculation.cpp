#include <boost/test/unit_test.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include "util/coordinate_calculation.hpp"

#include <osrm/coordinate.hpp>

#include <cmath>

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_SUITE(coordinate_calculation_tests)

BOOST_AUTO_TEST_CASE(compute_angle)
{
    // Simple cases
    // North-South straight line
    Coordinate first(FloatLongitude(1), FloatLatitude(-1));
    Coordinate middle(FloatLongitude(1), FloatLatitude(0));
    Coordinate end(FloatLongitude(1), FloatLatitude(1));
    auto angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_EQUAL(angle, 180);

    // North-South-North u-turn
    first = Coordinate(FloatLongitude(1), FloatLatitude(0));
    middle = Coordinate(FloatLongitude(1), FloatLatitude(1));
    end = Coordinate(FloatLongitude(1), FloatLatitude(0));
    angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_EQUAL(angle, 0);

    // East-west straight lines are harder, *simple* coordinates only
    // work at the equator.  For other locations, we need to follow
    // a rhumb line.
    first = Coordinate(FloatLongitude(1), FloatLatitude(0));
    middle = Coordinate(FloatLongitude(2), FloatLatitude(0));
    end = Coordinate(FloatLongitude(3), FloatLatitude(0));
    angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_EQUAL(angle, 180);

    // East-West-East u-turn
    first = Coordinate(FloatLongitude(1), FloatLatitude(0));
    middle = Coordinate(FloatLongitude(2), FloatLatitude(0));
    end = Coordinate(FloatLongitude(1), FloatLatitude(0));
    angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_EQUAL(angle, 0);

    // 90 degree left turn
    first = Coordinate(FloatLongitude(1), FloatLatitude(1));
    middle = Coordinate(FloatLongitude(0), FloatLatitude(1));
    end = Coordinate(FloatLongitude(0), FloatLatitude(2));
    angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_EQUAL(angle, 90);

    // 90 degree right turn
    first = Coordinate(FloatLongitude(1), FloatLatitude(1));
    middle = Coordinate(FloatLongitude(0), FloatLatitude(1));
    end = Coordinate(FloatLongitude(0), FloatLatitude(0));
    angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_EQUAL(angle, 270);

    // Weird cases
    // Crossing both the meridians
    first = Coordinate(FloatLongitude(-1), FloatLatitude(-1));
    middle = Coordinate(FloatLongitude(0), FloatLatitude(1));
    end = Coordinate(FloatLongitude(1), FloatLatitude(-1));
    angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_CLOSE(angle, 53.1, 0.2);

    // All coords in the same spot
    first = Coordinate(FloatLongitude(-1), FloatLatitude(-1));
    middle = Coordinate(FloatLongitude(-1), FloatLatitude(-1));
    end = Coordinate(FloatLongitude(-1), FloatLatitude(-1));
    angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_EQUAL(angle, 180);

    // First two coords in the same spot, then heading north-east
    first = Coordinate(FloatLongitude(-1), FloatLatitude(-1));
    middle = Coordinate(FloatLongitude(-1), FloatLatitude(-1));
    end = Coordinate(FloatLongitude(1), FloatLatitude(1));
    angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_EQUAL(angle, 180);

    // First two coords in the same spot, then heading west
    first = Coordinate(FloatLongitude(1), FloatLatitude(1));
    middle = Coordinate(FloatLongitude(1), FloatLatitude(1));
    end = Coordinate(FloatLongitude(2), FloatLatitude(1));
    angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_EQUAL(angle, 180);

    // First two coords in the same spot then heading north
    first = Coordinate(FloatLongitude(1), FloatLatitude(1));
    middle = Coordinate(FloatLongitude(1), FloatLatitude(1));
    end = Coordinate(FloatLongitude(1), FloatLatitude(2));
    angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_EQUAL(angle, 180);

    // Second two coords in the same spot
    first = Coordinate(FloatLongitude(1), FloatLatitude(1));
    middle = Coordinate(FloatLongitude(-1), FloatLatitude(-1));
    end = Coordinate(FloatLongitude(-1), FloatLatitude(-1));
    angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_EQUAL(angle, 180);

    // First and last coords on the same spot
    first = Coordinate(FloatLongitude(1), FloatLatitude(1));
    middle = Coordinate(FloatLongitude(-1), FloatLatitude(-1));
    end = Coordinate(FloatLongitude(1), FloatLatitude(1));
    angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_EQUAL(angle, 0);

    // Check the antimeridian
    first = Coordinate(FloatLongitude(180), FloatLatitude(90));
    middle = Coordinate(FloatLongitude(180), FloatLatitude(0));
    end = Coordinate(FloatLongitude(180), FloatLatitude(-90));
    angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_EQUAL(angle, 180);

    // Tiny changes below our calculation resolution
    // This should be equivalent to having two points on the same
    // spot.
    first = Coordinate(FloatLongitude(0), FloatLatitude(0));
    middle = Coordinate(FloatLongitude(1), FloatLatitude(0));
    end = Coordinate(FloatLongitude(1 + std::numeric_limits<double>::epsilon()), FloatLatitude(0));
    angle = coordinate_calculation::computeAngle(first,middle,end);
    BOOST_CHECK_EQUAL(angle, 180);

    // Invalid values
    /* TODO: Enable this when I figure out how to use BOOST_CHECK_THROW
     *       and not have the whole test case fail...
    first = Coordinate(FloatLongitude(0), FloatLatitude(0));
    middle = Coordinate(FloatLongitude(1), FloatLatitude(0));
    end = Coordinate(FloatLongitude(std::numeric_limits<double>::max()), FloatLatitude(0));
    BOOST_CHECK_THROW( coordinate_calculation::computeAngle(first,middle,end),
                       boost::numeric::positive_overflow);
                       */

}


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
