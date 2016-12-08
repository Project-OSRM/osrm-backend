#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "coordinates.hpp"
#include "equal_json.hpp"

#include "engine/api/json_factory.hpp"
#include "osrm/coordinate.hpp"

#include <iterator>
#include <vector>

using namespace osrm;

BOOST_AUTO_TEST_SUITE(json)

BOOST_AUTO_TEST_CASE(test_json_linestring)
{
    const std::vector<util::Coordinate> locations{get_dummy_location(),  //
                                                  get_dummy_location(),  //
                                                  get_dummy_location()}; //

    auto geom = engine::api::json::makeGeoJSONGeometry(begin(locations), end(locations));

    const auto type = geom.values["type"].get<util::json::String>().value;
    BOOST_CHECK_EQUAL(type, "LineString");

    const auto coords = geom.values["coordinates"].get<util::json::Array>().values;
    BOOST_CHECK_EQUAL(coords.size(), 3); // array of three location arrays

    for (const auto each : coords)
    {
        const auto loc = each.get<util::json::Array>().values;
        BOOST_CHECK_EQUAL(loc.size(), 2);

        const auto lon = loc[0].get<util::json::Number>().value;
        const auto lat = loc[1].get<util::json::Number>().value;

        (void)lon;
        (void)lat;
        // cast fails if type do not match
    }
}

BOOST_AUTO_TEST_CASE(test_json_single_point)
{
    const std::vector<util::Coordinate> locations{get_dummy_location()};

    auto geom = engine::api::json::makeGeoJSONGeometry(begin(locations), end(locations));

    const auto type = geom.values["type"].get<util::json::String>().value;
    BOOST_CHECK_EQUAL(type, "LineString");

    const auto coords = geom.values["coordinates"].get<util::json::Array>().values;
    BOOST_CHECK_EQUAL(coords.size(), 2); // array of two location arrays

    for (const auto each : coords)
    {
        const auto loc = each.get<util::json::Array>().values;
        BOOST_CHECK_EQUAL(loc.size(), 2);

        const auto lon = loc[0].get<util::json::Number>().value;
        const auto lat = loc[1].get<util::json::Number>().value;

        (void)lon;
        (void)lat;
        // cast fails if type do not match
    }
}

BOOST_AUTO_TEST_SUITE_END()
