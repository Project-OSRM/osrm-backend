#include <boost/test/unit_test.hpp>

#include "coordinates.hpp"
#include "fixture.hpp"

#include "engine/api/flatbuffers/fbresult_generated.h"
#include "osrm/nearest_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

osrm::Status run_nearest_json(const osrm::OSRM &osrm,
                              const osrm::NearestParameters &params,
                              osrm::json::Object &json_result,
                              bool use_json_only_api)
{
    if (use_json_only_api)
    {
        return osrm.Nearest(params, json_result);
    }
    osrm::engine::api::ResultT result = osrm::json::Object();
    auto rc = osrm.Nearest(params, result);
    json_result = result.get<osrm::json::Object>();
    return rc;
}

BOOST_AUTO_TEST_SUITE(nearest)

void test_nearest_response(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    NearestParameters params;
    params.coordinates.push_back(get_dummy_location());

    json::Object json_result;
    const auto rc = run_nearest_json(osrm, params, json_result, use_json_only_api);
    BOOST_REQUIRE(rc == Status::Ok);

    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = json_result.values.at("waypoints").get<json::Array>().values;
    BOOST_CHECK(!waypoints.empty()); // the dataset has at least one nearest coordinate

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = waypoint.get<json::Object>();
        const auto distance = waypoint_object.values.at("distance").get<json::Number>().value;
        BOOST_CHECK(distance >= 0);
    }
}
BOOST_AUTO_TEST_CASE(test_nearest_response_old_api) { test_nearest_response(true); }
BOOST_AUTO_TEST_CASE(test_nearest_response_new_api) { test_nearest_response(false); }

void test_nearest_response_skip_waypoints(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    NearestParameters params;
    params.skip_waypoints = true;
    params.coordinates.push_back(get_dummy_location());

    json::Object json_result;
    const auto rc = run_nearest_json(osrm, params, json_result, use_json_only_api);
    BOOST_REQUIRE(rc == Status::Ok);

    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    BOOST_CHECK(json_result.values.find("waypoints") == json_result.values.end());
}
BOOST_AUTO_TEST_CASE(test_nearest_response_skip_waypoints_old_api)
{
    test_nearest_response_skip_waypoints(true);
}
BOOST_AUTO_TEST_CASE(test_nearest_response_skip_waypoints_new_api)
{
    test_nearest_response_skip_waypoints(false);
}

void test_nearest_response_no_coordinates(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    NearestParameters params;

    json::Object json_result;
    const auto rc = run_nearest_json(osrm, params, json_result, use_json_only_api);
    BOOST_REQUIRE(rc == Status::Error);

    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "InvalidOptions");
}
BOOST_AUTO_TEST_CASE(test_nearest_response_no_coordinates_old_api)
{
    test_nearest_response_no_coordinates(true);
}
BOOST_AUTO_TEST_CASE(test_nearest_response_no_coordinates_new_api)
{
    test_nearest_response_no_coordinates(false);
}

void test_nearest_response_multiple_coordinates(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    NearestParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    json::Object json_result;
    const auto rc = run_nearest_json(osrm, params, json_result, use_json_only_api);
    BOOST_REQUIRE(rc == Status::Error);

    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "InvalidOptions");
}
BOOST_AUTO_TEST_CASE(test_nearest_response_multiple_coordinates_old_api)
{
    test_nearest_response_multiple_coordinates(true);
}
BOOST_AUTO_TEST_CASE(test_nearest_response_multiple_coordinates_new_api)
{
    test_nearest_response_multiple_coordinates(false);
}

void test_nearest_response_for_location_in_small_component(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    const auto locations = get_locations_in_small_component();

    NearestParameters params;
    params.coordinates.push_back(locations.at(0));
    params.number_of_results = 3;

    json::Object json_result;
    const auto rc = run_nearest_json(osrm, params, json_result, use_json_only_api);
    BOOST_REQUIRE(rc == Status::Ok);

    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = json_result.values.at("waypoints").get<json::Array>().values;
    BOOST_CHECK(!waypoints.empty());

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = waypoint.get<json::Object>();

        // Everything within ~20m (actually more) is still in small component.
        // Nearest service should snap to road network without considering components.
        const auto distance = waypoint_object.values.at("distance").get<json::Number>().value;
        BOOST_CHECK_LT(distance, 20);

        const auto &nodes = waypoint_object.values.at("nodes").get<json::Array>().values;
        BOOST_CHECK(nodes.size() == 2);
        BOOST_CHECK(nodes[0].get<util::json::Number>().value != 0);
        BOOST_CHECK(nodes[1].get<util::json::Number>().value != 0);
    }
}
BOOST_AUTO_TEST_CASE(test_nearest_response_for_location_in_small_component_old_api)
{
    test_nearest_response_for_location_in_small_component(true);
}
BOOST_AUTO_TEST_CASE(test_nearest_response_for_location_in_small_component_new_api)
{
    test_nearest_response_for_location_in_small_component(false);
}

BOOST_AUTO_TEST_CASE(test_nearest_fb_serialization)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    NearestParameters params;
    params.coordinates.push_back(get_dummy_location());

    engine::api::ResultT result = flatbuffers::FlatBufferBuilder();
    const auto rc = osrm.Nearest(params, result);
    BOOST_REQUIRE(rc == Status::Ok);

    auto &fb_result = result.get<flatbuffers::FlatBufferBuilder>();
    auto fb = engine::api::fbresult::GetFBResult(fb_result.GetBufferPointer());
    BOOST_CHECK(!fb->error());

    BOOST_CHECK(fb->waypoints() != nullptr);
    auto waypoints = fb->waypoints();
    BOOST_CHECK(waypoints->size() > 0); // the dataset has at least one nearest coordinate

    for (const auto &waypoint : *waypoints)
    {
        BOOST_CHECK(waypoint->distance() >= 0);
        BOOST_CHECK(waypoint->nodes()->first() != 0);
        BOOST_CHECK(waypoint->nodes()->second() != 0);
    }
}

BOOST_AUTO_TEST_CASE(test_nearest_fb_serialization_skip_waypoints)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    NearestParameters params;
    params.skip_waypoints = true;
    params.coordinates.push_back(get_dummy_location());

    engine::api::ResultT result = flatbuffers::FlatBufferBuilder();
    const auto rc = osrm.Nearest(params, result);
    BOOST_REQUIRE(rc == Status::Ok);

    auto &fb_result = result.get<flatbuffers::FlatBufferBuilder>();
    auto fb = engine::api::fbresult::GetFBResult(fb_result.GetBufferPointer());
    BOOST_CHECK(!fb->error());

    BOOST_CHECK(fb->waypoints() == nullptr);
}

BOOST_AUTO_TEST_CASE(test_nearest_fb_error)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    NearestParameters params;

    engine::api::ResultT result = flatbuffers::FlatBufferBuilder();
    const auto rc = osrm.Nearest(params, result);
    BOOST_REQUIRE(rc == Status::Error);

    auto &fb_result = result.get<flatbuffers::FlatBufferBuilder>();
    auto fb = engine::api::fbresult::GetFBResult(fb_result.GetBufferPointer());
    BOOST_CHECK(fb->error());
    BOOST_CHECK_EQUAL(fb->code()->code()->str(), "InvalidOptions");
}

BOOST_AUTO_TEST_SUITE_END()
