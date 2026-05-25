#include "osrm/coordinate.hpp"

#include <boost/test/unit_test.hpp>

#include <type_traits>

BOOST_AUTO_TEST_SUITE(coordinate_literals)

using namespace osrm;

BOOST_AUTO_TEST_CASE(lat_literal_type_and_value)
{
    static_assert(std::is_same_v<decltype(52_lat), FixedLatitude>);
    static_assert(std::is_same_v<decltype(52.125_lat), FloatLatitude>);

    BOOST_CHECK(52_lat == FixedLatitude{52});
    BOOST_CHECK(52.125_lat == FloatLatitude{52.125});
}

BOOST_AUTO_TEST_CASE(lon_literal_type_and_value)
{
    static_assert(std::is_same_v<decltype(13_lon), FixedLongitude>);
    static_assert(std::is_same_v<decltype(13.25_lon), FloatLongitude>);

    BOOST_CHECK(13_lon == FixedLongitude{13});
    BOOST_CHECK(13.25_lon == FloatLongitude{13.25});
}

BOOST_AUTO_TEST_SUITE_END()
