#include "server/api/parameters_parser.hpp"

#include "parameters_io.hpp"

#include "engine/api/base_parameters.hpp"
#include "engine/api/match_parameters.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/api/tile_parameters.hpp"
#include "engine/api/trip_parameters.hpp"

#include <boost/optional/optional_io.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#define CHECK_EQUAL_RANGE(R1, R2)                                                                  \
    BOOST_CHECK_EQUAL_COLLECTIONS(R1.begin(), R1.end(), R2.begin(), R2.end());

BOOST_AUTO_TEST_SUITE(api_parameters_parser)

using namespace osrm;
using namespace osrm::server;

// returns distance to front
template <typename ParameterT> std::size_t testInvalidOptions(std::string options)
{
    auto iter = options.begin();
    auto result = api::parseParameters<ParameterT>(iter, options.end());
    BOOST_CHECK(!result);
    return std::distance(options.begin(), iter);
}

BOOST_AUTO_TEST_CASE(invalid_route_urls)
{
    BOOST_CHECK_EQUAL(
        testInvalidOptions<engine::api::RouteParameters>("1,2;3,4?overview=false&bla=foo"), 22UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<engine::api::RouteParameters>("1,2;3,4?overview=false&bearings=foo"),
        32UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<engine::api::RouteParameters>("1,2;3,4?overview=false&continue_straight=foo"),
        41UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<engine::api::RouteParameters>("1,2;3,4?overview=false&radiuses=foo"),
        32UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<engine::api::RouteParameters>("1,2;3,4?overview=false&hints=foo"), 29UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<engine::api::RouteParameters>("1,2;3,4?overview=false&geometries=foo"),
        22UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<engine::api::RouteParameters>("1,2;3,4?overview=false&overview=foo"),
        22L);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<engine::api::RouteParameters>("1,2;3,4?overview=false&alternatives=foo"),
        36UL);
}

BOOST_AUTO_TEST_CASE(invalid_table_urls)
{
    BOOST_CHECK_EQUAL(testInvalidOptions<engine::api::TableParameters>("1,2;3,4?sources=1&bla=foo"),
                      17UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<engine::api::TableParameters>("1,2;3,4?destinations=1&bla=foo"), 22UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<engine::api::TableParameters>(
                          "1,2;3,4?sources=1&destinations=1&bla=foo"),
                      32UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<engine::api::TableParameters>("1,2;3,4?sources=foo"),
                      16UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<engine::api::TableParameters>("1,2;3,4?destinations=foo"),
                      21UL);
}

