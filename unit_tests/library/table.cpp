#include <boost/test/unit_test.hpp>

#include "coordinates.hpp"
#include "fixture.hpp"
#include "waypoint_check.hpp"

#include "osrm/table_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

osrm::Status run_table_json(const osrm::OSRM &osrm,
                            const osrm::TableParameters &params,
                            osrm::json::Object &json_result,
                            bool use_json_only_api)
{
    if (use_json_only_api)
    {
        return osrm.Table(params, json_result);
    }
    osrm::engine::api::ResultT result = osrm::json::Object();
    auto rc = osrm.Table(params, result);
    json_result = std::get<osrm::json::Object>(result);
    return rc;
}

BOOST_AUTO_TEST_SUITE(table)

void test_table_three_coords_one_source_one_dest_matrix(bool use_json_only_api)
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

    json::Object json_result;
    const auto rc = run_table_json(osrm, params, json_result, use_json_only_api);

    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    // check that returned durations error is expected size and proportions
    // this test expects a 1x1 matrix
    const auto &durations_array = std::get<json::Array>(json_result.values.at("durations")).values;
    BOOST_CHECK_EQUAL(durations_array.size(), params.sources.size());
    for (unsigned int i = 0; i < durations_array.size(); i++)
    {
        const auto durations_matrix = std::get<json::Array>(durations_array[i]).values;
        BOOST_CHECK_EQUAL(durations_matrix.size(),
                          params.sources.size() * params.destinations.size());
    }

    // check that returned distances error is expected size and proportions
    // this test expects a 1x1 matrix
    const auto &distances_array = std::get<json::Array>(json_result.values.at("distances")).values;
    BOOST_CHECK_EQUAL(distances_array.size(), params.sources.size());
    for (unsigned int i = 0; i < distances_array.size(); i++)
    {
        const auto distances_matrix = std::get<json::Array>(distances_array[i]).values;
        BOOST_CHECK_EQUAL(distances_matrix.size(),
                          params.sources.size() * params.destinations.size());
    }

    // check destinations array of waypoint objects
    const auto &destinations_array =
        std::get<json::Array>(json_result.values.at("destinations")).values;
    BOOST_CHECK_EQUAL(destinations_array.size(), params.destinations.size());
    for (const auto &destination : destinations_array)
    {
        BOOST_CHECK(waypoint_check(destination));
    }
    // check sources array of waypoint objects
    const auto &sources_array = std::get<json::Array>(json_result.values.at("sources")).values;
    BOOST_CHECK_EQUAL(sources_array.size(), params.sources.size());
    for (const auto &source : sources_array)
    {
        BOOST_CHECK(waypoint_check(source));
    }
}
BOOST_AUTO_TEST_CASE(test_table_three_coords_one_source_one_dest_matrix_old_api)
{
    test_table_three_coords_one_source_one_dest_matrix(true);
}
BOOST_AUTO_TEST_CASE(test_table_three_coords_one_source_one_dest_matrix_new_api)
{
    test_table_three_coords_one_source_one_dest_matrix(false);
}

void test_table_three_coords_one_source_one_dest_matrix_no_waypoints(bool use_json_only_api)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    TableParameters params;
    params.skip_waypoints = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.sources.push_back(0);
    params.destinations.push_back(2);
    params.annotations = TableParameters::AnnotationsType::All;

    json::Object json_result;
    const auto rc = run_table_json(osrm, params, json_result, use_json_only_api);

    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    // check that returned durations error is expected size and proportions
    // this test expects a 1x1 matrix
    const auto &durations_array = std::get<json::Array>(json_result.values.at("durations")).values;
    BOOST_CHECK_EQUAL(durations_array.size(), params.sources.size());
    for (unsigned int i = 0; i < durations_array.size(); i++)
    {
        const auto durations_matrix = std::get<json::Array>(durations_array[i]).values;
        BOOST_CHECK_EQUAL(durations_matrix.size(),
                          params.sources.size() * params.destinations.size());
    }

    // check that returned distances error is expected size and proportions
    // this test expects a 1x1 matrix
    const auto &distances_array = std::get<json::Array>(json_result.values.at("distances")).values;
    BOOST_CHECK_EQUAL(distances_array.size(), params.sources.size());
    for (unsigned int i = 0; i < distances_array.size(); i++)
    {
        const auto distances_matrix = std::get<json::Array>(distances_array[i]).values;
        BOOST_CHECK_EQUAL(distances_matrix.size(),
                          params.sources.size() * params.destinations.size());
    }

    // waypoint arrays should be missing
    BOOST_CHECK(json_result.values.find("destinations") == json_result.values.end());
    BOOST_CHECK(json_result.values.find("sources") == json_result.values.end());
}
BOOST_AUTO_TEST_CASE(test_table_three_coords_one_source_one_dest_matrix_no_waypoints_old_api)
{
    test_table_three_coords_one_source_one_dest_matrix_no_waypoints(true);
}
BOOST_AUTO_TEST_CASE(test_table_three_coords_one_source_one_dest_matrix_no_waypoints_new_api)
{
    test_table_three_coords_one_source_one_dest_matrix_no_waypoints(false);
}

