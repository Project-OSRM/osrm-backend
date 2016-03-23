#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include "args.hpp"
#include "fixture.hpp"
#include "coordinates.hpp"

#include "osrm/nearest_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/status.hpp"
#include "osrm/osrm.hpp"

BOOST_AUTO_TEST_SUITE(nearest)

BOOST_AUTO_TEST_CASE(test_nearest_response)
{
    const auto args = get_args();
    auto osrm = get_osrm(args.at(0));

    using namespace osrm;

    NearestParameters params;
    params.coordinates.push_back(get_dummy_location());

    json::Object result;
    const auto rc = osrm.Nearest(params, result);
    BOOST_REQUIRE(rc == Status::Ok);

    const auto code = result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "ok");

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
    const auto args = get_args();
    auto osrm = get_osrm(args.at(0));

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
    const auto args = get_args();
    auto osrm = get_osrm(args.at(0));

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

BOOST_AUTO_TEST_SUITE_END()
