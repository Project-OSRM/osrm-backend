#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "args.hpp"
#include "coordinates.hpp"
#include "fixture.hpp"

#include "osrm/trip_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

BOOST_AUTO_TEST_SUITE(trip)

BOOST_AUTO_TEST_CASE(test_trip_response_for_locations_in_small_component)
{
    const auto args = get_args();
    auto osrm = getOSRM(args.at(0));

    using namespace osrm;

    const auto locations = get_locations_in_small_component();

    TripParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    json::Object result;
    const auto rc = osrm.Trip(params, result);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = result.values.at("waypoints").get<json::Array>().values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    const auto &trips = result.values.at("trips").get<json::Array>().values;
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

BOOST_AUTO_TEST_CASE(test_trip_response_for_locations_in_big_component)
{
    const auto args = get_args();
    auto osrm = getOSRM(args.at(0));

    using namespace osrm;

    const auto locations = get_locations_in_big_component();

    TripParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    json::Object result;
    const auto rc = osrm.Trip(params, result);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = result.values.at("waypoints").get<json::Array>().values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    const auto &trips = result.values.at("trips").get<json::Array>().values;
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

BOOST_AUTO_TEST_SUITE_END()
