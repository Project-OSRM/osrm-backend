#include "server/api/json_parameters_parser.hpp"
#include "server/api/parameters_parser.hpp"

#include "parameters_io.hpp"

#include "engine/api/match_parameters.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"

#include "engine/approach.hpp"
#include "engine/hint.hpp"

#include "util/debug.hpp"

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <limits>
#include <string>

#define CHECK_EQUAL_RANGE(R1, R2)                                                                  \
    BOOST_CHECK_EQUAL_COLLECTIONS((R1).begin(), (R1).end(), (R2).begin(), (R2).end());

BOOST_AUTO_TEST_SUITE(api_json_parameters_parser)

using namespace osrm;
using namespace osrm::server;
using namespace osrm::server::api;
using namespace osrm::engine::api;

// A minimal well-formed request the option under test can be appended to.
const std::string two_coordinates = "\"coordinates\": [[1,2],[3,4]]";

std::string bodyWith(const std::string &member)
{ return "{" + two_coordinates + ", " + member + "}"; }

// Asserts that `body` is rejected and that a diagnostic is reported back to the caller.
template <typename ParametersT> void checkRejected(const std::string &body)
{
    std::string error;
    auto result = parseJSONParameters<ParametersT>(body, error);
    BOOST_CHECK_MESSAGE(!result, "expected to be rejected: " + body);
    BOOST_CHECK_MESSAGE(!error.empty(), "expected an error message for: " + body);
}

// Asserts that `body` is accepted and returns the parsed parameters.
template <typename ParametersT> ParametersT checkAccepted(const std::string &body)
{
    std::string error;
    auto result = parseJSONParameters<ParametersT>(body, error);
    BOOST_REQUIRE_MESSAGE(result, error + " for: " + body);
    return *result;
}

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

BOOST_AUTO_TEST_CASE(valid_nearest_json)
{
    std::string error;
    auto result = parseJSONParameters<NearestParameters>(
        "{\"coordinates\": [[1.1,2.2],[3.3,4.4]], \"number\": 3}", error);
    BOOST_REQUIRE_MESSAGE(result, error);

    BOOST_CHECK_EQUAL(result->coordinates.size(), 2);
    BOOST_CHECK_EQUAL(result->number_of_results, 3u);
    BOOST_CHECK(result->IsValid());
}

BOOST_AUTO_TEST_CASE(nearest_number_defaults_when_absent)
{
    // Omitting "number" must leave number_of_results at its default, exactly as omitting
    // number= does for the URL grammar.
    auto json = checkAccepted<NearestParameters>("{\"coordinates\": [[1.1,2.2]]}");
    auto url = parseParameters<NearestParameters>(std::string{"1.1,2.2"});
    BOOST_REQUIRE(url);

    BOOST_CHECK_EQUAL(json.number_of_results, url->number_of_results);
}

BOOST_AUTO_TEST_CASE(nearest_json_matches_url_parsing)
{
    auto url = parseParameters<NearestParameters>(std::string{"1.1,2.2;3.3,4.4?number=5"});
    BOOST_REQUIRE(url);

    std::string error;
    auto json = parseJSONParameters<NearestParameters>(
        "{\"coordinates\": [[1.1,2.2],[3.3,4.4]], \"number\": 5}", error);
    BOOST_REQUIRE_MESSAGE(json, error);

    BOOST_CHECK(*json == *url);
}

BOOST_AUTO_TEST_CASE(nearest_single_coordinate_still_valid)
{
    // Backward compatibility: a single-coordinate request remains valid, unchanged from the
    // pre-batch behaviour.
    auto params = checkAccepted<NearestParameters>("{\"coordinates\": [[1.1,2.2]], \"number\": 1}");
    BOOST_CHECK_EQUAL(params.coordinates.size(), 1);
    BOOST_CHECK(params.IsValid());
}

BOOST_AUTO_TEST_CASE(nearest_rejects_wrong_type)
{
    checkRejected<NearestParameters>(bodyWith("\"number\": \"three\""));
    checkRejected<NearestParameters>(bodyWith("\"number\": -1"));
    checkRejected<NearestParameters>(bodyWith("\"number\": 1.5"));
}

BOOST_AUTO_TEST_CASE(nearest_rejects_malformed_bodies)
{
    checkRejected<NearestParameters>("{");
    checkRejected<NearestParameters>("{\"coordinates\": 5}");
    checkRejected<NearestParameters>(bodyWith("\"bearings\": 5"));
}

