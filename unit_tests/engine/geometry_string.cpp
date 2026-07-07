#include <boost/test/tools/floating_point_comparison.hpp>
#include <boost/test/unit_test.hpp>

#include "engine/polyline_compressor.hpp"
#include "util/coordinate_calculation.hpp"

#include <osrm/coordinate.hpp>

#include <cmath>
#include <vector>

BOOST_AUTO_TEST_SUITE(polyline)

using namespace osrm;
using namespace osrm::engine;

BOOST_AUTO_TEST_CASE(decode)
{
    // Polyline string for the 5 coordinates
    const std::string polyline = "_c`|@_c`|@o}@_pRo}@_pRo}@_pRo}@_pR";
    const auto coords = decodePolyline(polyline);

    // Test coordinates; these would be the coordinates we give the loc parameter,
    // e.g. loc=10.00,10.0&loc=10.01,10.1...
    util::Coordinate coord1(10.0_lon, 10.00_lat);
    util::Coordinate coord2(10.1_lon, 10.01_lat);
    util::Coordinate coord3(10.2_lon, 10.02_lat);
    util::Coordinate coord4(10.3_lon, 10.03_lat);
    util::Coordinate coord5(10.4_lon, 10.04_lat);

    // Put the test coordinates into the vector for comparison
    std::vector<util::Coordinate> cmp_coords = {coord1, coord2, coord3, coord4, coord5};

    BOOST_CHECK_EQUAL(cmp_coords.size(), coords.size());

    for (std::size_t i = 0; i < cmp_coords.size(); ++i)
    {
        BOOST_CHECK_CLOSE(static_cast<double>(util::toFloating(coords[i].lat)),
                          static_cast<double>(util::toFloating(cmp_coords[i].lat)),
                          0.0001);
        BOOST_CHECK_CLOSE(static_cast<double>(util::toFloating(coords[i].lon)),
                          static_cast<double>(util::toFloating(cmp_coords[i].lon)),
                          0.0001);
    }
}

BOOST_AUTO_TEST_CASE(encode)
{
    // Coordinates; these would be the coordinates we give the loc parameter,
    // e.g. loc=10.00,10.0&loc=10.01,10.1...
    util::Coordinate coord1(10.0_lon, 10.00_lat);
    util::Coordinate coord2(10.1_lon, 10.01_lat);
    util::Coordinate coord3(10.2_lon, 10.02_lat);
    util::Coordinate coord4(10.3_lon, 10.03_lat);
    util::Coordinate coord5(10.4_lon, 10.04_lat);

    // Test polyline string for the 5 coordinates
    const std::string polyline = "_c`|@_c`|@o}@_pRo}@_pRo}@_pRo}@_pR";

    // Put the test coordinates into the vector for comparison
    std::vector<util::Coordinate> cmp_coords = {coord1, coord2, coord3, coord4, coord5};

    const auto encodedPolyline = encodePolyline<100000>(cmp_coords.begin(), cmp_coords.end());

    BOOST_CHECK_EQUAL(encodedPolyline, polyline);
}

BOOST_AUTO_TEST_CASE(encode6)
{
    // Coordinates; these would be the coordinates we give the loc parameter,
    // e.g. loc=10.00,10.0&loc=10.01,10.1...
    util::Coordinate coord1(10.0_lon, 10.00_lat);
    util::Coordinate coord2(10.1_lon, 10.01_lat);
    util::Coordinate coord3(10.2_lon, 10.02_lat);
    util::Coordinate coord4(10.3_lon, 10.03_lat);
    util::Coordinate coord5(10.4_lon, 10.04_lat);

    // Test polyline string for the 6 coordinates
    const std::string polyline = "_gjaR_gjaR_pR_ibE_pR_ibE_pR_ibE_pR_ibE";

    // Put the test coordinates into the vector for comparison
    std::vector<util::Coordinate> cmp_coords = {coord1, coord2, coord3, coord4, coord5};

    const auto encodedPolyline = encodePolyline<1000000>(cmp_coords.begin(), cmp_coords.end());

    BOOST_CHECK_EQUAL(encodedPolyline, polyline);
}

BOOST_AUTO_TEST_CASE(polyline_sign_check)
{
    // check sign conversion correctness from zig-zag encoding to two's complement
    std::vector<util::Coordinate> coords = {
        {0.0_lon, 0.0_lat}, {-0.00001_lon, 0.00000_lat}, {0.00000_lon, -0.00001_lat}};

    const auto polyline = encodePolyline<100000>(coords.begin(), coords.end());
    const auto result = decodePolyline(polyline);

    BOOST_CHECK(coords.size() == result.size());
    for (std::size_t i = 0; i < result.size(); ++i)
    {
        BOOST_CHECK(coords[i] == result[i]);
    }
}

BOOST_AUTO_TEST_CASE(polyline_short_strings)
{
    // check zero longitude difference in the last coordinate
    // the polyline is incorrect, but decodePolyline must not fail
    std::vector<util::Coordinate> coords = {
        {13.32476_lon, 52.52632_lat}, {13.30179_lon, 52.59155_lat}, {13.30179_lon, 52.60391_lat}};

    const auto polyline = encodePolyline<100000>(coords.begin(), coords.end());
    BOOST_CHECK(polyline.back() == '?');

    const auto result_short = decodePolyline(polyline.substr(0, polyline.size() - 1));
    BOOST_CHECK(coords.size() == result_short.size());
    for (std::size_t i = 0; i < result_short.size(); ++i)
    {
        BOOST_CHECK(coords[i] == result_short[i]);
    }
}

BOOST_AUTO_TEST_CASE(incorrect_polylines)
{
    // check incorrect polylines
    std::vector<std::string> polylines = {
        "?",       // latitude only
        "_",       // unfinished latitude
        "?_",      // unfinished longitude
        "?_______" // too long longitude (35 bits)
    };
    util::Coordinate coord{0.0_lon, 0.0_lat};

    for (const auto &polyline : polylines)
    {
        const auto result = decodePolyline(polyline);
        BOOST_CHECK(result.size() == 1);
        BOOST_CHECK(result.front() == coord);
    }
}

BOOST_AUTO_TEST_SUITE_END()
