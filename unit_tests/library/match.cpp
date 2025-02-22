#include <boost/test/unit_test.hpp>
#include <variant>

#include "coordinates.hpp"
#include "fixture.hpp"
#include "waypoint_check.hpp"

#include "osrm/match_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

osrm::Status run_match_json(const osrm::OSRM &osrm,
                            const osrm::MatchParameters &params,
                            osrm::json::Object &json_result,
                            bool use_json_only_api)
{
    using namespace osrm;

    if (use_json_only_api)
    {
        return osrm.Match(params, json_result);
    }
    engine::api::ResultT result = json::Object();
    auto rc = osrm.Match(params, result);
    json_result = std::get<json::Object>(result);
    return rc;
}

BOOST_AUTO_TEST_SUITE(match)

void test_match(bool use_json_only_api)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    MatchParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    json::Object json_result;
    const auto rc = run_match_json(osrm, params, json_result, use_json_only_api);

    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &tracepoints = std::get<json::Array>(json_result.values.at("tracepoints")).values;
    BOOST_CHECK_EQUAL(tracepoints.size(), params.coordinates.size());

    const auto &matchings = std::get<json::Array>(json_result.values.at("matchings")).values;
    const auto &number_of_matchings = matchings.size();
    for (const auto &waypoint : tracepoints)
    {
        if (std::holds_alternative<util::json::Object>(waypoint))
        {
            BOOST_CHECK(waypoint_check(waypoint));
            const auto &waypoint_object = std::get<json::Object>(waypoint);
            const auto matchings_index =
                std::get<json::Number>(waypoint_object.values.at("matchings_index")).value;
            const auto waypoint_index =
                std::get<json::Number>(waypoint_object.values.at("waypoint_index")).value;
            const auto &route_legs =
                std::get<json::Array>(
                    std::get<json::Object>(matchings[matchings_index]).values.at("legs"))
                    .values;
            BOOST_CHECK_LT(waypoint_index, route_legs.size() + 1);
            BOOST_CHECK_LT(matchings_index, number_of_matchings);
        }
        else
        {
            BOOST_CHECK(std::holds_alternative<json::Null>(waypoint));
        }
    }
}
BOOST_AUTO_TEST_CASE(test_match_new_api) { test_match(false); }
BOOST_AUTO_TEST_CASE(test_match_old_api) { test_match(true); }

void test_match_skip_waypoints(bool use_json_only_api)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    MatchParameters params;
    params.skip_waypoints = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    json::Object json_result;
    const auto rc = run_match_json(osrm, params, json_result, use_json_only_api);

    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    BOOST_CHECK(json_result.values.find("tracepoints") == json_result.values.end());
}
BOOST_AUTO_TEST_CASE(test_match_skip_waypoints_old_api) { test_match_skip_waypoints(true); }
BOOST_AUTO_TEST_CASE(test_match_skip_waypoints_new_api) { test_match_skip_waypoints(false); }

void test_match_split(bool use_json_only_api)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    MatchParameters params;
    params.coordinates = get_split_trace_locations();
    params.timestamps = {1, 2, 1700, 1800};

    json::Object json_result;
    const auto rc = run_match_json(osrm, params, json_result, use_json_only_api);

    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &tracepoints = std::get<json::Array>(json_result.values.at("tracepoints")).values;
    BOOST_CHECK_EQUAL(tracepoints.size(), params.coordinates.size());

    const auto &matchings = std::get<json::Array>(json_result.values.at("matchings")).values;
    const auto &number_of_matchings = matchings.size();
    BOOST_CHECK_EQUAL(number_of_matchings, 2);
    std::size_t current_matchings_index = 0, expected_waypoint_index = 0;
    for (const auto &waypoint : tracepoints)
    {
        if (std::holds_alternative<util::json::Object>(waypoint))
        {
            BOOST_CHECK(waypoint_check(waypoint));
            const auto &waypoint_object = std::get<json::Object>(waypoint);
            const auto matchings_index =
                std::get<json::Number>(waypoint_object.values.at("matchings_index")).value;
            const auto waypoint_index =
                std::get<json::Number>(waypoint_object.values.at("waypoint_index")).value;

            BOOST_CHECK_LT(matchings_index, number_of_matchings);

            expected_waypoint_index =
                (current_matchings_index != matchings_index) ? 0 : expected_waypoint_index;
            BOOST_CHECK_EQUAL(waypoint_index, expected_waypoint_index);

            current_matchings_index = matchings_index;
            ++expected_waypoint_index;
        }
        else
        {
            BOOST_CHECK(std::holds_alternative<json::Null>(waypoint));
        }
    }
}
BOOST_AUTO_TEST_CASE(test_match_split_old_api) { test_match_split(true); }
BOOST_AUTO_TEST_CASE(test_match_split_new_api) { test_match_split(false); }

BOOST_AUTO_TEST_CASE(test_match_fb_serialization)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    MatchParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    engine::api::ResultT result = flatbuffers::FlatBufferBuilder();

    const auto rc = osrm.Match(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &fb_result = std::get<flatbuffers::FlatBufferBuilder>(result);
    auto fb = engine::api::fbresult::GetFBResult(fb_result.GetBufferPointer());

    BOOST_CHECK(!fb->error());

    BOOST_CHECK(fb->waypoints() != nullptr);
    const auto waypoints = fb->waypoints();
    BOOST_CHECK(waypoints->size() == params.coordinates.size());

    BOOST_CHECK(fb->routes() != nullptr);
    const auto matchings = fb->routes();
    const auto &number_of_matchings = matchings->size();

    for (const auto waypoint : *waypoints)
    {
        BOOST_CHECK(waypoint_check(waypoint));
        const auto matchings_index = waypoint->matchings_index();
        const auto waypoint_index = waypoint->waypoint_index();
        const auto &route_legs = matchings->operator[](matchings_index)->legs();

        BOOST_CHECK_LT(waypoint_index, route_legs->size() + 1);
        BOOST_CHECK_LT(matchings_index, number_of_matchings);
    }
}

BOOST_AUTO_TEST_CASE(test_match_fb_serialization_skip_waypoints)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    MatchParameters params;
    params.skip_waypoints = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    engine::api::ResultT result = flatbuffers::FlatBufferBuilder();

    const auto rc = osrm.Match(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &fb_result = std::get<flatbuffers::FlatBufferBuilder>(result);
    auto fb = engine::api::fbresult::GetFBResult(fb_result.GetBufferPointer());

    BOOST_CHECK(!fb->error());

    BOOST_CHECK(fb->waypoints() == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
