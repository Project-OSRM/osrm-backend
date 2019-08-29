#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "coordinates.hpp"
#include "fixture.hpp"

#include "osrm/trip_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

BOOST_AUTO_TEST_SUITE(trip)

BOOST_AUTO_TEST_CASE(test_roundtrip_response_for_locations_in_small_component)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    const auto locations = get_locations_in_small_component();

    TripParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    engine::api::ResultT result = json::Object();
    const auto rc = osrm.Trip(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = json_result.values.at("waypoints").get<json::Array>().values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    const auto &trips = json_result.values.at("trips").get<json::Array>().values;
    BOOST_CHECK_EQUAL(trips.size(), 1);

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = waypoint.get<json::Object>();

        const auto location = waypoint_object.values.at("location").get<json::Array>().values;
        const auto longitude = location[0].get<json::Number>().value;
        const auto latitude = location[1].get<json::Number>().value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);

        const auto trip = waypoint_object.values.at("trips_index").get<json::Number>().value;
        const auto pos = waypoint_object.values.at("waypoint_index").get<json::Number>().value;
        BOOST_CHECK(trip >= 0 && trip < trips.size());
        BOOST_CHECK(pos >= 0 && pos < waypoints.size());
    }
}

BOOST_AUTO_TEST_CASE(test_roundtrip_response_for_locations_in_big_component)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    const auto locations = get_locations_in_big_component();

    TripParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    engine::api::ResultT result = json::Object();
    const auto rc = osrm.Trip(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = json_result.values.at("waypoints").get<json::Array>().values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    const auto &trips = json_result.values.at("trips").get<json::Array>().values;
    BOOST_CHECK_EQUAL(trips.size(), 1);

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = waypoint.get<json::Object>();

        const auto location = waypoint_object.values.at("location").get<json::Array>().values;
        const auto longitude = location[0].get<json::Number>().value;
        const auto latitude = location[1].get<json::Number>().value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);

        const auto trip = waypoint_object.values.at("trips_index").get<json::Number>().value;
        const auto pos = waypoint_object.values.at("waypoint_index").get<json::Number>().value;
        BOOST_CHECK(trip >= 0 && trip < trips.size());
        BOOST_CHECK(pos >= 0 && pos < waypoints.size());
    }
}

BOOST_AUTO_TEST_CASE(test_roundtrip_response_for_locations_across_components)
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

    engine::api::ResultT result = json::Object();
    const auto rc = osrm.Trip(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = json_result.values.at("waypoints").get<json::Array>().values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    const auto &trips = json_result.values.at("trips").get<json::Array>().values;
    BOOST_CHECK_EQUAL(trips.size(), 1);
    // ^ First snapping, then SCC decomposition (see plugins/trip.cpp). Therefore only a single
    // trip.

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = waypoint.get<json::Object>();

        const auto location = waypoint_object.values.at("location").get<json::Array>().values;
        const auto longitude = location[0].get<json::Number>().value;
        const auto latitude = location[1].get<json::Number>().value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);

        const auto trip = waypoint_object.values.at("trips_index").get<json::Number>().value;
        const auto pos = waypoint_object.values.at("waypoint_index").get<json::Number>().value;
        BOOST_CHECK(trip >= 0 && trip < trips.size());
        BOOST_CHECK(pos >= 0 && pos < waypoints.size());
    }
}

BOOST_AUTO_TEST_CASE(test_tfse_1)
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

    engine::api::ResultT result = json::Object();
    const auto rc = osrm.Trip(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = json_result.values.at("waypoints").get<json::Array>().values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    const auto &trips = json_result.values.at("trips").get<json::Array>().values;
    BOOST_CHECK_EQUAL(trips.size(), 1);

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = waypoint.get<json::Object>();

        const auto location = waypoint_object.values.at("location").get<json::Array>().values;
        const auto longitude = location[0].get<json::Number>().value;
        const auto latitude = location[1].get<json::Number>().value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);

        const auto trip = waypoint_object.values.at("trips_index").get<json::Number>().value;
        const auto pos = waypoint_object.values.at("waypoint_index").get<json::Number>().value;
        BOOST_CHECK(trip >= 0 && trip < trips.size());
        BOOST_CHECK(pos >= 0 && pos < waypoints.size());
    }
}

BOOST_AUTO_TEST_CASE(test_tfse_2)
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

    engine::api::ResultT result = json::Object();
    const auto rc = osrm.Trip(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = json_result.values.at("waypoints").get<json::Array>().values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    const auto &trips = json_result.values.at("trips").get<json::Array>().values;
    BOOST_CHECK_EQUAL(trips.size(), 1);

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = waypoint.get<json::Object>();

        const auto location = waypoint_object.values.at("location").get<json::Array>().values;
        const auto longitude = location[0].get<json::Number>().value;
        const auto latitude = location[1].get<json::Number>().value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);

        const auto trip = waypoint_object.values.at("trips_index").get<json::Number>().value;
        const auto pos = waypoint_object.values.at("waypoint_index").get<json::Number>().value;
        BOOST_CHECK(trip >= 0 && trip < trips.size());
        BOOST_CHECK(pos >= 0 && pos < waypoints.size());
    }
}

