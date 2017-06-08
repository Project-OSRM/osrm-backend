#include "engine/polyline_compressor.hpp"
#include "util/coordinate.hpp"

#include <boost/test/test_case_template.hpp>
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

BOOST_AUTO_TEST_SUITE_END()
