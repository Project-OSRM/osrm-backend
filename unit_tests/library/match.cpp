#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "coordinates.hpp"
#include "fixture.hpp"
#include "waypoint_check.hpp"

#include "osrm/match_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

BOOST_AUTO_TEST_SUITE(match)

BOOST_AUTO_TEST_CASE(test_match)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    MatchParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    engine::api::ResultT result = json::Object();

    const auto rc = osrm.Match(params, result);

    auto &json_result = result.get<json::Object>();
    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &tracepoints = json_result.values.at("tracepoints").get<json::Array>().values;
    BOOST_CHECK_EQUAL(tracepoints.size(), params.coordinates.size());

    const auto &matchings = json_result.values.at("matchings").get<json::Array>().values;
    const auto &number_of_matchings = matchings.size();
    for (const auto &waypoint : tracepoints)
    {
        if (waypoint.is<mapbox::util::recursive_wrapper<util::json::Object>>())
        {
            BOOST_CHECK(waypoint_check(waypoint));
            const auto &waypoint_object = waypoint.get<json::Object>();
            const auto matchings_index =
                waypoint_object.values.at("matchings_index").get<json::Number>().value;
            const auto waypoint_index =
                waypoint_object.values.at("waypoint_index").get<json::Number>().value;
            const auto &route_legs = matchings[matchings_index]
                                         .get<json::Object>()
                                         .values.at("legs")
                                         .get<json::Array>()
                                         .values;
            BOOST_CHECK_LT(waypoint_index, route_legs.size() + 1);
            BOOST_CHECK_LT(matchings_index, number_of_matchings);
        }
        else
        {
            BOOST_CHECK(waypoint.is<json::Null>());
        }
    }
}

BOOST_AUTO_TEST_CASE(test_match_split)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    MatchParameters params;
    params.coordinates = get_split_trace_locations();
    params.timestamps = {1, 2, 1700, 1800};

    engine::api::ResultT result = json::Object();

    const auto rc = osrm.Match(params, result);

    auto &json_result = result.get<json::Object>();
    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &tracepoints = json_result.values.at("tracepoints").get<json::Array>().values;
    BOOST_CHECK_EQUAL(tracepoints.size(), params.coordinates.size());

    const auto &matchings = json_result.values.at("matchings").get<json::Array>().values;
    const auto &number_of_matchings = matchings.size();
    BOOST_CHECK_EQUAL(number_of_matchings, 2);
    std::size_t current_matchings_index = 0, expected_waypoint_index = 0;
    for (const auto &waypoint : tracepoints)
    {
        if (waypoint.is<mapbox::util::recursive_wrapper<util::json::Object>>())
        {
            BOOST_CHECK(waypoint_check(waypoint));
            const auto &waypoint_object = waypoint.get<json::Object>();
            const auto matchings_index =
                waypoint_object.values.at("matchings_index").get<json::Number>().value;
            const auto waypoint_index =
                waypoint_object.values.at("waypoint_index").get<json::Number>().value;

            BOOST_CHECK_LT(matchings_index, number_of_matchings);

            expected_waypoint_index =
                (current_matchings_index != matchings_index) ? 0 : expected_waypoint_index;
            BOOST_CHECK_EQUAL(waypoint_index, expected_waypoint_index);

            current_matchings_index = matchings_index;
            ++expected_waypoint_index;
        }
        else
        {
            BOOST_CHECK(waypoint.is<json::Null>());
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
