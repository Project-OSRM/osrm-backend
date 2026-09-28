#include <boost/test/unit_test.hpp>

#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"

using namespace osrm::util;

static double shortestAngularDistance(double a, double b)
{
    double diff = std::abs(a - b);
    if (diff > 180.0)
        diff = 360.0 - diff;
    return diff;
}

BOOST_AUTO_TEST_SUITE(antimeridian_regression_tests)

BOOST_AUTO_TEST_CASE(least_square_regression_handles_antimeridian)
{
    std::vector<Coordinate> coords{
        Coordinate{FloatLongitude{179.5}, FloatLatitude{0}},
        Coordinate{FloatLongitude{-179.0}, FloatLatitude{1}},
        Coordinate{FloatLongitude{179.9}, FloatLatitude{2}},
    };

    const auto regression =
        osrm::util::coordinate_calculation::leastSquareRegression(coords.begin(), coords.end());

    const double lon1 = static_cast<double>(toFloating(regression.first.lon));
    const double lon2 = static_cast<double>(toFloating(regression.second.lon));

    const double ang_diff = shortestAngularDistance(lon1, lon2);

    // Expect the regression to consider the short wraparound (distance small)
    BOOST_CHECK_LT(ang_diff, 5.0);
}

BOOST_AUTO_TEST_CASE(least_square_regression_regular_case)
{
    std::vector<Coordinate> coords{
        Coordinate{FloatLongitude{10.0}, FloatLatitude{0}},
        Coordinate{FloatLongitude{20.0}, FloatLatitude{1}},
        Coordinate{FloatLongitude{30.0}, FloatLatitude{2}},
    };

    const auto regression =
        osrm::util::coordinate_calculation::leastSquareRegression(coords.begin(), coords.end());

    const double lon1 = static_cast<double>(toFloating(regression.first.lon));
    const double lon2 = static_cast<double>(toFloating(regression.second.lon));

    const double ang_diff = std::abs(lon1 - lon2);

    // Expect the regression line to span roughly the lon range (> 15 degrees)
    BOOST_CHECK_GT(ang_diff, 15.0);
}

BOOST_AUTO_TEST_SUITE_END()