BOOST_AUTO_TEST_CASE(valid_route_urls)
{
    std::vector<util::Coordinate> coords_1 = {{util::FloatLongitude(1), util::FloatLatitude(2)},
                                              {util::FloatLongitude(3), util::FloatLatitude(4)}};

    engine::api::RouteParameters reference_1{};
    reference_1.coordinates = coords_1;
    auto result_1 = api::parseParameters<engine::api::RouteParameters>("1,2;3,4");
    BOOST_CHECK(result_1);
    BOOST_CHECK_EQUAL(reference_1.steps, result_1->steps);
    BOOST_CHECK_EQUAL(reference_1.alternatives, result_1->alternatives);
    BOOST_CHECK_EQUAL(reference_1.geometries, result_1->geometries);
    BOOST_CHECK_EQUAL(reference_1.overview, result_1->overview);
    BOOST_CHECK_EQUAL(reference_1.continue_straight, result_1->continue_straight);
    CHECK_EQUAL_RANGE(reference_1.bearings, result_1->bearings);
    CHECK_EQUAL_RANGE(reference_1.radiuses, result_1->radiuses);
    CHECK_EQUAL_RANGE(reference_1.coordinates, result_1->coordinates);

    engine::api::RouteParameters reference_2{};
    reference_2.alternatives = true;
    reference_2.steps = true;
    reference_2.coordinates = coords_1;
    auto result_2 = api::parseParameters<engine::api::RouteParameters>(
        "1,2;3,4?steps=true&alternatives=true&geometries=polyline&overview=simplified");
    BOOST_CHECK(result_2);
    BOOST_CHECK_EQUAL(reference_2.steps, result_2->steps);
    BOOST_CHECK_EQUAL(reference_2.alternatives, result_2->alternatives);
    BOOST_CHECK_EQUAL(reference_2.geometries, result_2->geometries);
    BOOST_CHECK_EQUAL(reference_2.overview, result_2->overview);
    BOOST_CHECK_EQUAL(reference_2.continue_straight, result_2->continue_straight);
    CHECK_EQUAL_RANGE(reference_2.bearings, result_2->bearings);
    CHECK_EQUAL_RANGE(reference_2.radiuses, result_2->radiuses);
    CHECK_EQUAL_RANGE(reference_2.coordinates, result_2->coordinates);

    engine::api::RouteParameters reference_3{
        false, false, engine::api::RouteParameters::GeometriesType::GeoJSON,
        engine::api::RouteParameters::OverviewType::False, true};
    reference_3.coordinates = coords_1;
    auto result_3 = api::parseParameters<engine::api::RouteParameters>(
        "1,2;3,4?steps=false&alternatives=false&geometries=geojson&overview=false&continue_straight=true");
    BOOST_CHECK(result_3);
    BOOST_CHECK_EQUAL(reference_3.steps, result_3->steps);
    BOOST_CHECK_EQUAL(reference_3.alternatives, result_3->alternatives);
    BOOST_CHECK_EQUAL(reference_3.geometries, result_3->geometries);
    BOOST_CHECK_EQUAL(reference_3.overview, result_3->overview);
    BOOST_CHECK_EQUAL(reference_3.continue_straight, result_3->continue_straight);
    CHECK_EQUAL_RANGE(reference_3.bearings, result_3->bearings);
    CHECK_EQUAL_RANGE(reference_3.radiuses, result_3->radiuses);
    CHECK_EQUAL_RANGE(reference_3.coordinates, result_3->coordinates);

    std::vector<boost::optional<engine::Hint>> hints_4 = {
        engine::Hint::FromBase64("DAIAgP___"
                                 "38AAAAAAAAAAAIAAAAAAAAAEAAAAOgDAAD0AwAAGwAAAOUacQBQP5sCshpxAB0_"
                                 "mwIAAAEBl-Umfg=="),
        engine::Hint::FromBase64("cgAAgP___"
                                 "39jAAAADgAAACIAAABeAAAAkQAAANoDAABOAgAAGwAAAFVGcQCiRJsCR0VxAOZFmw"
                                 "IFAAEBl-Umfg=="),
        engine::Hint::FromBase64("3gAAgP___"
                                 "39KAAAAHgAAACEAAAAAAAAAGAAAAE0BAABOAQAAGwAAAIAzcQBkUJsC1zNxAHBQmw"
                                 "IAAAEBl-Umfg==")};
    engine::api::RouteParameters reference_4{false,
                                             false,
                                             engine::api::RouteParameters::GeometriesType::Polyline,
                                             engine::api::RouteParameters::OverviewType::Simplified,
                                             boost::optional<bool>{},
                                             coords_1,
                                             hints_4,
                                             std::vector<boost::optional<double>>{},
                                             std::vector<boost::optional<engine::Bearing>>{}};
    auto result_4 = api::parseParameters<engine::api::RouteParameters>(
        "1,2;3,4?steps=false&hints="
        "DAIAgP___38AAAAAAAAAAAIAAAAAAAAAEAAAAOgDAAD0AwAAGwAAAOUacQBQP5sCshpxAB0_mwIAAAEBl-Umfg==;"
        "cgAAgP___39jAAAADgAAACIAAABeAAAAkQAAANoDAABOAgAAGwAAAFVGcQCiRJsCR0VxAOZFmwIFAAEBl-Umfg==;"
        "3gAAgP___39KAAAAHgAAACEAAAAAAAAAGAAAAE0BAABOAQAAGwAAAIAzcQBkUJsC1zNxAHBQmwIAAAEBl-Umfg==");
    BOOST_CHECK(result_4);
    BOOST_CHECK_EQUAL(reference_4.steps, result_4->steps);
    BOOST_CHECK_EQUAL(reference_4.alternatives, result_4->alternatives);
    BOOST_CHECK_EQUAL(reference_4.geometries, result_4->geometries);
    BOOST_CHECK_EQUAL(reference_4.overview, result_4->overview);
    BOOST_CHECK_EQUAL(reference_4.continue_straight, result_4->continue_straight);
    CHECK_EQUAL_RANGE(reference_4.bearings, result_4->bearings);
    CHECK_EQUAL_RANGE(reference_4.radiuses, result_4->radiuses);
    CHECK_EQUAL_RANGE(reference_4.coordinates, result_4->coordinates);

    std::vector<boost::optional<engine::Bearing>> bearings_4 = {
        boost::none, engine::Bearing{200, 10}, engine::Bearing{100, 5},
    };
    engine::api::RouteParameters reference_5{false,
                                             false,
                                             engine::api::RouteParameters::GeometriesType::Polyline,
                                             engine::api::RouteParameters::OverviewType::Simplified,
                                             boost::optional<bool>{},
                                             coords_1,
                                             std::vector<boost::optional<engine::Hint>>{},
                                             std::vector<boost::optional<double>>{},
                                             bearings_4};
    auto result_5 = api::parseParameters<engine::api::RouteParameters>(
        "1,2;3,4?steps=false&bearings=;200,10;100,5");
    BOOST_CHECK(result_5);
    BOOST_CHECK_EQUAL(reference_5.steps, result_5->steps);
    BOOST_CHECK_EQUAL(reference_5.alternatives, result_5->alternatives);
    BOOST_CHECK_EQUAL(reference_5.geometries, result_5->geometries);
    BOOST_CHECK_EQUAL(reference_5.overview, result_5->overview);
    BOOST_CHECK_EQUAL(reference_5.continue_straight, result_5->continue_straight);
    CHECK_EQUAL_RANGE(reference_5.bearings, result_5->bearings);
    CHECK_EQUAL_RANGE(reference_5.radiuses, result_5->radiuses);
    CHECK_EQUAL_RANGE(reference_5.coordinates, result_5->coordinates);

    std::vector<util::Coordinate> coords_2 = {{util::FloatLongitude(0), util::FloatLatitude(1)},
                                              {util::FloatLongitude(2), util::FloatLatitude(3)},
                                              {util::FloatLongitude(4), util::FloatLatitude(5)}};

    engine::api::RouteParameters reference_6{};
    reference_6.coordinates = coords_2;
    auto result_6 =
        api::parseParameters<engine::api::RouteParameters>("polyline(_ibE?_seK_seK_seK_seK)");
    BOOST_CHECK(result_6);
    BOOST_CHECK_EQUAL(reference_6.steps, result_6->steps);
    BOOST_CHECK_EQUAL(reference_6.alternatives, result_6->alternatives);
    BOOST_CHECK_EQUAL(reference_6.geometries, result_6->geometries);
    BOOST_CHECK_EQUAL(reference_6.overview, result_6->overview);
    BOOST_CHECK_EQUAL(reference_6.continue_straight, result_6->continue_straight);
    CHECK_EQUAL_RANGE(reference_6.bearings, result_6->bearings);
    CHECK_EQUAL_RANGE(reference_6.radiuses, result_6->radiuses);
    CHECK_EQUAL_RANGE(reference_6.coordinates, result_6->coordinates);
}

