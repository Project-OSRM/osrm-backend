#include "server/api/json_parameters_parser.hpp"
#include "server/api/parameters_parser.hpp"

#include "parameters_io.hpp"

#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"

#include "util/debug.hpp"

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <string>

#define CHECK_EQUAL_RANGE(R1, R2)                                                                  \
    BOOST_CHECK_EQUAL_COLLECTIONS((R1).begin(), (R1).end(), (R2).begin(), (R2).end());

BOOST_AUTO_TEST_SUITE(api_json_parameters_parser)

using namespace osrm;
using namespace osrm::server;
using namespace osrm::server::api;
using namespace osrm::engine::api;

BOOST_AUTO_TEST_CASE(invalid_json_returns_error)
{
    std::string error;
    auto result = parseJSONParameters<RouteParameters>("{not valid json", error);
    BOOST_CHECK(!result);
    BOOST_CHECK(!error.empty());
}

BOOST_AUTO_TEST_CASE(missing_coordinates_returns_error)
{
    std::string error;
    auto result = parseJSONParameters<RouteParameters>("{\"steps\": true}", error);
    BOOST_CHECK(!result);
    BOOST_CHECK(!error.empty());
}

BOOST_AUTO_TEST_CASE(wrong_type_returns_error)
{
    std::string error;
    // steps must be a boolean
    auto result = parseJSONParameters<RouteParameters>(
        "{\"coordinates\": [[1,2],[3,4]], \"steps\": \"yes\"}", error);
    BOOST_CHECK(!result);
    BOOST_CHECK(!error.empty());

    // coordinate elements must be numbers
    error.clear();
    result = parseJSONParameters<RouteParameters>("{\"coordinates\": [[\"a\",\"b\"]]}", error);
    BOOST_CHECK(!result);
    BOOST_CHECK(!error.empty());
}

BOOST_AUTO_TEST_CASE(valid_route_json)
{
    std::string error;
    auto result = parseJSONParameters<RouteParameters>(
        "{\"coordinates\": [[1.1,2.2],[3.3,4.4]], \"steps\": true, "
        "\"annotations\": [\"duration\",\"distance\"], \"geometries\": \"geojson\", "
        "\"overview\": \"full\", \"alternatives\": 2}",
        error);
    BOOST_REQUIRE_MESSAGE(result, error);

    BOOST_CHECK_EQUAL(result->coordinates.size(), 2);
    BOOST_CHECK_EQUAL(result->steps, true);
    BOOST_CHECK(result->annotations);
    BOOST_CHECK(result->annotations_type == (RouteParameters::AnnotationsType::Duration |
                                             RouteParameters::AnnotationsType::Distance));
    BOOST_CHECK(result->geometries == RouteParameters::GeometriesType::GeoJSON);
    BOOST_CHECK(result->overview == RouteParameters::OverviewType::Full);
    BOOST_CHECK_EQUAL(result->alternatives, true);
    BOOST_CHECK_EQUAL(result->number_of_alternatives, 2u);
}

BOOST_AUTO_TEST_CASE(valid_table_json)
{
    std::string error;
    auto result = parseJSONParameters<TableParameters>(
        "{\"coordinates\": [[1,2],[3,4],[5,6]], \"sources\": [0], "
        "\"destinations\": [1,2], \"annotations\": [\"duration\",\"distance\"], "
        "\"fallback_speed\": 5.0, \"fallback_coordinate\": \"snapped\", \"scale_factor\": 2.0}",
        error);
    BOOST_REQUIRE_MESSAGE(result, error);

    BOOST_CHECK_EQUAL(result->coordinates.size(), 3);
    BOOST_REQUIRE_EQUAL(result->sources.size(), 1);
    BOOST_CHECK_EQUAL(result->sources[0], 0);
    BOOST_REQUIRE_EQUAL(result->destinations.size(), 2);
    BOOST_CHECK_EQUAL(result->destinations[1], 2);
    BOOST_CHECK(result->annotations == TableParameters::AnnotationsType::All);
    BOOST_CHECK_EQUAL(result->fallback_speed, 5.0);
    BOOST_CHECK(result->fallback_coordinate_type ==
                TableParameters::FallbackCoordinateType::Snapped);
    BOOST_CHECK_EQUAL(result->scale_factor, 2.0);
    BOOST_CHECK(result->IsValid());
}

BOOST_AUTO_TEST_CASE(polyline_coordinates)
{
    // "_ibE_seK_seK_seK" encodes two coordinates; compare against the URL grammar's result.
    const std::string polyline = "polyline(_ibE_seK_seK_seK)";

    std::string error;
    auto json =
        parseJSONParameters<RouteParameters>("{\"coordinates\": \"" + polyline + "\"}", error);
    BOOST_REQUIRE_MESSAGE(json, error);

    auto url = parseParameters<RouteParameters>(std::string{polyline});
    BOOST_REQUIRE(url);

    CHECK_EQUAL_RANGE(json->coordinates, url->coordinates);
}

BOOST_AUTO_TEST_CASE(json_matches_url_parsing)
{
    // The same request expressed as a URL query and as a JSON body must yield the same
    // coordinates and options.
    auto url =
        parseParameters<RouteParameters>(std::string{"1.1,2.2;3.3,4.4?steps=true&overview=false"});
    BOOST_REQUIRE(url);

    std::string error;
    auto json = parseJSONParameters<RouteParameters>(
        "{\"coordinates\": [[1.1,2.2],[3.3,4.4]], \"steps\": true, \"overview\": \"false\"}",
        error);
    BOOST_REQUIRE_MESSAGE(json, error);

    // The parameter structs are value-comparable (defaulted operator==), so the URL- and
    // JSON-parsed requests must be exactly equal.
    BOOST_CHECK(*json == *url);
}

BOOST_AUTO_TEST_CASE(invalid_options_fail_validation)
{
    std::string error;
    // Only one coordinate: parses fine but is not a valid route/table request.
    auto route = parseJSONParameters<RouteParameters>("{\"coordinates\": [[1,2]]}", error);
    BOOST_REQUIRE_MESSAGE(route, error);
    BOOST_CHECK(!route->IsValid());

    // Source index out of range.
    auto table = parseJSONParameters<TableParameters>(
        "{\"coordinates\": [[1,2],[3,4]], \"sources\": [5]}", error);
    BOOST_REQUIRE_MESSAGE(table, error);
    BOOST_CHECK(!table->IsValid());
}

BOOST_AUTO_TEST_SUITE_END()
