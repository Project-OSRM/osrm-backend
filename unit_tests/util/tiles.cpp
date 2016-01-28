#include "util/tiles.hpp"

using namespace osrm::util;

#include <boost/functional/hash.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(tiles_test)

using namespace osrm;
using namespace osrm::util;

BOOST_AUTO_TEST_CASE(point_to_tile_test)
{
    tiles::Tile tile_1_reference{2306375680,1409941503,32};
    tiles::Tile tile_2_reference{2308259840,1407668224,32};
    tiles::Tile tile_3_reference{616562688,2805989376,32};
    tiles::Tile tile_4_reference{1417674752,2084569088,32};
    tiles::Tile tile_5_reference{616562688,2805989376,32};
    tiles::Tile tile_6_reference{712654285,2671662374,32};

    auto tile_1 = tiles::pointToTile(13.31817626953125,52.449314140869696);
    auto tile_2 = tiles::pointToTile(13.476104736328125,52.56529070208021);
    auto tile_3 = tiles::pointToTile(-128.32031249999997,-48.224672649565186);
    auto tile_4 = tiles::pointToTile(-61.17187499999999,5.266007882805498);
    auto tile_5 = tiles::pointToTile(-128.32031249999997,-48.224672649565186);
    auto tile_6 = tiles::pointToTile(-120.266007882805532,-40.17187499999999);
    BOOST_CHECK_EQUAL(tile_1.x, tile_1_reference.x);
    BOOST_CHECK_EQUAL(tile_1.y, tile_1_reference.y);
    BOOST_CHECK_EQUAL(tile_1.z, tile_1_reference.z);
    BOOST_CHECK_EQUAL(tile_2.x, tile_2_reference.x);
    BOOST_CHECK_EQUAL(tile_2.y, tile_2_reference.y);
    BOOST_CHECK_EQUAL(tile_2.z, tile_2_reference.z);
    BOOST_CHECK_EQUAL(tile_3.x, tile_3_reference.x);
    BOOST_CHECK_EQUAL(tile_3.y, tile_3_reference.y);
    BOOST_CHECK_EQUAL(tile_3.z, tile_3_reference.z);
    BOOST_CHECK_EQUAL(tile_4.x, tile_4_reference.x);
    BOOST_CHECK_EQUAL(tile_4.y, tile_4_reference.y);
    BOOST_CHECK_EQUAL(tile_4.z, tile_4_reference.z);
    BOOST_CHECK_EQUAL(tile_5.x, tile_5_reference.x);
    BOOST_CHECK_EQUAL(tile_5.y, tile_5_reference.y);
    BOOST_CHECK_EQUAL(tile_5.z, tile_5_reference.z);
    BOOST_CHECK_EQUAL(tile_6.x, tile_6_reference.x);
    BOOST_CHECK_EQUAL(tile_6.y, tile_6_reference.y);
    BOOST_CHECK_EQUAL(tile_6.z, tile_6_reference.z);
}

// Verify that the bearing-bounds checking function behaves as expected
BOOST_AUTO_TEST_CASE(bounding_box_to_tile_test)
{
    tiles::Tile tile_1_reference{17, 10, 5};
    tiles::Tile tile_2_reference{0, 0, 0};
    tiles::Tile tile_3_reference{0, 2, 2};
    auto tile_1 = tiles::getBBMaxZoomTile(13.31817626953125, 52.449314140869696,
                                          13.476104736328125, 52.56529070208021);
    auto tile_2 = tiles::getBBMaxZoomTile(-128.32031249999997, -48.224672649565186,
                                          -61.17187499999999, 5.266007882805498);
    auto tile_3 = tiles::getBBMaxZoomTile(-128.32031249999997, -48.224672649565186,
                                          -120.2660078828055, -40.17187499999999);
    BOOST_CHECK_EQUAL(tile_1.x, tile_1_reference.x);
    BOOST_CHECK_EQUAL(tile_1.y, tile_1_reference.y);
    BOOST_CHECK_EQUAL(tile_1.z, tile_1_reference.z);
    BOOST_CHECK_EQUAL(tile_2.x, tile_2_reference.x);
    BOOST_CHECK_EQUAL(tile_2.y, tile_2_reference.y);
    BOOST_CHECK_EQUAL(tile_2.z, tile_2_reference.z);
    BOOST_CHECK_EQUAL(tile_3.x, tile_3_reference.x);
    BOOST_CHECK_EQUAL(tile_3.y, tile_3_reference.y);
    BOOST_CHECK_EQUAL(tile_3.z, tile_3_reference.z);
}

BOOST_AUTO_TEST_SUITE_END()