BOOST_AUTO_TEST_CASE(valid_table_urls)
{
    std::vector<util::Coordinate> coords_1 = {{util::FloatLongitude(1), util::FloatLatitude(2)},
                                              {util::FloatLongitude(3), util::FloatLatitude(4)}};

    engine::api::TableParameters reference_1{};
    reference_1.coordinates = coords_1;
    auto result_1 = api::parseParameters<engine::api::TableParameters>("1,2;3,4");
    BOOST_CHECK(result_1);
    CHECK_EQUAL_RANGE(reference_1.sources, result_1->sources);
    CHECK_EQUAL_RANGE(reference_1.destinations, result_1->destinations);
    CHECK_EQUAL_RANGE(reference_1.bearings, result_1->bearings);
    CHECK_EQUAL_RANGE(reference_1.radiuses, result_1->radiuses);
    CHECK_EQUAL_RANGE(reference_1.coordinates, result_1->coordinates);

    std::vector<std::size_t> sources_2 = {1, 2, 3};
    std::vector<std::size_t> destinations_2 = {4, 5};
    engine::api::TableParameters reference_2{sources_2, destinations_2};
    reference_2.coordinates = coords_1;
    auto result_2 = api::parseParameters<engine::api::TableParameters>(
        "1,2;3,4?sources=1;2;3&destinations=4;5");
    BOOST_CHECK(result_2);
    CHECK_EQUAL_RANGE(reference_2.sources, result_2->sources);
    CHECK_EQUAL_RANGE(reference_2.destinations, result_2->destinations);
    CHECK_EQUAL_RANGE(reference_2.bearings, result_2->bearings);
    CHECK_EQUAL_RANGE(reference_2.radiuses, result_2->radiuses);
    CHECK_EQUAL_RANGE(reference_2.coordinates, result_2->coordinates);
}

