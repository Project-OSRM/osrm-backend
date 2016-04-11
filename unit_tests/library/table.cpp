#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include "args.hpp"
#include "coordinates.hpp"
#include "fixture.hpp"

#include "osrm/table_parameters.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/status.hpp"
#include "osrm/osrm.hpp"

BOOST_AUTO_TEST_SUITE(table)

// TODO
//
// three coordinates input, no options returns 3x3 matrix
// three coordinates input, one source returns 1x3 matrix
// three coordinate input, one source, one destination returns 1x1 matrix
// break out waypoint object array checker into header file
//

BOOST_AUTO_TEST_CASE(test_table_three_coordinates_matrix)
{

    const auto args = get_args();
    BOOST_REQUIRE_EQUAL(args.size(), 1);

    using namespace osrm;

    auto osrm = getOSRM(args[0]);

    TableParameters params;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    json::Object result;

    const auto rc = osrm.Table(params, result);

    BOOST_CHECK(rc == Status::Ok || rc == Status::Error);
    const auto code = result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &durations_array = result.values.at("durations").get<json::Array>().values;
    BOOST_CHECK_EQUAL(durations_array.size(), params.coordinates.size());
    for (unsigned int i = 0; i < durations_array.size(); i++)
    {
        const auto durations_matrix = durations_array[i].get<json::Array>().values;
        BOOST_CHECK_EQUAL(durations_matrix[i].get<json::Number>().value, 0);
        BOOST_CHECK_EQUAL(durations_matrix.size(), params.coordinates.size());
    }
    const auto &destinations_array = result.values.at("destinations").get<json::Array>().values;
    for (const auto &destination : destinations_array)
    {
        const auto &dest_object = destination.get<json::Object>();
        const auto &dest_location = dest_object.values.at("location").get<json::Array>().values;
        util::FloatLongitude lon(dest_location[0].get<json::Number>().value);
        util::FloatLatitude lat(dest_location[1].get<json::Number>().value);
        util::Coordinate location_coordinate(lon, lat);
        BOOST_CHECK(location_coordinate.IsValid());
    }
    const auto &sources_array = result.values.at("sources").get<json::Array>().values;
    BOOST_CHECK_EQUAL(sources_array.size(), params.coordinates.size());
    for (const auto &destination : destinations_array)
    {
        const auto &dest_object = destination.get<json::Object>();
        const auto &dest_location = dest_object.values.at("location").get<json::Array>().values;
        util::FloatLongitude lon(dest_location[0].get<json::Number>().value);
        util::FloatLatitude lat(dest_location[1].get<json::Number>().value);
        util::Coordinate location_coordinate(lon, lat);
        BOOST_CHECK(location_coordinate.IsValid());
    }
}

BOOST_AUTO_TEST_SUITE_END()
