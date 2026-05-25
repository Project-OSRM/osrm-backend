#include "engine/polyline_compressor.hpp"
#include "util/coordinate.hpp"

#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

BOOST_AUTO_TEST_SUITE(polyline_compression)

BOOST_AUTO_TEST_CASE(polyline5_test_case)
{
    using namespace osrm::engine;
    using namespace osrm::util;

    const std::vector<Coordinate> coords({{-73.990171_lon, 40.714701_lat},
                                          {-73.991801_lon, 40.717571_lat},
                                          {-73.985751_lon, 40.715651_lat}});

    const std::vector<Coordinate> coords_truncated({{-73.990170_lon, 40.714700_lat},
                                                    {-73.991800_lon, 40.717570_lat},
                                                    {-73.985750_lon, 40.715650_lat}});

    BOOST_CHECK_EQUAL(encodePolyline(coords.begin(), coords.end()), "{aowFperbM}PdI~Jyd@");
    BOOST_CHECK(std::equal(coords_truncated.begin(),
                           coords_truncated.end(),
                           decodePolyline(encodePolyline(coords.begin(), coords.end())).begin()));
}

BOOST_AUTO_TEST_CASE(polyline6_test_case)
{
    using namespace osrm::engine;
    using namespace osrm::util;

    const std::vector<Coordinate> coords({{-73.990171_lon, 40.714701_lat},
                                          {-73.991801_lon, 40.717571_lat},
                                          {-73.985751_lon, 40.715651_lat}});

    BOOST_CHECK_EQUAL(encodePolyline<1000000>(coords.begin(), coords.end()),
                      "y{_tlAt`_clCkrDzdB~vBcyJ");
    BOOST_CHECK(std::equal(
        coords.begin(),
        coords.end(),
        decodePolyline<1000000>(encodePolyline<1000000>(coords.begin(), coords.end())).begin()));
}

BOOST_AUTO_TEST_CASE(empty_polyline_test)
{
    using namespace osrm::engine;
    using namespace osrm::util;

    std::vector<Coordinate> empty_coords;
    BOOST_CHECK_EQUAL(encodePolyline(empty_coords.begin(), empty_coords.end()), "");
    BOOST_CHECK(decodePolyline("").empty());
}
BOOST_AUTO_TEST_CASE(polyline_single_point_test)
{
    using namespace osrm::engine;
    using namespace osrm::util;

    const std::vector<Coordinate> coords({{-122.414000_lon, 37.776000_lat}});

    const std::string encoded = encodePolyline(coords.begin(), coords.end());
    BOOST_CHECK_EQUAL(encoded, "_cqeFn~cjV");

    const auto decoded = decodePolyline(encoded);
    BOOST_CHECK_EQUAL(decoded.size(), 1);
    BOOST_CHECK_EQUAL(decoded[0].lon, toFixed(-122.414000_lon));
    BOOST_CHECK_EQUAL(decoded[0].lat, toFixed(37.776000_lat));
}

BOOST_AUTO_TEST_CASE(polyline_multiple_points_test)
{
    using namespace osrm::engine;
    using namespace osrm::util;

    const std::vector<Coordinate> coords({{-122.414000_lon, 37.776000_lat},
                                          {-122.420000_lon, 37.779000_lat},
                                          {-122.421000_lon, 37.780000_lat}});

    const std::string encoded = encodePolyline(coords.begin(), coords.end());
    BOOST_CHECK_EQUAL(encoded, "_cqeFn~cjVwQnd@gEfE");

    const auto decoded = decodePolyline(encoded);
    BOOST_CHECK_EQUAL(decoded.size(), 3);
    for (size_t i = 0; i < coords.size(); ++i)
    {
        BOOST_CHECK_EQUAL(decoded[i].lon, coords[i].lon);
        BOOST_CHECK_EQUAL(decoded[i].lat, coords[i].lat);
    }
}

BOOST_AUTO_TEST_CASE(polyline_large_coordinate_difference_test)
{
    using namespace osrm::engine;
    using namespace osrm::util;

    const std::vector<Coordinate> coords(
        {{-179.000000_lon, -89.000000_lat}, {179.000000_lon, 89.000000_lat}});

    const std::string encoded = encodePolyline(coords.begin(), coords.end());
    BOOST_CHECK_EQUAL(encoded, "~xe~O~|oca@_sl}`@_{`hcA");

    const auto decoded = decodePolyline(encoded);
    BOOST_CHECK_EQUAL(decoded.size(), 2);
    for (size_t i = 0; i < coords.size(); ++i)
    {
        BOOST_CHECK_EQUAL(decoded[i].lon, coords[i].lon);
        BOOST_CHECK_EQUAL(decoded[i].lat, coords[i].lat);
    }
}

BOOST_AUTO_TEST_CASE(roundtrip)
{
    using namespace osrm::engine;
    using namespace osrm::util;

    {
        const auto encoded = "_chxEn`zvN\\\\]]";
        const auto decoded = decodePolyline(encoded);
        const auto reencoded = encodePolyline(decoded.begin(), decoded.end());
        BOOST_CHECK_EQUAL(encoded, reencoded);
    }
    {
        const auto encoded =
            "gcneIpgxzRcDnBoBlEHzKjBbHlG`@`IkDxIiKhKoMaLwTwHeIqHuAyGXeB~Ew@fFjAtIzExF";
        const auto decoded = decodePolyline(encoded);
        const auto reencoded = encodePolyline(decoded.begin(), decoded.end());
        BOOST_CHECK_EQUAL(encoded, reencoded);
    }
    {
        const auto encoded = "_p~iF~ps|U_ulLnnqC_mqNvxq`@";
        const auto decoded = decodePolyline(encoded);
        const auto reencoded = encodePolyline(decoded.begin(), decoded.end());
        BOOST_CHECK_EQUAL(encoded, reencoded);
    }
}

BOOST_AUTO_TEST_SUITE_END()