BOOST_AUTO_TEST_CASE(valid_match_urls)
{
    std::vector<util::Coordinate> coords_1 = {{util::FloatLongitude(1), util::FloatLatitude(2)},
                                              {util::FloatLongitude(3), util::FloatLatitude(4)}};

    engine::api::MatchParameters reference_1{};
    reference_1.coordinates = coords_1;
    auto result_1 = api::parseParameters<engine::api::MatchParameters>("1,2;3,4");
    BOOST_CHECK(result_1);
    CHECK_EQUAL_RANGE(reference_1.timestamps, result_1->timestamps);
    CHECK_EQUAL_RANGE(reference_1.bearings, result_1->bearings);
    CHECK_EQUAL_RANGE(reference_1.radiuses, result_1->radiuses);
    CHECK_EQUAL_RANGE(reference_1.coordinates, result_1->coordinates);
}

BOOST_AUTO_TEST_CASE(valid_nearest_urls)
{
    std::vector<util::Coordinate> coords_1 = {{util::FloatLongitude(1), util::FloatLatitude(2)}};

    engine::api::NearestParameters reference_1{};
    reference_1.coordinates = coords_1;
    auto result_1 = api::parseParameters<engine::api::NearestParameters>("1,2");
    BOOST_CHECK(result_1);
    BOOST_CHECK_EQUAL(reference_1.number_of_results, result_1->number_of_results);
    CHECK_EQUAL_RANGE(reference_1.bearings, result_1->bearings);
    CHECK_EQUAL_RANGE(reference_1.radiuses, result_1->radiuses);
    CHECK_EQUAL_RANGE(reference_1.coordinates, result_1->coordinates);
}

BOOST_AUTO_TEST_CASE(valid_tile_urls)
{
    engine::api::TileParameters reference_1{1, 2, 3};
    auto result_1 = api::parseParameters<engine::api::TileParameters>("tile(1,2,3).mvt");
    BOOST_CHECK(result_1);
    BOOST_CHECK_EQUAL(reference_1.x, result_1->x);
    BOOST_CHECK_EQUAL(reference_1.y, result_1->y);
    BOOST_CHECK_EQUAL(reference_1.z, result_1->z);
}

BOOST_AUTO_TEST_CASE(valid_trip_urls)
{
    std::vector<util::Coordinate> coords_1 = {{util::FloatLongitude(1), util::FloatLatitude(2)},
                                              {util::FloatLongitude(3), util::FloatLatitude(4)}};

    engine::api::TripParameters reference_1{};
    reference_1.coordinates = coords_1;
    auto result_1 = api::parseParameters<engine::api::TripParameters>("1,2;3,4");
    BOOST_CHECK(result_1);
    CHECK_EQUAL_RANGE(reference_1.bearings, result_1->bearings);
    CHECK_EQUAL_RANGE(reference_1.radiuses, result_1->radiuses);
    CHECK_EQUAL_RANGE(reference_1.coordinates, result_1->coordinates);
}

BOOST_AUTO_TEST_SUITE_END()
