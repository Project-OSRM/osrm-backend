#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "coordinates.hpp"
#include "fixture.hpp"
#include "waypoint_check.hpp"

#include "osrm/table_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

BOOST_AUTO_TEST_SUITE(table)

BOOST_AUTO_TEST_CASE(test_table_three_coords_one_source_one_dest_matrix)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    TableParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.sources.push_back(0);
    params.destinations.push_back(2);
    params.annotations = TableParameters::AnnotationsType::All;

    engine::api::ResultT result = json::Object();

    const auto rc = osrm.Table(params, result);

    auto &json_result = result.get<json::Object>();
    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    // check that returned durations error is expected size and proportions
    // this test expects a 1x1 matrix
    const auto &durations_array = json_result.values.at("durations").get<json::Array>().values;
    BOOST_CHECK_EQUAL(durations_array.size(), params.sources.size());
    for (unsigned int i = 0; i < durations_array.size(); i++)
    {
        const auto durations_matrix = durations_array[i].get<json::Array>().values;
        BOOST_CHECK_EQUAL(durations_matrix.size(),
                          params.sources.size() * params.destinations.size());
    }

    // check that returned distances error is expected size and proportions
    // this test expects a 1x1 matrix
    const auto &distances_array = json_result.values.at("distances").get<json::Array>().values;
    BOOST_CHECK_EQUAL(distances_array.size(), params.sources.size());
    for (unsigned int i = 0; i < distances_array.size(); i++)
    {
        const auto distances_matrix = distances_array[i].get<json::Array>().values;
        BOOST_CHECK_EQUAL(distances_matrix.size(),
                          params.sources.size() * params.destinations.size());
    }

    // check destinations array of waypoint objects
    const auto &destinations_array =
        json_result.values.at("destinations").get<json::Array>().values;
    BOOST_CHECK_EQUAL(destinations_array.size(), params.destinations.size());
    for (const auto &destination : destinations_array)
    {
        BOOST_CHECK(waypoint_check(destination));
    }
    // check sources array of waypoint objects
    const auto &sources_array = json_result.values.at("sources").get<json::Array>().values;
    BOOST_CHECK_EQUAL(sources_array.size(), params.sources.size());
    for (const auto &source : sources_array)
    {
        BOOST_CHECK(waypoint_check(source));
    }
}

BOOST_AUTO_TEST_CASE(test_table_three_coords_one_source_matrix)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    TableParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.sources.push_back(0);
    engine::api::ResultT result = json::Object();

    const auto rc = osrm.Table(params, result);

    auto &json_result = result.get<json::Object>();
    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    // check that returned durations error is expected size and proportions
    // this test expects a 1x3 matrix
    const auto &durations_array = json_result.values.at("durations").get<json::Array>().values;
    BOOST_CHECK_EQUAL(durations_array.size(), params.sources.size());
    for (unsigned int i = 0; i < durations_array.size(); i++)
    {
        const auto durations_matrix = durations_array[i].get<json::Array>().values;
        BOOST_CHECK_EQUAL(durations_matrix[i].get<json::Number>().value, 0);
        BOOST_CHECK_EQUAL(durations_matrix.size(),
                          params.sources.size() * params.coordinates.size());
    }
    // check destinations array of waypoint objects
    const auto &destinations_array =
        json_result.values.at("destinations").get<json::Array>().values;
    BOOST_CHECK_EQUAL(destinations_array.size(), params.coordinates.size());
    for (const auto &destination : destinations_array)
    {
        BOOST_CHECK(waypoint_check(destination));
    }
    // check sources array of waypoint objects
    const auto &sources_array = json_result.values.at("sources").get<json::Array>().values;
    BOOST_CHECK_EQUAL(sources_array.size(), params.sources.size());
    for (const auto &source : sources_array)
    {
        BOOST_CHECK(waypoint_check(source));
    }
}

BOOST_AUTO_TEST_CASE(test_table_three_coordinates_matrix)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    TableParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.annotations = TableParameters::AnnotationsType::Duration;

    engine::api::ResultT result = json::Object();

    const auto rc = osrm.Table(params, result);

    auto &json_result = result.get<json::Object>();
    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    // check that returned durations error is expected size and proportions
    // this test expects a 3x3 matrix
    const auto &durations_array = json_result.values.at("durations").get<json::Array>().values;
    BOOST_CHECK_EQUAL(durations_array.size(), params.coordinates.size());
    for (unsigned int i = 0; i < durations_array.size(); i++)
    {
        const auto durations_matrix = durations_array[i].get<json::Array>().values;
        BOOST_CHECK_EQUAL(durations_matrix[i].get<json::Number>().value, 0);
        BOOST_CHECK_EQUAL(durations_matrix.size(), params.coordinates.size());
    }
    const auto &destinations_array =
        json_result.values.at("destinations").get<json::Array>().values;
    for (const auto &destination : destinations_array)
    {
        BOOST_CHECK(waypoint_check(destination));
    }
    const auto &sources_array = json_result.values.at("sources").get<json::Array>().values;
    BOOST_CHECK_EQUAL(sources_array.size(), params.coordinates.size());
    for (const auto &source : sources_array)
    {
        BOOST_CHECK(waypoint_check(source));
    }
}

// See https://github.com/Project-OSRM/osrm-backend/pull/3992
BOOST_AUTO_TEST_CASE(test_table_no_segment_for_some_coordinates)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    TableParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    // resembles query option: `&radiuses=0;`
    params.radiuses.push_back(boost::make_optional(0.));
    params.radiuses.push_back(boost::none);

    engine::api::ResultT result = json::Object();

    const auto rc = osrm.Table(params, result);

    auto &json_result = result.get<json::Object>();
    BOOST_CHECK(rc == Status::Error);
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "NoSegment");
}

BOOST_AUTO_TEST_CASE(test_table_serialiaze_fb)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    TableParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.sources.push_back(0);
    params.destinations.push_back(2);
    params.annotations = TableParameters::AnnotationsType::All;

    engine::api::ResultT result = flatbuffers::FlatBufferBuilder();

    const auto rc = osrm.Table(params, result);

    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);

    auto &fb_result = result.get<flatbuffers::FlatBufferBuilder>();
    auto fb = engine::api::fbresult::GetFBResult(fb_result.GetBufferPointer());
    BOOST_CHECK(!fb->error());
    BOOST_CHECK(fb->table() != nullptr);

    // check that returned durations error is expected size and proportions
    // this test expects a 1x1 matrix
    BOOST_CHECK(fb->table()->durations() != nullptr);
    auto durations_array = fb->table()->durations();
    BOOST_CHECK_EQUAL(durations_array->size(), params.sources.size() * params.destinations.size());

    // check that returned distances error is expected size and proportions
    // this test expects a 1x1 matrix
    BOOST_CHECK(fb->table()->distances() != nullptr);
    auto distances_array = fb->table()->distances();
    BOOST_CHECK_EQUAL(distances_array->size(), params.sources.size() * params.destinations.size());

    // check destinations array of waypoint objects
    const auto &destinations_array = fb->table()->destinations();
    BOOST_CHECK_EQUAL(destinations_array->size(), params.destinations.size());
    for (const auto &destination : *destinations_array)
    {
        BOOST_CHECK(waypoint_check(destination));
    }
    // check sources array of waypoint objects
    const auto &sources_array = fb->waypoints();
    BOOST_CHECK_EQUAL(sources_array->size(), params.sources.size());
    for (const auto &source : *sources_array)
    {
        BOOST_CHECK(waypoint_check(source));
    }
}

BOOST_AUTO_TEST_SUITE_END()
