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

    const std::vector<Coordinate> coords({{FixedLongitude{-73990171}, FixedLatitude{40714701}},
                                          {FixedLongitude{-73991801}, FixedLatitude{40717571}},
                                          {FixedLongitude{-73985751}, FixedLatitude{40715651}}});

    const std::vector<Coordinate> coords_truncated(
        {{FixedLongitude{-73990170}, FixedLatitude{40714700}},
         {FixedLongitude{-73991800}, FixedLatitude{40717570}},
         {FixedLongitude{-73985750}, FixedLatitude{40715650}}});

    BOOST_CHECK_EQUAL(encodePolyline(coords.begin(), coords.end()), "{aowFperbM}PdI~Jyd@");
    BOOST_CHECK(std::equal(coords_truncated.begin(),
                           coords_truncated.end(),
                           decodePolyline(encodePolyline(coords.begin(), coords.end())).begin()));
}

BOOST_AUTO_TEST_CASE(polyline6_test_case)
{
    using namespace osrm::engine;
    using namespace osrm::util;

    const std::vector<Coordinate> coords({{FixedLongitude{-73990171}, FixedLatitude{40714701}},
                                          {FixedLongitude{-73991801}, FixedLatitude{40717571}},
                                          {FixedLongitude{-73985751}, FixedLatitude{40715651}}});

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

    const std::vector<Coordinate> coords({{FixedLongitude{-122414000}, FixedLatitude{37776000}}});

    const std::string encoded = encodePolyline(coords.begin(), coords.end());
    BOOST_CHECK_EQUAL(encoded, "_cqeFn~cjV");

    const auto decoded = decodePolyline(encoded);
    BOOST_CHECK_EQUAL(decoded.size(), 1);
    BOOST_CHECK_EQUAL(decoded[0].lon, FixedLongitude{-122414000});
    BOOST_CHECK_EQUAL(decoded[0].lat, FixedLatitude{37776000});
}

BOOST_AUTO_TEST_CASE(polyline_multiple_points_test)
{
    using namespace osrm::engine;
    using namespace osrm::util;

    const std::vector<Coordinate> coords({{FixedLongitude{-122414000}, FixedLatitude{37776000}},
                                          {FixedLongitude{-122420000}, FixedLatitude{37779000}},
                                          {FixedLongitude{-122421000}, FixedLatitude{37780000}}});

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

    const std::vector<Coordinate> coords({{FixedLongitude{-179000000}, FixedLatitude{-89000000}},
                                          {FixedLongitude{179000000}, FixedLatitude{89000000}}});

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
