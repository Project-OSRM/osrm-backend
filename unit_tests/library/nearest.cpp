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
    json_result = std::get<osrm::json::Object>(result);
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

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_CHECK(!waypoints.empty()); // the dataset has at least one nearest coordinate

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = std::get<json::Object>(waypoint);
        const auto distance = std::get<json::Number>(waypoint_object.values.at("distance")).value;
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

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    BOOST_CHECK(!json_result.values.contains("waypoints"));
}
BOOST_AUTO_TEST_CASE(test_nearest_response_skip_waypoints_old_api)
{ test_nearest_response_skip_waypoints(true); }
BOOST_AUTO_TEST_CASE(test_nearest_response_skip_waypoints_new_api)
{ test_nearest_response_skip_waypoints(false); }

void test_nearest_response_no_coordinates(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    NearestParameters params;

    json::Object json_result;
    const auto rc = run_nearest_json(osrm, params, json_result, use_json_only_api);
    BOOST_REQUIRE(rc == Status::Error);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "InvalidOptions");
}
BOOST_AUTO_TEST_CASE(test_nearest_response_no_coordinates_old_api)
{ test_nearest_response_no_coordinates(true); }
BOOST_AUTO_TEST_CASE(test_nearest_response_no_coordinates_new_api)
{ test_nearest_response_no_coordinates(false); }
void test_nearest_response_multiple_coordinates(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;
    NearestParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    json::Object json_result;
    const auto rc = run_nearest_json(osrm, params, json_result, use_json_only_api);
    BOOST_REQUIRE(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &groups = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_REQUIRE_EQUAL(groups.size(), 2);

    for (const auto &group : groups)
    {
        const auto &waypoints = std::get<json::Array>(group).values;
        BOOST_CHECK(!waypoints.empty());
        for (const auto &waypoint : waypoints)
        {
            const auto &waypoint_object = std::get<json::Object>(waypoint);
            const auto distance =
                std::get<json::Number>(waypoint_object.values.at("distance")).value;
            BOOST_CHECK(distance >= 0);
        }
    }
}
BOOST_AUTO_TEST_CASE(test_nearest_response_multiple_coordinates_old_api)
{ test_nearest_response_multiple_coordinates(true); }
BOOST_AUTO_TEST_CASE(test_nearest_response_multiple_coordinates_new_api)
{ test_nearest_response_multiple_coordinates(false); }

void test_nearest_response_all_unmatched_coordinates(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;
    NearestParameters params;
    params.coordinates.push_back(get_unmatched_location());
    params.coordinates.push_back(get_unmatched_location());
    params.coordinates.push_back(get_unmatched_location());
    params.radiuses.push_back(1.0);
    params.radiuses.push_back(1.0);
    params.radiuses.push_back(1.0);

    json::Object json_result;

    const auto rc = run_nearest_json(osrm, params, json_result, use_json_only_api);
    BOOST_REQUIRE(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &groups = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_REQUIRE_EQUAL(groups.size(), 3);

    for (const auto &group : groups)
    {
        const auto &unmatched_slot = std::get<json::Object>(group);
        const auto slot_code = std::get<json::String>(unmatched_slot.values.at("code")).value;
        BOOST_CHECK_EQUAL(slot_code, "NoSegment");
        const auto slot_message = std::get<json::String>(unmatched_slot.values.at("message")).value;
        BOOST_CHECK_EQUAL(slot_message, "Could not find a matching segment for coordinate");
    }
}
BOOST_AUTO_TEST_CASE(test_nearest_response_all_unmatched_coordinates_old_api)
{ test_nearest_response_all_unmatched_coordinates(true); }
BOOST_AUTO_TEST_CASE(test_nearest_response_all_unmatched_coordinates_new_api)
{ test_nearest_response_all_unmatched_coordinates(false); }

void test_nearest_response_one_unmatched_coordinate(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    NearestParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_unmatched_location());
    params.radiuses.push_back(std::nullopt);
    params.radiuses.push_back(1.0);

    json::Object json_result;
    const auto rc = run_nearest_json(osrm, params, json_result, use_json_only_api);

    BOOST_REQUIRE(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &groups = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_REQUIRE_EQUAL(groups.size(), 2);

    const auto &matched_group = std::get<json::Array>(groups[0]).values;
    BOOST_CHECK(!matched_group.empty());
    for (const auto &waypoint : matched_group)
    {
        const auto &waypoint_object = std::get<json::Object>(waypoint);
        const auto distance = std::get<json::Number>(waypoint_object.values.at("distance")).value;
        BOOST_CHECK(distance >= 0);
    }

    const auto &unmatched_slot = std::get<json::Object>(groups[1]);
    const auto slot_code = std::get<json::String>(unmatched_slot.values.at("code")).value;
    BOOST_CHECK_EQUAL(slot_code, "NoSegment");
}
BOOST_AUTO_TEST_CASE(test_nearest_response_one_unmatched_coordinate_old_api)
{ test_nearest_response_one_unmatched_coordinate(true); }
BOOST_AUTO_TEST_CASE(test_nearest_response_one_unmatched_coordinate_new_api)
{ test_nearest_response_one_unmatched_coordinate(false); }

void nearest_response_too_many_coordinates(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    using namespace osrm;

    NearestParameters params;
    for (int i = 0; i < 11; ++i)
        params.coordinates.push_back(get_dummy_location());

    json::Object json_result;
    const auto rc = run_nearest_json(osrm, params, json_result, use_json_only_api);
    BOOST_REQUIRE(rc == Status::Error);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "TooBig");
}
BOOST_AUTO_TEST_CASE(test_nearest_response_too_many_coordinates_old_api)
{ nearest_response_too_many_coordinates(true); }
BOOST_AUTO_TEST_CASE(test_nearest_response_too_many_coordinates_new_api)
{ nearest_response_too_many_coordinates(false); }

void test_nearest_response_per_coordinate_options(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    using namespace osrm;

    NearestParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    params.bearings.push_back(engine::Bearing{0, 180});
    params.bearings.push_back(std::nullopt);

    params.radiuses.push_back(100.0);
    params.radiuses.push_back(std::nullopt);

    json::Object json_result;
    const auto rc = run_nearest_json(osrm, params, json_result, use_json_only_api);
    BOOST_REQUIRE(rc == Status::Ok);

    const auto &groups = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_REQUIRE_EQUAL(groups.size(), 2);

    for (const auto &group : groups)
    {
        const auto &waypoints = std::get<json::Array>(group).values;
        BOOST_CHECK(!waypoints.empty());
    }
}
BOOST_AUTO_TEST_CASE(test_nearest_response_per_coordinate_options_old_api)
{ test_nearest_response_per_coordinate_options(true); }
BOOST_AUTO_TEST_CASE(test_nearest_response_per_coordinate_options_new_api)
{ test_nearest_response_per_coordinate_options(false); }

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

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_CHECK(!waypoints.empty());

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = std::get<json::Object>(waypoint);

        // Everything within ~20m (actually more) is still in small component.
        // Nearest service should snap to road network without considering components.
        const auto distance = std::get<json::Number>(waypoint_object.values.at("distance")).value;
        BOOST_CHECK_LT(distance, 20);

        const auto &nodes = std::get<json::Array>(waypoint_object.values.at("nodes")).values;
        BOOST_CHECK(nodes.size() == 2);
        BOOST_CHECK(std::get<util::json::Number>(nodes[0]).value != 0);
        BOOST_CHECK(std::get<util::json::Number>(nodes[1]).value != 0);
    }
}
BOOST_AUTO_TEST_CASE(test_nearest_response_for_location_in_small_component_old_api)
{ test_nearest_response_for_location_in_small_component(true); }
BOOST_AUTO_TEST_CASE(test_nearest_response_for_location_in_small_component_new_api)
{ test_nearest_response_for_location_in_small_component(false); }

