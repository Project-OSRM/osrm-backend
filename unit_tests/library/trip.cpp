#include <boost/test/unit_test.hpp>

#include "coordinates.hpp"
#include "fixture.hpp"

#include "osrm/trip_parameters.hpp"
#include <engine/api/flatbuffers/fbresult_generated.h>

#include "osrm/coordinate.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

osrm::Status run_trip_json(const osrm::OSRM &osrm,
                           const osrm::TripParameters &params,
                           osrm::json::Object &json_result,
                           bool use_json_only_api)
{
    if (use_json_only_api)
    {
        return osrm.Trip(params, json_result);
    }
    osrm::engine::api::ResultT result = osrm::json::Object();
    auto rc = osrm.Trip(params, result);
    json_result = std::get<osrm::json::Object>(result);
    return rc;
}

BOOST_AUTO_TEST_SUITE(trip)

void test_roundtrip_response_for_locations_in_small_component(bool use_json_only_api)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    const auto locations = get_locations_in_small_component();

    TripParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    json::Object json_result;
    const auto rc = run_trip_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    const auto &trips = std::get<json::Array>(json_result.values.at("trips")).values;
    BOOST_CHECK_EQUAL(trips.size(), 1);

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = std::get<json::Object>(waypoint);

        const auto location = std::get<json::Array>(waypoint_object.values.at("location")).values;
        const auto longitude = std::get<json::Number>(location[0]).value;
        const auto latitude = std::get<json::Number>(location[1]).value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);

        const auto trip = std::get<json::Number>(waypoint_object.values.at("trips_index")).value;
        const auto pos = std::get<json::Number>(waypoint_object.values.at("waypoint_index")).value;
        BOOST_CHECK(trip >= 0 && trip < trips.size());
        BOOST_CHECK(pos >= 0 && pos < waypoints.size());
    }
}
BOOST_AUTO_TEST_CASE(test_roundtrip_response_for_locations_in_small_component_old_api)
{
    test_roundtrip_response_for_locations_in_small_component(true);
}
BOOST_AUTO_TEST_CASE(test_roundtrip_response_for_locations_in_small_component_new_api)
{
    test_roundtrip_response_for_locations_in_small_component(false);
}

void test_roundtrip_response_for_locations_in_small_component_skip_waypoints(bool use_json_only_api)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    const auto locations = get_locations_in_small_component();

    TripParameters params;
    params.skip_waypoints = true;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    json::Object json_result;
    const auto rc = run_trip_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    BOOST_CHECK(json_result.values.find("waypoints") == json_result.values.end());
}
BOOST_AUTO_TEST_CASE(
    test_roundtrip_response_for_locations_in_small_component_skip_waypoints_old_api)
{
    test_roundtrip_response_for_locations_in_small_component_skip_waypoints(true);
}
BOOST_AUTO_TEST_CASE(
    test_roundtrip_response_for_locations_in_small_component_skip_waypoints_new_api)
{
    test_roundtrip_response_for_locations_in_small_component_skip_waypoints(false);
}

void test_roundtrip_response_for_locations_in_big_component(bool use_json_only_api)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    const auto locations = get_locations_in_big_component();

    TripParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    json::Object json_result;
    const auto rc = run_trip_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    const auto &trips = std::get<json::Array>(json_result.values.at("trips")).values;
    BOOST_CHECK_EQUAL(trips.size(), 1);

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = std::get<json::Object>(waypoint);

        const auto location = std::get<json::Array>(waypoint_object.values.at("location")).values;
        const auto longitude = std::get<json::Number>(location[0]).value;
        const auto latitude = std::get<json::Number>(location[1]).value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);

        const auto trip = std::get<json::Number>(waypoint_object.values.at("trips_index")).value;
        const auto pos = std::get<json::Number>(waypoint_object.values.at("waypoint_index")).value;
        BOOST_CHECK(trip >= 0 && trip < trips.size());
        BOOST_CHECK(pos >= 0 && pos < waypoints.size());
    }
}
BOOST_AUTO_TEST_CASE(test_roundtrip_response_for_locations_in_big_component_old_api)
{
    test_roundtrip_response_for_locations_in_big_component(true);
}
BOOST_AUTO_TEST_CASE(test_roundtrip_response_for_locations_in_big_component_new_api)
{
    test_roundtrip_response_for_locations_in_big_component(false);
}

