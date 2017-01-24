#include <boost/numeric/conversion/cast.hpp>
#include <boost/test/unit_test.hpp>

#include "util/bearing.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/web_mercator.hpp"

#include <osrm/coordinate.hpp>

#include <cmath>

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_SUITE(coordinate_calculation_tests)

BOOST_AUTO_TEST_CASE(compute_angle)
{
    // Simple cases
    // North-South straight line
    Coordinate first(FloatLongitude{1}, FloatLatitude{-1});
    Coordinate middle(FloatLongitude{1}, FloatLatitude{0});
    Coordinate end(FloatLongitude{1}, FloatLatitude{1});
    auto angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // North-South-North u-turn
    first = Coordinate(FloatLongitude{1}, FloatLatitude{0});
    middle = Coordinate(FloatLongitude{1}, FloatLatitude{1});
    end = Coordinate(FloatLongitude{1}, FloatLatitude{0});
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 0);

    // East-west straight lines are harder, *simple* coordinates only
    // work at the equator.  For other locations, we need to follow
    // a rhumb line.
    first = Coordinate(FloatLongitude{1}, FloatLatitude{0});
    middle = Coordinate(FloatLongitude{2}, FloatLatitude{0});
    end = Coordinate(FloatLongitude{3}, FloatLatitude{0});
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // East-West-East u-turn
    first = Coordinate(FloatLongitude{1}, FloatLatitude{0});
    middle = Coordinate(FloatLongitude{2}, FloatLatitude{0});
    end = Coordinate(FloatLongitude{1}, FloatLatitude{0});
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 0);

    // 90 degree left turn
    first = Coordinate(FloatLongitude{1}, FloatLatitude{1});
    middle = Coordinate(FloatLongitude{0}, FloatLatitude{1});
    end = Coordinate(FloatLongitude{0}, FloatLatitude{2});
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 90);

    // 90 degree right turn
    first = Coordinate(FloatLongitude{1}, FloatLatitude{1});
    middle = Coordinate(FloatLongitude{0}, FloatLatitude{1});
    end = Coordinate(FloatLongitude{0}, FloatLatitude{0});
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 270);

    // Weird cases
    // Crossing both the meridians
    first = Coordinate(FloatLongitude{-1}, FloatLatitude{-1});
    middle = Coordinate(FloatLongitude{0}, FloatLatitude{1});
    end = Coordinate(FloatLongitude{1}, FloatLatitude{-1});
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_CLOSE(angle, 53.1, 0.2);

    // All coords in the same spot
    first = Coordinate(FloatLongitude{-1}, FloatLatitude{-1});
    middle = Coordinate(FloatLongitude{-1}, FloatLatitude{-1});
    end = Coordinate(FloatLongitude{-1}, FloatLatitude{-1});
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // First two coords in the same spot, then heading north-east
    first = Coordinate(FloatLongitude{-1}, FloatLatitude{-1});
    middle = Coordinate(FloatLongitude{-1}, FloatLatitude{-1});
    end = Coordinate(FloatLongitude{1}, FloatLatitude{1});
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // First two coords in the same spot, then heading west
    first = Coordinate(FloatLongitude{1}, FloatLatitude{1});
    middle = Coordinate(FloatLongitude{1}, FloatLatitude{1});
    end = Coordinate(FloatLongitude{2}, FloatLatitude{1});
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // First two coords in the same spot then heading north
    first = Coordinate(FloatLongitude{1}, FloatLatitude{1});
    middle = Coordinate(FloatLongitude{1}, FloatLatitude{1});
    end = Coordinate(FloatLongitude{1}, FloatLatitude{2});
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // Second two coords in the same spot
    first = Coordinate(FloatLongitude{1}, FloatLatitude{1});
    middle = Coordinate(FloatLongitude{-1}, FloatLatitude{-1});
    end = Coordinate(FloatLongitude{-1}, FloatLatitude{-1});
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // First and last coords on the same spot
    first = Coordinate(FloatLongitude{1}, FloatLatitude{1});
    middle = Coordinate(FloatLongitude{-1}, FloatLatitude{-1});
    end = Coordinate(FloatLongitude{1}, FloatLatitude{1});
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 0);

    // Check the antimeridian
    first = Coordinate(FloatLongitude{180}, FloatLatitude{90});
    middle = Coordinate(FloatLongitude{180}, FloatLatitude{0});
    end = Coordinate(FloatLongitude{180}, FloatLatitude{-90});
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // Tiny changes below our calculation resolution
    // This should be equivalent to having two points on the same spot.
    first = Coordinate{FloatLongitude{0}, FloatLatitude{0}};
    middle = Coordinate{FloatLongitude{1}, FloatLatitude{0}};
    end = Coordinate{FloatLongitude{1 + std::numeric_limits<double>::epsilon()}, FloatLatitude{0}};
    angle = coordinate_calculation::computeAngle(first, middle, end);
    BOOST_CHECK_EQUAL(angle, 180);

    // Invalid values
    BOOST_CHECK_THROW(
        coordinate_calculation::computeAngle(
            Coordinate(FloatLongitude{0}, FloatLatitude{0}),
            Coordinate(FloatLongitude{1}, FloatLatitude{0}),
            Coordinate(FloatLongitude{std::numeric_limits<double>::max()}, FloatLatitude{0})),
        boost::numeric::positive_overflow);
}

