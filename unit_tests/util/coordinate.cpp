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

BOOST_AUTO_TEST_CASE(lon_to_pixel)
{
    using namespace coordinate_calculation;
    BOOST_CHECK_CLOSE(7.416042 * mercator::DEGREE_TO_PX, 825550.019142, 0.1);
    BOOST_CHECK_CLOSE(7.415892 * mercator::DEGREE_TO_PX, 825533.321218, 0.1);
    BOOST_CHECK_CLOSE(7.416016 * mercator::DEGREE_TO_PX, 825547.124835, 0.1);
    BOOST_CHECK_CLOSE(7.41577 * mercator::DEGREE_TO_PX, 825519.74024, 0.1);
    BOOST_CHECK_CLOSE(7.415808 * mercator::DEGREE_TO_PX, 825523.970381, 0.1);
}

BOOST_AUTO_TEST_CASE(lat_to_pixel)
{
    using namespace coordinate_calculation;
    BOOST_CHECK_CLOSE(mercator::latToY(util::FloatLatitude(43.733947)) * mercator::DEGREE_TO_PX,
                      5424361.75863, 0.1);
    BOOST_CHECK_CLOSE(mercator::latToY(util::FloatLatitude(43.733799)) * mercator::DEGREE_TO_PX,
                      5424338.95731, 0.1);
    BOOST_CHECK_CLOSE(mercator::latToY(util::FloatLatitude(43.733922)) * mercator::DEGREE_TO_PX,
                      5424357.90705, 0.1);
    BOOST_CHECK_CLOSE(mercator::latToY(util::FloatLatitude(43.733697)) * mercator::DEGREE_TO_PX,
                      5424323.24293, 0.1);
    BOOST_CHECK_CLOSE(mercator::latToY(util::FloatLatitude(43.733729)) * mercator::DEGREE_TO_PX,
                      5424328.17293, 0.1);
}

BOOST_AUTO_TEST_CASE(xyz_to_wgs84)
{
    using namespace coordinate_calculation;

    double minx_1;
    double miny_1;
    double maxx_1;
    double maxy_1;
    mercator::xyzToWSG84(2, 2, 1, minx_1, miny_1, maxx_1, maxy_1);
    BOOST_CHECK_CLOSE(minx_1, 180, 0.0001);
    BOOST_CHECK_CLOSE(miny_1, -89.786, 0.0001);
    BOOST_CHECK_CLOSE(maxx_1, 360, 0.0001);
    BOOST_CHECK_CLOSE(maxy_1, -85.0511, 0.0001);

    double minx_2;
    double miny_2;
    double maxx_2;
    double maxy_2;
    mercator::xyzToWSG84(100, 0, 13, minx_2, miny_2, maxx_2, maxy_2);
    BOOST_CHECK_CLOSE(minx_2, -175.6054, 0.0001);
    BOOST_CHECK_CLOSE(miny_2, 85.0473, 0.0001);
    BOOST_CHECK_CLOSE(maxx_2, -175.5615, 0.0001);
    BOOST_CHECK_CLOSE(maxy_2, 85.0511, 0.0001);
}

BOOST_AUTO_TEST_CASE(xyz_to_mercator)
{
    using namespace coordinate_calculation;

    double minx;
    double miny;
    double maxx;
    double maxy;
    mercator::xyzToMercator(100, 0, 13, minx, miny, maxx, maxy);

    BOOST_CHECK_CLOSE(minx, -19548311.361764118075, 0.0001);
    BOOST_CHECK_CLOSE(miny, 20032616.372979003936, 0.0001);
    BOOST_CHECK_CLOSE(maxx, -19543419.391953866929, 0.0001);
    BOOST_CHECK_CLOSE(maxy, 20037508.342789277434, 0.0001);
}