BOOST_AUTO_TEST_CASE(nearest_accepts_per_coordinate_options)
{
    // bearings/radiuses/hints/approaches are inherited from BaseParameters and, per the
    // batch-lookup design, must be accepted once more than one coordinate is present.
    auto params = checkAccepted<NearestParameters>(
        bodyWith("\"bearings\": [null,[10,20]], \"radiuses\": [null,\"unlimited\"], "
                 "\"approaches\": [null,\"curb\"], \"number\": 2"));

    BOOST_REQUIRE_EQUAL(params.bearings.size(), 2u);
    BOOST_CHECK(!params.bearings[0]);
    BOOST_REQUIRE(params.bearings[1]);
    BOOST_CHECK_EQUAL(params.bearings[1]->bearing, 10);

    BOOST_REQUIRE_EQUAL(params.radiuses.size(), 2u);
    BOOST_CHECK(params.radiuses[1] == std::numeric_limits<double>::infinity());

    BOOST_REQUIRE_EQUAL(params.approaches.size(), 2u);
    BOOST_CHECK(params.approaches[1] == engine::Approach::CURB);

    BOOST_CHECK(params.IsValid());
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

BOOST_AUTO_TEST_CASE(body_must_be_a_json_object)
{
    checkRejected<RouteParameters>("[[1,2],[3,4]]");
    checkRejected<RouteParameters>("42");
}

BOOST_AUTO_TEST_CASE(coordinate_forms)
{
    // polyline6(...) decodes with the higher precision, just like the URL grammar does.
    const std::string polyline6 = "polyline6(_ibE_seK_seK_seK)";
    auto json = checkAccepted<RouteParameters>("{\"coordinates\": \"" + polyline6 + "\"}");
    auto url = parseParameters<RouteParameters>(std::string{polyline6});
    BOOST_REQUIRE(url);
    CHECK_EQUAL_RANGE(json.coordinates, url->coordinates);

    // A string that is not a polyline literal, a non-array, and a value that does not fit the
    // fixed-point representation are all rejected.
    checkRejected<RouteParameters>("{\"coordinates\": \"1,2;3,4\"}");
    checkRejected<RouteParameters>("{\"coordinates\": 5}");
    checkRejected<RouteParameters>("{\"coordinates\": [[1e18,1],[3,4]]}");
}

BOOST_AUTO_TEST_CASE(base_parameters_reject_wrong_types)
{
    // The member itself has the wrong type.
    checkRejected<RouteParameters>(bodyWith("\"bearings\": 5"));
    checkRejected<RouteParameters>(bodyWith("\"radiuses\": 5"));
    checkRejected<RouteParameters>(bodyWith("\"hints\": 5"));
    checkRejected<RouteParameters>(bodyWith("\"approaches\": 5"));
    checkRejected<RouteParameters>(bodyWith("\"exclude\": \"toll\""));
    checkRejected<RouteParameters>(bodyWith("\"generate_hints\": \"yes\""));
    checkRejected<RouteParameters>(bodyWith("\"skip_waypoints\": \"yes\""));
    checkRejected<RouteParameters>(bodyWith("\"snapping\": 5"));
    checkRejected<RouteParameters>(bodyWith("\"format\": 5"));
    checkRejected<RouteParameters>(bodyWith("\"waypoints\": 5"));

    // The member is an array, but one of its entries has the wrong type.
    checkRejected<RouteParameters>(bodyWith("\"bearings\": [[10]]"));
    checkRejected<RouteParameters>(bodyWith("\"radiuses\": [\"far\"]"));
    checkRejected<RouteParameters>(bodyWith("\"hints\": [5]"));
    checkRejected<RouteParameters>(bodyWith("\"approaches\": [5]"));
    checkRejected<RouteParameters>(bodyWith("\"exclude\": [5]"));
    checkRejected<RouteParameters>(bodyWith("\"waypoints\": [-1]"));
}

BOOST_AUTO_TEST_CASE(base_parameters_reject_unknown_enum_values)
{
    checkRejected<RouteParameters>(bodyWith("\"approaches\": [\"sideways\"]"));
    checkRejected<RouteParameters>(bodyWith("\"snapping\": \"sometimes\""));
    checkRejected<RouteParameters>(bodyWith("\"format\": \"xml\""));
}

BOOST_AUTO_TEST_CASE(base_parameters_accept_per_coordinate_nulls)
{
    // null (and, for hints, the empty string) marks "no value for this coordinate", exactly as
    // an empty element does in the semicolon-separated URL encoding.
    auto params = checkAccepted<RouteParameters>(
        bodyWith("\"bearings\": [null,[10,20]], \"radiuses\": [null,\"unlimited\"], "
                 "\"hints\": [null,\"\"], \"approaches\": [null,\"curb\"], "
                 "\"exclude\": [\"toll\"], \"generate_hints\": false, "
                 "\"skip_waypoints\": true, \"snapping\": \"default\", \"format\": \"json\""));

    BOOST_REQUIRE_EQUAL(params.bearings.size(), 2u);
    BOOST_CHECK(!params.bearings[0]);
    BOOST_REQUIRE(params.bearings[1]);
    BOOST_CHECK_EQUAL(params.bearings[1]->bearing, 10);
    BOOST_CHECK_EQUAL(params.bearings[1]->range, 20);

    BOOST_REQUIRE_EQUAL(params.radiuses.size(), 2u);
    BOOST_CHECK(!params.radiuses[0]);
    BOOST_CHECK(params.radiuses[1] == std::numeric_limits<double>::infinity());

    // Hints are deprecated and no longer decoded, but they still have to be counted so that
    // IsValid() sees one entry per coordinate.
    BOOST_CHECK_EQUAL(params.hints.size(), 2u);

    BOOST_REQUIRE_EQUAL(params.approaches.size(), 2u);
    BOOST_CHECK(!params.approaches[0]);
    BOOST_CHECK(params.approaches[1] == engine::Approach::CURB);

    BOOST_REQUIRE_EQUAL(params.exclude.size(), 1u);
    BOOST_CHECK_EQUAL(params.exclude[0], "toll");
    BOOST_CHECK_EQUAL(params.generate_hints, false);
    BOOST_CHECK_EQUAL(params.skip_waypoints, true);
    BOOST_CHECK(params.snapping == RouteParameters::SnappingType::Default);
    BOOST_CHECK(params.format == RouteParameters::OutputFormatType::JSON);
    BOOST_CHECK(params.IsValid());
}

BOOST_AUTO_TEST_CASE(route_options_reject_wrong_types_and_values)
{
    checkRejected<RouteParameters>(bodyWith("\"alternatives\": \"many\""));
    checkRejected<RouteParameters>(bodyWith("\"continue_straight\": \"maybe\""));
    checkRejected<RouteParameters>(bodyWith("\"geometries\": 5"));
    checkRejected<RouteParameters>(bodyWith("\"geometries\": \"wkt\""));
    checkRejected<RouteParameters>(bodyWith("\"overview\": 5"));
    checkRejected<RouteParameters>(bodyWith("\"overview\": \"some\""));
    checkRejected<RouteParameters>(bodyWith("\"annotations\": 5"));
    checkRejected<RouteParameters>(bodyWith("\"annotations\": [5]"));
    checkRejected<RouteParameters>(bodyWith("\"annotations\": [\"altitude\"]"));
}

BOOST_AUTO_TEST_CASE(route_options_remaining_values)
{
    // "default" is accepted and, like the URL grammar, leaves continue_straight unset.
    auto params = checkAccepted<RouteParameters>(
        bodyWith("\"continue_straight\": \"default\", \"overview\": \"by_legs\", "
                 "\"geometries\": \"polyline6\", \"annotations\": false, \"waypoints\": [0,1]"));
    BOOST_CHECK(!params.continue_straight);
    BOOST_CHECK(params.overview == RouteParameters::OverviewType::ByLegs);
    BOOST_CHECK(params.geometries == RouteParameters::GeometriesType::Polyline6);
    BOOST_CHECK(!params.annotations);
    BOOST_CHECK(params.annotations_type == RouteParameters::AnnotationsType::None);
    BOOST_REQUIRE_EQUAL(params.waypoints.size(), 2u);
    BOOST_CHECK_EQUAL(params.waypoints[0], 0u);
    BOOST_CHECK_EQUAL(params.waypoints[1], 1u);
}

BOOST_AUTO_TEST_CASE(match_rejects_malformed_bodies)
{
    checkRejected<MatchParameters>("{");
    checkRejected<MatchParameters>("{\"coordinates\": 5}");
    checkRejected<MatchParameters>(bodyWith("\"gaps\": 5"));

    auto params = checkAccepted<MatchParameters>(bodyWith("\"gaps\": \"split\""));
    BOOST_CHECK(params.gaps == MatchParameters::GapsType::Split);
}

BOOST_AUTO_TEST_CASE(table_rejects_malformed_bodies)
{
    checkRejected<TableParameters>("{");
    checkRejected<TableParameters>("{\"coordinates\": 5}");
    checkRejected<TableParameters>(bodyWith("\"sources\": 5"));
    checkRejected<TableParameters>(bodyWith("\"destinations\": 5"));
    checkRejected<TableParameters>(bodyWith("\"annotations\": 5"));
    checkRejected<TableParameters>(bodyWith("\"annotations\": [5]"));
    // /table only annotates durations and distances.
    checkRejected<TableParameters>(bodyWith("\"annotations\": [\"speed\"]"));
    checkRejected<TableParameters>(bodyWith("\"fallback_speed\": \"fast\""));
    checkRejected<TableParameters>(bodyWith("\"fallback_coordinate\": 5"));
    checkRejected<TableParameters>(bodyWith("\"fallback_coordinate\": \"elsewhere\""));
    checkRejected<TableParameters>(bodyWith("\"scale_factor\": \"big\""));
}

BOOST_AUTO_TEST_CASE(table_boolean_annotations_and_fallback_coordinate)
{
    auto all = checkAccepted<TableParameters>(
        bodyWith("\"annotations\": true, \"fallback_coordinate\": \"input\""));
    BOOST_CHECK(all.annotations == TableParameters::AnnotationsType::All);
    BOOST_CHECK(all.fallback_coordinate_type == TableParameters::FallbackCoordinateType::Input);

    auto none = checkAccepted<TableParameters>(bodyWith("\"annotations\": false"));
    BOOST_CHECK(none.annotations == TableParameters::AnnotationsType::None);
}

BOOST_AUTO_TEST_CASE(deeply_nested_json_does_not_overflow_the_stack)
{
    // RapidJSON's default recursive-descent parser overflows the stack on a body such as
    // "[[[[..." at a depth well inside the configured body-size limit, taking the whole
    // worker down. The parser must reject such input instead of crashing.
    for (const std::size_t depth : {1000u, 100000u, 1000000u})
    {
        std::string body = "{\"coordinates\": ";
        body.append(depth, '[');
        // Leave the arrays unclosed: the point is that parsing terminates without recursing
        // once per level, not that the document is well-formed.
        body += "}";

        checkRejected<RouteParameters>(body);
    }

    // A fully-closed but pathologically deep array is also parsed and freed without recursion.
    std::string closed = "{\"coordinates\": ";
    closed.append(100000, '[');
    closed.append(100000, ']');
    closed += "}";
    checkRejected<RouteParameters>(closed);
}

BOOST_AUTO_TEST_CASE(out_of_range_bearings_are_rejected_not_truncated)
{
    // A plain static_cast<short> would wrap 65536 to 0 and 65890 to 354 — both in range for
    // Bearing::IsValid — so a request the URL grammar rejects (x3::short_ overflows) would be
    // silently accepted with a different meaning. The parser must reject it up front.
    checkRejected<RouteParameters>(bodyWith("\"bearings\": [[65536,65536],[0,180]]"));
    checkRejected<RouteParameters>(bodyWith("\"bearings\": [[65890,65546],[0,180]]"));
    checkRejected<RouteParameters>(bodyWith("\"bearings\": [[-40000,0],[0,180]]"));

    // A value that fits in a short but is out of the valid bearing domain still parses (as in
    // the URL API) and is caught by IsValid().
    auto params = checkAccepted<RouteParameters>(bodyWith("\"bearings\": [[400,0],[0,180]]"));
    BOOST_CHECK(!params.IsValid());
}

BOOST_AUTO_TEST_CASE(requests_with_hints_compare_equal)
{
    // BaseParameters::operator== is defaulted, so equality of two requests compares the hint
    // vectors element-wise. Make sure that comparison is exercised on a non-empty vector.
    const std::string hint(engine::ENCODED_SEGMENT_HINT_SIZE, 'A');
    const std::string query = "1,2;3,4?hints=" + hint + ";" + hint;

    auto lhs = parseParameters<RouteParameters>(std::string{query});
    auto rhs = parseParameters<RouteParameters>(std::string{query});
    BOOST_REQUIRE(lhs);
    BOOST_REQUIRE(rhs);
    BOOST_REQUIRE_EQUAL(lhs->hints.size(), 2u);
    BOOST_CHECK(*lhs == *rhs);
}

BOOST_AUTO_TEST_SUITE_END()