// Regression test for bug captured in #1347
BOOST_AUTO_TEST_CASE(regression_test_1347)
{
    Coordinate u(FloatLongitude{-100}, FloatLatitude{10});
    Coordinate v(FloatLongitude{-100.002}, FloatLatitude{10.001});
    Coordinate q(FloatLongitude{-100.001}, FloatLatitude{10.002});

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

    BOOST_CHECK_CLOSE(static_cast<double>(start.lon + FloatLongitude{ratio} * diff.lon),
                      static_cast<double>(nearest.lon),
                      0.1);
    BOOST_CHECK_CLOSE(static_cast<double>(start.lat + FloatLatitude{ratio} * diff.lat),
                      static_cast<double>(nearest.lat),
                      0.1);
}

BOOST_AUTO_TEST_CASE(point_on_segment)
{
    //  t
    //  |
    //  |---- i
    //  |
    //  s
    auto result_1 =
        coordinate_calculation::projectPointOnSegment({FloatLongitude{0}, FloatLatitude{0}},
                                                      {FloatLongitude{0}, FloatLatitude{2}},
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
    auto result_2 =
        coordinate_calculation::projectPointOnSegment({FloatLongitude{0.}, FloatLatitude{0.}},
                                                      {FloatLongitude{0}, FloatLatitude{2}},
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
    auto result_3 =
        coordinate_calculation::projectPointOnSegment({FloatLongitude{0.}, FloatLatitude{0.}},
                                                      {FloatLongitude{0}, FloatLatitude{2}},
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
        {FloatLongitude{0}, FloatLatitude{0}},
        {FloatLongitude{1}, FloatLatitude{1}},
        {FloatLongitude{0.5 + 0.1}, FloatLatitude{0.5 - 0.1}});
    auto reference_ratio_4 = 0.5;
    auto reference_point_4 = FloatCoordinate{FloatLongitude{0.5}, FloatLatitude{0.5}};
    BOOST_CHECK_EQUAL(result_4.first, reference_ratio_4);
    BOOST_CHECK_EQUAL(result_4.second.lon, reference_point_4.lon);
    BOOST_CHECK_EQUAL(result_4.second.lat, reference_point_4.lat);
}

BOOST_AUTO_TEST_CASE(circleCenter)
{
    Coordinate a(FloatLongitude{-100.}, FloatLatitude{10.});
    Coordinate b(FloatLongitude{-100.002}, FloatLatitude{10.001});
    Coordinate c(FloatLongitude{-100.001}, FloatLatitude{10.002});

    auto result = coordinate_calculation::circleCenter(a, b, c);
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(*result, Coordinate(FloatLongitude{-100.0008333}, FloatLatitude{10.0008333}));

    // Co-linear longitude
    a = Coordinate(FloatLongitude{-100.}, FloatLatitude{10.});
    b = Coordinate(FloatLongitude{-100.001}, FloatLatitude{10.001});
    c = Coordinate(FloatLongitude{-100.001}, FloatLatitude{10.002});
    result = coordinate_calculation::circleCenter(a, b, c);
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(*result, Coordinate(FloatLongitude{-99.9995}, FloatLatitude{10.0015}));

    // Co-linear longitude, impossible to calculate
    a = Coordinate(FloatLongitude{-100.001}, FloatLatitude{10.});
    b = Coordinate(FloatLongitude{-100.001}, FloatLatitude{10.001});
    c = Coordinate(FloatLongitude{-100.001}, FloatLatitude{10.002});
    result = coordinate_calculation::circleCenter(a, b, c);
    BOOST_CHECK(!result);

    // Co-linear latitude, this is a real case that failed
    a = Coordinate(FloatLongitude{-112.096234}, FloatLatitude{41.147101});
    b = Coordinate(FloatLongitude{-112.096606}, FloatLatitude{41.147101});
    c = Coordinate(FloatLongitude{-112.096419}, FloatLatitude{41.147259});
    result = coordinate_calculation::circleCenter(a, b, c);
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(*result, Coordinate(FloatLongitude{-112.09642}, FloatLatitude{41.1470705}));

    // Co-linear latitude, variation
    a = Coordinate(FloatLongitude{-112.096234}, FloatLatitude{41.147101});
    b = Coordinate(FloatLongitude{-112.096606}, FloatLatitude{41.147259});
    c = Coordinate(FloatLongitude{-112.096419}, FloatLatitude{41.147259});
    result = coordinate_calculation::circleCenter(a, b, c);
    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(*result, Coordinate(FloatLongitude{-112.0965125}, FloatLatitude{41.1469622}));

    // Co-linear latitude, impossible to calculate
    a = Coordinate(FloatLongitude{-112.096234}, FloatLatitude{41.147259});
    b = Coordinate(FloatLongitude{-112.096606}, FloatLatitude{41.147259});
    c = Coordinate(FloatLongitude{-112.096419}, FloatLatitude{41.147259});
    result = coordinate_calculation::circleCenter(a, b, c);
    BOOST_CHECK(!result);

    // Out of bounds
    a = Coordinate(FloatLongitude{-112.096234}, FloatLatitude{41.147258});
    b = Coordinate(FloatLongitude{-112.106606}, FloatLatitude{41.147259});
    c = Coordinate(FloatLongitude{-113.096419}, FloatLatitude{41.147258});
    result = coordinate_calculation::circleCenter(a, b, c);
    BOOST_CHECK(!result);
}

// For overflow issue #3483, introduced in 68ee4eab61548. Run with -fsanitize=integer.
BOOST_AUTO_TEST_CASE(squaredEuclideanDistance)
{
    // Overflow happens when left hand side values are smaller than right hand side values,
    // then `lhs - rhs` will be negative but stored in a uint64_t (wraps around).

    Coordinate lhs(FloatLongitude{-180}, FloatLatitude{-90});
    Coordinate rhs(FloatLongitude{180}, FloatLatitude{90});

    const auto result = coordinate_calculation::squaredEuclideanDistance(lhs, rhs);

    BOOST_CHECK_EQUAL(result, 162000000000000000ull);
}

BOOST_AUTO_TEST_CASE(vertical_regression)
{
    // check a vertical line for its bearing
    std::vector<Coordinate> coordinates;
    for (std::size_t i = 0; i < 100; ++i)
        coordinates.push_back(Coordinate(FloatLongitude{0.0}, FloatLatitude{i / 100.0}));

    const auto regression =
        util::coordinate_calculation::leastSquareRegression(coordinates.begin(), coordinates.end());
    const auto is_valid =
        util::angularDeviation(
            util::coordinate_calculation::bearing(regression.first, regression.second), 0) < 2;
    BOOST_CHECK(is_valid);
}

BOOST_AUTO_TEST_CASE(sinus_curve)
{
    // create a full sinus curve, sampled in 3.6 degree
    std::vector<Coordinate> coordinates;
    for (std::size_t i = 0; i < 360; ++i)
        coordinates.push_back(Coordinate(
            FloatLongitude{i / 360.0},
            FloatLatitude{sin(util::coordinate_calculation::detail::degToRad(i / 360.0))}));

    const auto regression =
        util::coordinate_calculation::leastSquareRegression(coordinates.begin(), coordinates.end());
    const auto is_valid =
        util::angularDeviation(
            util::coordinate_calculation::bearing(regression.first, regression.second), 90) < 2;

    BOOST_CHECK(is_valid);
}

BOOST_AUTO_TEST_CASE(parallel_lines_slight_offset)
{
    std::vector<Coordinate> coordinates_lhs;
    for (std::size_t i = 0; i < 100; ++i)
        coordinates_lhs.push_back(Coordinate(util::FloatLongitude{(50 - (rand() % 101)) / 100000.0},
                                             util::FloatLatitude{i / 100000.0}));
    std::vector<Coordinate> coordinates_rhs;
    for (std::size_t i = 0; i < 100; ++i)
        coordinates_rhs.push_back(
            Coordinate(util::FloatLongitude{(150 - (rand() % 101)) / 100000.0},
                       util::FloatLatitude{i / 100000.0}));

    const auto are_parallel = util::coordinate_calculation::areParallel(coordinates_lhs.begin(),
                                                                        coordinates_lhs.end(),
                                                                        coordinates_rhs.begin(),
                                                                        coordinates_rhs.end());
    BOOST_CHECK(are_parallel);
}

BOOST_AUTO_TEST_CASE(consistent_invalid_bearing_result)
{
    const auto pos1 = Coordinate(util::FloatLongitude{0.}, util::FloatLatitude{0.});
    const auto pos2 = Coordinate(util::FloatLongitude{5.}, util::FloatLatitude{5.});
    const auto pos3 = Coordinate(util::FloatLongitude{-5.}, util::FloatLatitude{-5.});

    BOOST_CHECK_EQUAL(0., util::coordinate_calculation::bearing(pos1, pos1));
    BOOST_CHECK_EQUAL(0., util::coordinate_calculation::bearing(pos2, pos2));
    BOOST_CHECK_EQUAL(0., util::coordinate_calculation::bearing(pos3, pos3));
}

// Regression test for bug captured in #3516
BOOST_AUTO_TEST_CASE(regression_test_3516)
{
    Coordinate u(FloatLongitude{-73.989687}, FloatLatitude{40.752288});
    Coordinate v(FloatLongitude{-73.990134}, FloatLatitude{40.751658});
    Coordinate q(FloatLongitude{-73.99039}, FloatLatitude{40.75171});

    BOOST_CHECK_EQUAL(Coordinate{web_mercator::toWGS84(web_mercator::fromWGS84(u))}, u);
    BOOST_CHECK_EQUAL(Coordinate{web_mercator::toWGS84(web_mercator::fromWGS84(v))}, v);

    double ratio;
    Coordinate nearest_location;
    coordinate_calculation::perpendicularDistance(u, v, q, nearest_location, ratio);

    BOOST_CHECK_EQUAL(ratio, 1.);
    BOOST_CHECK_EQUAL(nearest_location, v);
}

BOOST_AUTO_TEST_SUITE_END()
