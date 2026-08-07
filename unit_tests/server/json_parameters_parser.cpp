#include "server/api/json_parameters_parser.hpp"
#include "server/api/parameters_parser.hpp"

#include "parameters_io.hpp"

#include "engine/api/match_parameters.hpp"
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

BOOST_AUTO_TEST_CASE(valid_match_json)
{
    std::string error;
    auto result = parseJSONParameters<MatchParameters>(
        "{\"coordinates\": [[1.1,2.2],[3.3,4.4],[5.5,6.6]], "
        "\"timestamps\": [1424684612,1424684616,1424684620], \"gaps\": \"ignore\", "
        "\"tidy\": true, \"steps\": true, \"annotations\": [\"duration\"]}",
        error);
    BOOST_REQUIRE_MESSAGE(result, error);

    BOOST_CHECK_EQUAL(result->coordinates.size(), 3);
    BOOST_REQUIRE_EQUAL(result->timestamps.size(), 3);
    BOOST_CHECK_EQUAL(result->timestamps[0], 1424684612u);
    BOOST_CHECK_EQUAL(result->timestamps[2], 1424684620u);
    BOOST_CHECK(result->gaps == MatchParameters::GapsType::Ignore);
    BOOST_CHECK_EQUAL(result->tidy, true);
    // Options inherited from RouteParameters are parsed the same way as for /route.
    BOOST_CHECK_EQUAL(result->steps, true);
    BOOST_CHECK(result->annotations_type == MatchParameters::AnnotationsType::Duration);
    BOOST_CHECK(result->IsValid());
}

BOOST_AUTO_TEST_CASE(match_json_defaults_match_url_defaults)
{
    // An otherwise empty body must leave every match-specific default untouched.
    std::string error;
    auto json =
        parseJSONParameters<MatchParameters>("{\"coordinates\": [[1.1,2.2],[3.3,4.4]]}", error);
    BOOST_REQUIRE_MESSAGE(json, error);

    auto url = parseParameters<MatchParameters>(std::string{"1.1,2.2;3.3,4.4"});
    BOOST_REQUIRE(url);

    BOOST_CHECK(*json == *url);
}

BOOST_AUTO_TEST_CASE(match_json_matches_url_parsing)
{
    auto url = parseParameters<MatchParameters>(
        std::string{"1.1,2.2;3.3,4.4?timestamps=1;2&gaps=ignore&tidy=true&overview=false"});
    BOOST_REQUIRE(url);

    std::string error;
    auto json = parseJSONParameters<MatchParameters>(
        "{\"coordinates\": [[1.1,2.2],[3.3,4.4]], \"timestamps\": [1,2], "
        "\"gaps\": \"ignore\", \"tidy\": true, \"overview\": \"false\"}",
        error);
    BOOST_REQUIRE_MESSAGE(json, error);

    BOOST_CHECK(*json == *url);

    // Guard against comparing only the RouteParameters base: flipping a match-specific option
    // must make the two requests compare unequal.
    auto different = *json;
    different.tidy = !different.tidy;
    BOOST_CHECK(!(different == *url));
}

BOOST_AUTO_TEST_CASE(invalid_match_values)
{
    std::string error;
    auto result = parseJSONParameters<MatchParameters>(
        "{\"coordinates\": [[1,2],[3,4]], \"gaps\": \"sometimes\"}", error);
    BOOST_CHECK(!result);
    BOOST_CHECK(!error.empty());

    error.clear();
    result = parseJSONParameters<MatchParameters>(
        "{\"coordinates\": [[1,2],[3,4]], \"tidy\": \"yes\"}", error);
    BOOST_CHECK(!result);
    BOOST_CHECK(!error.empty());

    error.clear();
    result = parseJSONParameters<MatchParameters>(
        "{\"coordinates\": [[1,2],[3,4]], \"timestamps\": [-1,2]}", error);
    BOOST_CHECK(!result);
    BOOST_CHECK(!error.empty());

    error.clear();
    result = parseJSONParameters<MatchParameters>(
        "{\"coordinates\": [[1,2],[3,4]], \"timestamps\": 5}", error);
    BOOST_CHECK(!result);
    BOOST_CHECK(!error.empty());
}

BOOST_AUTO_TEST_CASE(match_timestamps_size_mismatch)
{
    std::string error;
    // One timestamp per coordinate is required: this parses but is not a valid match request.
    auto result = parseJSONParameters<MatchParameters>(
        "{\"coordinates\": [[1,2],[3,4]], \"timestamps\": [1,2,3]}", error);
    BOOST_REQUIRE_MESSAGE(result, error);
    BOOST_CHECK(!result->IsValid());
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