BOOST_AUTO_TEST_CASE(test_nearest_fb_serialization)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    NearestParameters params;
    params.coordinates.push_back(get_dummy_location());

    engine::api::ResultT result = flatbuffers::FlatBufferBuilder();
    const auto rc = osrm.Nearest(params, result);
    BOOST_REQUIRE(rc == Status::Ok);

    auto &fb_result = std::get<flatbuffers::FlatBufferBuilder>(result);
    auto fb = engine::api::fbresult::GetFBResult(fb_result.GetBufferPointer());
    BOOST_CHECK(!fb->error());

    BOOST_CHECK(fb->waypoints() != nullptr);
    auto waypoints = fb->waypoints();
    BOOST_CHECK(waypoints->size() > 0); // the dataset has at least one nearest coordinate

    for (const auto waypoint : *waypoints)
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

    auto &fb_result = std::get<flatbuffers::FlatBufferBuilder>(result);
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

    auto &fb_result = std::get<flatbuffers::FlatBufferBuilder>(result);
    auto fb = engine::api::fbresult::GetFBResult(fb_result.GetBufferPointer());
    BOOST_CHECK(fb->error());
    BOOST_CHECK_EQUAL(fb->code()->code()->str(), "InvalidOptions");
}

BOOST_AUTO_TEST_SUITE_END()
