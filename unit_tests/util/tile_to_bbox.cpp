#include "util/tile_bbox.hpp"
#include "util/rectangle.hpp"

#include <boost/functional/hash.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <iostream>

BOOST_AUTO_TEST_SUITE(tile_to_bbox_test)

using namespace osrm;
using namespace osrm::util;

// Check that this osm tile to coordinate bbox converter works
struct R
{
bool operator()(const RectangleInt2D lhs, const RectangleInt2D rhs)
{
    return std::tie(lhs.min_lon, lhs.max_lon, lhs.min_lat, lhs.max_lat) == std::tie(rhs.min_lon, rhs.max_lon, rhs.min_lat, rhs.max_lat);
}
};

BOOST_AUTO_TEST_CASE(tile_to_bbox_test)
{
    R equal;
    RectangleInt2D expected(-180000000,0,85051128,0);
    BOOST_CHECK_EQUAL(true, equal(expected, TileToBBOX(1,0,0)));

    expected = RectangleInt2D(13051757,13095703,52402418,52375599);
    BOOST_CHECK_EQUAL(true, equal(expected, TileToBBOX(13,4393,2691)));
}

BOOST_AUTO_TEST_SUITE_END()