void test_table_three_coords_one_source_matrix(bool use_json_only_api)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    TableParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.sources.push_back(0);
    engine::api::ResultT result = json::Object();

    json::Object json_result;
    const auto rc = run_table_json(osrm, params, json_result, use_json_only_api);

    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    // check that returned durations error is expected size and proportions
    // this test expects a 1x3 matrix
    const auto &durations_array = std::get<json::Array>(json_result.values.at("durations")).values;
    BOOST_CHECK_EQUAL(durations_array.size(), params.sources.size());
    for (unsigned int i = 0; i < durations_array.size(); i++)
    {
        const auto durations_matrix = std::get<json::Array>(durations_array[i]).values;
        BOOST_CHECK_EQUAL(std::get<json::Number>(durations_matrix[i]).value, 0);
        BOOST_CHECK_EQUAL(durations_matrix.size(),
                          params.sources.size() * params.coordinates.size());
    }
    // check destinations array of waypoint objects
    const auto &destinations_array =
        std::get<json::Array>(json_result.values.at("destinations")).values;
    BOOST_CHECK_EQUAL(destinations_array.size(), params.coordinates.size());
    for (const auto &destination : destinations_array)
    {
        BOOST_CHECK(waypoint_check(destination));
    }
    // check sources array of waypoint objects
    const auto &sources_array = std::get<json::Array>(json_result.values.at("sources")).values;
    BOOST_CHECK_EQUAL(sources_array.size(), params.sources.size());
    for (const auto &source : sources_array)
    {
        BOOST_CHECK(waypoint_check(source));
    }
}
BOOST_AUTO_TEST_CASE(test_table_three_coords_one_source_matrix_old_api)
{
    test_table_three_coords_one_source_matrix(true);
}
BOOST_AUTO_TEST_CASE(test_table_three_coords_one_source_matrix_new_api)
{
    test_table_three_coords_one_source_matrix(false);
}

void test_table_three_coordinates_matrix(bool use_json_only_api)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    TableParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.annotations = TableParameters::AnnotationsType::Duration;

    json::Object json_result;
    const auto rc = run_table_json(osrm, params, json_result, use_json_only_api);

    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    // check that returned durations error is expected size and proportions
    // this test expects a 3x3 matrix
    const auto &durations_array = std::get<json::Array>(json_result.values.at("durations")).values;
    BOOST_CHECK_EQUAL(durations_array.size(), params.coordinates.size());
    for (unsigned int i = 0; i < durations_array.size(); i++)
    {
        const auto durations_matrix = std::get<json::Array>(durations_array[i]).values;
        BOOST_CHECK_EQUAL(std::get<json::Number>(durations_matrix[i]).value, 0);
        BOOST_CHECK_EQUAL(durations_matrix.size(), params.coordinates.size());
    }
    const auto &destinations_array =
        std::get<json::Array>(json_result.values.at("destinations")).values;
    for (const auto &destination : destinations_array)
    {
        BOOST_CHECK(waypoint_check(destination));
    }
    const auto &sources_array = std::get<json::Array>(json_result.values.at("sources")).values;
    BOOST_CHECK_EQUAL(sources_array.size(), params.coordinates.size());
    for (const auto &source : sources_array)
    {
        BOOST_CHECK(waypoint_check(source));
    }
}
BOOST_AUTO_TEST_CASE(test_table_three_coordinates_matrix_old_api)
{
    test_table_three_coordinates_matrix(true);
}
BOOST_AUTO_TEST_CASE(test_table_three_coordinates_matrix_new_api)
{
    test_table_three_coordinates_matrix(false);
}

// See https://github.com/Project-OSRM/osrm-backend/pull/3992
void test_table_no_segment_for_some_coordinates(bool use_json_only_api)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    TableParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    // resembles query option: `&radiuses=0;`
    params.radiuses.push_back(std::make_optional(0.));
    params.radiuses.push_back(std::nullopt);

    json::Object json_result;
    const auto rc = run_table_json(osrm, params, json_result, use_json_only_api);

    BOOST_CHECK(rc == Status::Error);
    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "NoSegment");
    const auto message = std::get<json::String>(json_result.values.at("message")).value;
    BOOST_CHECK_EQUAL(message, "Could not find a matching segment for coordinate 0");
}
BOOST_AUTO_TEST_CASE(test_table_no_segment_for_some_coordinates_old_api)
{
    test_table_no_segment_for_some_coordinates(true);
}
BOOST_AUTO_TEST_CASE(test_table_no_segment_for_some_coordinates_new_api)
{
    test_table_no_segment_for_some_coordinates(false);
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

    auto &fb_result = std::get<flatbuffers::FlatBufferBuilder>(result);
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
    for (const auto destination : *destinations_array)
    {
        BOOST_CHECK(waypoint_check(destination));
    }
    // check sources array of waypoint objects
    const auto &sources_array = fb->waypoints();
    BOOST_CHECK_EQUAL(sources_array->size(), params.sources.size());
    for (const auto source : *sources_array)
    {
        BOOST_CHECK(waypoint_check(source));
    }
}

BOOST_AUTO_TEST_CASE(test_table_serialiaze_fb_no_waypoints)
{
    using namespace osrm;

    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    TableParameters params;
    params.skip_waypoints = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.sources.push_back(0);
    params.destinations.push_back(2);
    params.annotations = TableParameters::AnnotationsType::All;

    engine::api::ResultT result = flatbuffers::FlatBufferBuilder();

    const auto rc = osrm.Table(params, result);

    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);

    auto &fb_result = std::get<flatbuffers::FlatBufferBuilder>(result);
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

    BOOST_CHECK(fb->table()->destinations() == nullptr);
    BOOST_CHECK(fb->waypoints() == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