void ResetParams(const Locations &locations, osrm::TripParameters &params)
{
    params = osrm::TripParameters();
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));
}
void CheckNotImplemented(const osrm::OSRM &osrm, osrm::TripParameters &params)
{
    using namespace osrm;
    engine::api::ResultT result = json::Object();
    auto rc = osrm.Trip(params, result);
    BOOST_REQUIRE(rc == osrm::Status::Error);
    auto &json_result = result.get<json::Object>();
    auto code = json_result.values.at("code").get<osrm::json::String>().value;
    BOOST_CHECK_EQUAL(code, "NotImplemented");
}

void CheckOk(const osrm::OSRM &osrm, osrm::TripParameters &params)
{
    using namespace osrm;
    engine::api::ResultT result = json::Object();
    auto rc = osrm.Trip(params, result);
    BOOST_REQUIRE(rc == osrm::Status::Ok);
    auto &json_result = result.get<json::Object>();
    auto code = json_result.values.at("code").get<osrm::json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");
}

BOOST_AUTO_TEST_CASE(test_tfse_illegal_parameters)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    const auto locations = get_locations_in_big_component();
    auto params = osrm::TripParameters();

    // one parameter set
    ResetParams(locations, params);
    params.roundtrip = false;
    CheckNotImplemented(osrm, params);

    // two parameter set
    ResetParams(locations, params);
    params.source = TripParameters::SourceType::Any;
    params.roundtrip = false;
    CheckNotImplemented(osrm, params);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::First;
    params.roundtrip = false;
    CheckNotImplemented(osrm, params);

    ResetParams(locations, params);
    params.destination = TripParameters::DestinationType::Any;
    params.roundtrip = false;
    CheckNotImplemented(osrm, params);

    ResetParams(locations, params);
    params.destination = TripParameters::DestinationType::Last;
    params.roundtrip = false;
    CheckNotImplemented(osrm, params);

    // three parameters set
    params.source = TripParameters::SourceType::Any;
    params.destination = TripParameters::DestinationType::Any;
    params.roundtrip = false;
    CheckNotImplemented(osrm, params);

    params.source = TripParameters::SourceType::Any;
    params.destination = TripParameters::DestinationType::Last;
    params.roundtrip = false;
    CheckNotImplemented(osrm, params);

    params.source = TripParameters::SourceType::First;
    params.destination = TripParameters::DestinationType::Any;
    params.roundtrip = false;
    CheckNotImplemented(osrm, params);
}

BOOST_AUTO_TEST_CASE(test_tfse_legal_parameters)
{
    using namespace osrm;
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");
    const auto locations = get_locations_in_big_component();
    json::Object result;
    TripParameters params;

    // no parameter set
    ResetParams(locations, params);
    CheckOk(osrm, params);

    // one parameter set
    ResetParams(locations, params);
    params.roundtrip = true;
    CheckOk(osrm, params);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::First;
    CheckOk(osrm, params);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::Any;
    CheckOk(osrm, params);

    ResetParams(locations, params);
    params.destination = TripParameters::DestinationType::Any;
    CheckOk(osrm, params);

    ResetParams(locations, params);
    params.destination = TripParameters::DestinationType::Last;
    CheckOk(osrm, params);

    // two parameter set
    ResetParams(locations, params);
    params.destination = TripParameters::DestinationType::Last;
    params.roundtrip = true;
    CheckOk(osrm, params);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::First;
    params.roundtrip = true;
    CheckOk(osrm, params);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::First;
    params.destination = TripParameters::DestinationType::Any;
    CheckOk(osrm, params);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::Any;
    params.destination = TripParameters::DestinationType::Last;
    CheckOk(osrm, params);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::First;
    params.destination = TripParameters::DestinationType::Last;
    CheckOk(osrm, params);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::Any;
    params.roundtrip = true;
    CheckOk(osrm, params);

    ResetParams(locations, params);
    params.destination = TripParameters::DestinationType::Any;
    params.roundtrip = true;
    CheckOk(osrm, params);

    ResetParams(locations, params);
    params.source = TripParameters::SourceType::Any;
    params.destination = TripParameters::DestinationType::Any;
    CheckOk(osrm, params);

    // three parameter set
    params.source = TripParameters::SourceType::Any;
    params.destination = TripParameters::DestinationType::Any;
    params.roundtrip = true;
    CheckOk(osrm, params);

    params.source = TripParameters::SourceType::First;
    params.destination = TripParameters::DestinationType::Last;
    params.roundtrip = false;
    CheckOk(osrm, params);

    params.source = TripParameters::SourceType::Any;
    params.destination = TripParameters::DestinationType::Last;
    params.roundtrip = true;
    CheckOk(osrm, params);

    params.source = TripParameters::SourceType::First;
    params.destination = TripParameters::DestinationType::Any;
    params.roundtrip = true;
    CheckOk(osrm, params);

    params.source = TripParameters::SourceType::First;
    params.destination = TripParameters::DestinationType::Last;
    params.roundtrip = true;
    CheckOk(osrm, params);
}

BOOST_AUTO_TEST_SUITE_END()
