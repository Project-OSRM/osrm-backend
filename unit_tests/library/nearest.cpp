#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "coordinates.hpp"
#include "fixture.hpp"

#include "osrm/nearest_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

BOOST_AUTO_TEST_SUITE(nearest)

BOOST_AUTO_TEST_CASE(test_nearest_response)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    NearestParameters params;
    params.coordinates.push_back(get_dummy_location());

    json::Object result;
    const auto rc = osrm.Nearest(params, result);
    BOOST_REQUIRE(rc == Status::Ok);

    const auto code = result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = result.values.at("waypoints").get<json::Array>().values;
    BOOST_CHECK(!waypoints.empty()); // the dataset has at least one nearest coordinate

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = waypoint.get<json::Object>();
        const auto distance = waypoint_object.values.at("distance").get<json::Number>().value;
        BOOST_CHECK(distance >= 0);
    }
}

BOOST_AUTO_TEST_CASE(test_nearest_response_no_coordinates)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    NearestParameters params;

    json::Object result;
    const auto rc = osrm.Nearest(params, result);
    BOOST_REQUIRE(rc == Status::Error);

    const auto code = result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "InvalidOptions");
}

BOOST_AUTO_TEST_CASE(test_nearest_response_multiple_coordinates)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    NearestParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    json::Object result;
    const auto rc = osrm.Nearest(params, result);
    BOOST_REQUIRE(rc == Status::Error);

    const auto code = result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "InvalidOptions");
}

BOOST_AUTO_TEST_CASE(test_nearest_response_for_location_in_small_component)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    const auto locations = get_locations_in_small_component();

    NearestParameters params;
    params.coordinates.push_back(locations.at(0));
    params.number_of_results = 3;

    json::Object result;
    const auto rc = osrm.Nearest(params, result);
    BOOST_REQUIRE(rc == Status::Ok);

    const auto code = result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = result.values.at("waypoints").get<json::Array>().values;
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

BOOST_AUTO_TEST_SUITE_END()