void test_roundtrip_response_for_locations_across_components(bool use_json_only_api)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    const auto small = get_locations_in_small_component();
    const auto big = get_locations_in_big_component();

    TripParameters params;
    params.coordinates.push_back(small.at(0));
    params.coordinates.push_back(big.at(0));
    params.coordinates.push_back(small.at(1));
    params.coordinates.push_back(big.at(1));

    json::Object json_result;
    const auto rc = run_trip_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    const auto &trips = std::get<json::Array>(json_result.values.at("trips")).values;
    BOOST_CHECK_EQUAL(trips.size(), 1);
    // ^ First snapping, then SCC decomposition (see plugins/trip.cpp). Therefore only a single
    // trip.

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = std::get<json::Object>(waypoint);

        const auto location = std::get<json::Array>(waypoint_object.values.at("location")).values;
        const auto longitude = std::get<json::Number>(location[0]).value;
        const auto latitude = std::get<json::Number>(location[1]).value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);

        const auto trip = std::get<json::Number>(waypoint_object.values.at("trips_index")).value;
        const auto pos = std::get<json::Number>(waypoint_object.values.at("waypoint_index")).value;
        BOOST_CHECK(trip >= 0 && trip < trips.size());
        BOOST_CHECK(pos >= 0 && pos < waypoints.size());
    }
}
BOOST_AUTO_TEST_CASE(test_roundtrip_response_for_locations_across_components_old_api)
{
    test_roundtrip_response_for_locations_across_components(true);
}
BOOST_AUTO_TEST_CASE(test_roundtrip_response_for_locations_across_components_new_api)
{
    test_roundtrip_response_for_locations_across_components(false);
}

void test_tfse_1(bool use_json_only_api)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    const auto locations = get_locations_in_small_component();

    TripParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    params.source = TripParameters::SourceType::First;
    params.destination = TripParameters::DestinationType::Last;
    params.roundtrip = false;

    json::Object json_result;
    const auto rc = run_trip_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    const auto &trips = std::get<json::Array>(json_result.values.at("trips")).values;
    BOOST_CHECK_EQUAL(trips.size(), 1);

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = std::get<json::Object>(waypoint);

        const auto location = std::get<json::Array>(waypoint_object.values.at("location")).values;
        const auto longitude = std::get<json::Number>(location[0]).value;
        const auto latitude = std::get<json::Number>(location[1]).value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);

        const auto trip = std::get<json::Number>(waypoint_object.values.at("trips_index")).value;
        const auto pos = std::get<json::Number>(waypoint_object.values.at("waypoint_index")).value;
        BOOST_CHECK(trip >= 0 && trip < trips.size());
        BOOST_CHECK(pos >= 0 && pos < waypoints.size());
    }
}
BOOST_AUTO_TEST_CASE(test_tfse_1_old_api) { test_tfse_1(true); }
BOOST_AUTO_TEST_CASE(test_tfse_1_new_api) { test_tfse_1(false); }

void test_tfse_2(bool use_json_only_api)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    const auto locations = get_locations_in_big_component();

    TripParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(2));
    params.coordinates.push_back(locations.at(1));

    params.source = TripParameters::SourceType::First;
    params.destination = TripParameters::DestinationType::Last;
    params.roundtrip = false;

    json::Object json_result;
    const auto rc = run_trip_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    const auto &trips = std::get<json::Array>(json_result.values.at("trips")).values;
    BOOST_CHECK_EQUAL(trips.size(), 1);

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = std::get<json::Object>(waypoint);

        const auto location = std::get<json::Array>(waypoint_object.values.at("location")).values;
        const auto longitude = std::get<json::Number>(location[0]).value;
        const auto latitude = std::get<json::Number>(location[1]).value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);

        const auto trip = std::get<json::Number>(waypoint_object.values.at("trips_index")).value;
        const auto pos = std::get<json::Number>(waypoint_object.values.at("waypoint_index")).value;
        BOOST_CHECK(trip >= 0 && trip < trips.size());
        BOOST_CHECK(pos >= 0 && pos < waypoints.size());
    }
}
BOOST_AUTO_TEST_CASE(test_tfse_2_old_api) { test_tfse_2(true); }
BOOST_AUTO_TEST_CASE(test_tfse_2_new_api) { test_tfse_2(false); }

