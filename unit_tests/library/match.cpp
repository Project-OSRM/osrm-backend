#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include "args.hpp"
#include "fixture.hpp"
#include "coordinates.hpp"

#include "osrm/match_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/status.hpp"
#include "osrm/osrm.hpp"

BOOST_AUTO_TEST_SUITE(match)

BOOST_AUTO_TEST_CASE(test_match)
{
    const auto args = get_args();
    BOOST_REQUIRE_EQUAL(args.size(), 1);

    using namespace osrm;

    auto osrm = getOSRM(args[0]);

    MatchParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    json::Object result;

    const auto rc = osrm.Match(params, result);

    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    const auto code = result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &tracepoints = result.values.at("tracepoints").get<json::Array>().values;
    BOOST_CHECK_EQUAL(tracepoints.size(), params.coordinates.size());

    const auto &number_of_matchings = result.values.at("matchings").get<json::Array>().values.size();
    const auto &number_of_tracepoints = tracepoints.size();
    for (const auto &waypoint : tracepoints)
    {
        if (waypoint.is<mapbox::util::recursive_wrapper<util::json::Object>>())
        {
            const auto &waypoint_object = waypoint.get<json::Object>();
            const auto &waypoint_object_location = waypoint_object.values.at("location").get<json::Array>().values;
            util::FloatLongitude lon(waypoint_object_location[0].get<json::Number>().value);
            util::FloatLatitude lat(waypoint_object_location[1].get<json::Number>().value);
            util::Coordinate location_coordinate(lon, lat);
            BOOST_CHECK(location_coordinate.IsValid());
            const auto matchings_index = waypoint_object.values.at("matchings_index").get<json::Number>().value;
            const auto waypoint_index = waypoint_object.values.at("waypoint_index").get<json::Number>().value;
        } else
        {
          BOOST_CHECK(waypoint.is<json::Null>());
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
