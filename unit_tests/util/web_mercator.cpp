#include <boost/test/unit_test.hpp>

#include "util/web_mercator.hpp"

#include <osrm/coordinate.hpp>

#include <cmath>

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_SUITE(web_mercator_tests)

BOOST_AUTO_TEST_CASE(lon_to_pixel)
{
    BOOST_CHECK_CLOSE(7.416042 * web_mercator::DEGREE_TO_PX, 825550.019142, 0.1);
    BOOST_CHECK_CLOSE(7.415892 * web_mercator::DEGREE_TO_PX, 825533.321218, 0.1);
    BOOST_CHECK_CLOSE(7.416016 * web_mercator::DEGREE_TO_PX, 825547.124835, 0.1);
    BOOST_CHECK_CLOSE(7.41577 * web_mercator::DEGREE_TO_PX, 825519.74024, 0.1);
    BOOST_CHECK_CLOSE(7.415808 * web_mercator::DEGREE_TO_PX, 825523.970381, 0.1);
}

BOOST_AUTO_TEST_CASE(lat_to_pixel)
{
    BOOST_CHECK_CLOSE(web_mercator::latToY(util::FloatLatitude{43.733947}) *
                          web_mercator::DEGREE_TO_PX,
                      5424361.75863,
                      0.1);
    BOOST_CHECK_CLOSE(web_mercator::latToY(util::FloatLatitude{43.733799}) *
                          web_mercator::DEGREE_TO_PX,
                      5424338.95731,
                      0.1);
    BOOST_CHECK_CLOSE(web_mercator::latToY(util::FloatLatitude{43.733922}) *
                          web_mercator::DEGREE_TO_PX,
                      5424357.90705,
                      0.1);
    BOOST_CHECK_CLOSE(web_mercator::latToY(util::FloatLatitude{43.733697}) *
                          web_mercator::DEGREE_TO_PX,
                      5424323.24293,
                      0.1);
    BOOST_CHECK_CLOSE(web_mercator::latToY(util::FloatLatitude{43.733729}) *
                          web_mercator::DEGREE_TO_PX,
                      5424328.17293,
                      0.1);
}

BOOST_AUTO_TEST_CASE(xyz_to_wgs84)
{
    double minx_1;
    double miny_1;
    double maxx_1;
    double maxy_1;
    web_mercator::xyzToWGS84(2, 2, 1, minx_1, miny_1, maxx_1, maxy_1);
    BOOST_CHECK_CLOSE(minx_1, 180, 0.0001);
    BOOST_CHECK_CLOSE(miny_1, -85.0511, 0.0001);
    BOOST_CHECK_CLOSE(maxx_1, 360, 0.0001);
    BOOST_CHECK_CLOSE(maxy_1, -85.0511, 0.0001);

    double minx_2;
    double miny_2;
    double maxx_2;
    double maxy_2;
    web_mercator::xyzToWGS84(100, 0, 13, minx_2, miny_2, maxx_2, maxy_2);
    BOOST_CHECK_CLOSE(minx_2, -175.6054, 0.0001);
    BOOST_CHECK_CLOSE(miny_2, 85.0473, 0.0001);
    BOOST_CHECK_CLOSE(maxx_2, -175.5615, 0.0001);
    BOOST_CHECK_CLOSE(maxy_2, 85.0511, 0.0001);
}

BOOST_AUTO_TEST_CASE(xyz_to_mercator)
{
    double minx;
    double miny;
    double maxx;
    double maxy;

    // http://tools.geofabrik.de/map/#13/85.0500/-175.5876&type=Geofabrik_Standard&grid=1
    web_mercator::xyzToMercator(100, 0, 13, minx, miny, maxx, maxy);

    BOOST_CHECK_CLOSE(minx, -19548311.361764118075, 0.0001);
    BOOST_CHECK_CLOSE(miny, 20032616.372979045, 0.0001);
    BOOST_CHECK_CLOSE(maxx, -19543419.391953866929, 0.0001);
    BOOST_CHECK_CLOSE(maxy, 20037508.342789277, 0.0001); // Mercator 6378137*pi, WGS 85.0511
}

BOOST_AUTO_TEST_SUITE_END()