void ResetParams(const Locations &locations, osrm::TripParameters &params)
{
    params = osrm::TripParameters();
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));
}
void CheckNotImplemented(const osrm::OSRM &osrm,
                         osrm::TripParameters &params,
                         bool use_json_only_api)
{
    using namespace osrm;
    json::Object json_result;
    const auto rc = run_trip_json(osrm, params, json_result, use_json_only_api);
    BOOST_REQUIRE(rc == osrm::Status::Error);
    auto code = std::get<osrm::json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "NotImplemented");
}

void CheckOk(const osrm::OSRM &osrm, osrm::TripParameters &params, bool use_json_only_api)
{
    using namespace osrm;
    json::Object json_result;
    const auto rc = run_trip_json(osrm, params, json_result, use_json_only_api);
    BOOST_REQUIRE(rc == osrm::Status::Ok);
    auto code = std::get<osrm::json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");
}

void test_tfse_illegal_parameters(bool use_json_only_api)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    const auto locations = get_locations_in_big_component();
    auto params = osrm::TripParameters();

    // one parameter set
    ResetParams(locations, params);
    params.roundtrip = false;
    CheckNotImplemented(osrm, params, use_json_only_api);

    // two parameter set
    ResetParams(locations, params);
    params.source = TripParameters::SourceType::Any;
    params.roundtrip = false;
    CheckNotImplemented(osrm, params, use_json_only_api);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::First;
    params.roundtrip = false;
    CheckOk(osrm, params, use_json_only_api);

    ResetParams(locations, params);
    params.destination = TripParameters::DestinationType::Any;
    params.roundtrip = false;
    CheckNotImplemented(osrm, params, use_json_only_api);

    ResetParams(locations, params);
    params.destination = TripParameters::DestinationType::Last;
    params.roundtrip = false;
    CheckOk(osrm, params, use_json_only_api);

    // three parameters set
    params.source = TripParameters::SourceType::Any;
    params.destination = TripParameters::DestinationType::Any;
    params.roundtrip = false;
    CheckNotImplemented(osrm, params, use_json_only_api);

    params.source = TripParameters::SourceType::Any;
    params.destination = TripParameters::DestinationType::Last;
    params.roundtrip = false;
    CheckOk(osrm, params, use_json_only_api);

    params.source = TripParameters::SourceType::First;
    params.destination = TripParameters::DestinationType::Any;
    params.roundtrip = false;
    CheckOk(osrm, params, use_json_only_api);
}
BOOST_AUTO_TEST_CASE(test_tfse_illegal_parameters_old_api) { test_tfse_illegal_parameters(true); }
BOOST_AUTO_TEST_CASE(test_tfse_illegal_parameters_new_api) { test_tfse_illegal_parameters(false); }

void test_tfse_legal_parameters(bool use_json_only_api)
{
    using namespace osrm;
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    const auto locations = get_locations_in_big_component();
    json::Object result;
    TripParameters params;

    // no parameter set
    ResetParams(locations, params);
    CheckOk(osrm, params, use_json_only_api);

    // one parameter set
    ResetParams(locations, params);
    params.roundtrip = true;
    CheckOk(osrm, params, use_json_only_api);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::First;
    CheckOk(osrm, params, use_json_only_api);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::Any;
    CheckOk(osrm, params, use_json_only_api);

    ResetParams(locations, params);
    params.destination = TripParameters::DestinationType::Any;
    CheckOk(osrm, params, use_json_only_api);

    ResetParams(locations, params);
    params.destination = TripParameters::DestinationType::Last;
    CheckOk(osrm, params, use_json_only_api);

    // two parameter set
    ResetParams(locations, params);
    params.destination = TripParameters::DestinationType::Last;
    params.roundtrip = true;
    CheckOk(osrm, params, use_json_only_api);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::First;
    params.roundtrip = true;
    CheckOk(osrm, params, use_json_only_api);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::First;
    params.destination = TripParameters::DestinationType::Any;
    CheckOk(osrm, params, use_json_only_api);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::Any;
    params.destination = TripParameters::DestinationType::Last;
    CheckOk(osrm, params, use_json_only_api);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::First;
    params.destination = TripParameters::DestinationType::Last;
    CheckOk(osrm, params, use_json_only_api);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::Any;
    params.roundtrip = true;
    CheckOk(osrm, params, use_json_only_api);

    ResetParams(locations, params);
    params.destination = TripParameters::DestinationType::Any;
    params.roundtrip = true;
    CheckOk(osrm, params, use_json_only_api);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::Any;
    params.destination = TripParameters::DestinationType::Any;
    CheckOk(osrm, params, use_json_only_api);

    // three parameter set
    params.source = TripParameters::SourceType::Any;
    params.destination = TripParameters::DestinationType::Any;
    params.roundtrip = true;
    CheckOk(osrm, params, use_json_only_api);

    params.source = TripParameters::SourceType::First;
    params.destination = TripParameters::DestinationType::Last;
    params.roundtrip = false;
    CheckOk(osrm, params, use_json_only_api);

    params.source = TripParameters::SourceType::Any;
    params.destination = TripParameters::DestinationType::Last;
    params.roundtrip = true;
    CheckOk(osrm, params, use_json_only_api);

    params.source = TripParameters::SourceType::First;
    params.destination = TripParameters::DestinationType::Any;
    params.roundtrip = true;
    CheckOk(osrm, params, use_json_only_api);

    params.source = TripParameters::SourceType::First;
    params.destination = TripParameters::DestinationType::Last;
    params.roundtrip = true;
    CheckOk(osrm, params, use_json_only_api);
}
BOOST_AUTO_TEST_CASE(test_tfse_legal_parameters_old_api) { test_tfse_legal_parameters(true); }
BOOST_AUTO_TEST_CASE(test_tfse_legal_parameters_new_api) { test_tfse_legal_parameters(false); }

BOOST_AUTO_TEST_CASE(test_roundtrip_response_fb_serialization)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    const auto locations = get_locations_in_small_component();

    TripParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    engine::api::ResultT result = flatbuffers::FlatBufferBuilder();
    const auto rc = osrm.Trip(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &fb_result = std::get<flatbuffers::FlatBufferBuilder>(result);
    auto fb = engine::api::fbresult::GetFBResult(fb_result.GetBufferPointer());

    BOOST_CHECK(!fb->error());

    BOOST_CHECK(fb->waypoints() != nullptr);
    const auto waypoints = fb->waypoints();
    BOOST_CHECK(waypoints->size() == params.coordinates.size());

    BOOST_CHECK(fb->routes() != nullptr);
    const auto trips = fb->routes();
    BOOST_CHECK_EQUAL(trips->size(), 1);

    for (const auto waypoint : *waypoints)
    {
        const auto longitude = waypoint->location()->longitude();
        const auto latitude = waypoint->location()->latitude();
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);

        const auto trip = waypoint->trips_index();
        const auto pos = waypoint->waypoint_index();
        BOOST_CHECK(trip < trips->size());
        BOOST_CHECK(pos < waypoints->size());
    }
}

BOOST_AUTO_TEST_CASE(test_roundtrip_response_fb_serialization_skip_waypoints)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    const auto locations = get_locations_in_small_component();

    TripParameters params;
    params.skip_waypoints = true;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    engine::api::ResultT result = flatbuffers::FlatBufferBuilder();
    const auto rc = osrm.Trip(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &fb_result = std::get<flatbuffers::FlatBufferBuilder>(result);
    auto fb = engine::api::fbresult::GetFBResult(fb_result.GetBufferPointer());

    BOOST_CHECK(!fb->error());

    BOOST_CHECK(fb->waypoints() == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
