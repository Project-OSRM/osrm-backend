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
using namespace osrm::server::api;
using namespace osrm::engine::api;

// returns distance to front
template <typename ParameterT> std::size_t testInvalidOptions(std::string options)
{
    auto iter = options.begin();
    auto result = parseParameters<ParameterT>(iter, options.end());
    BOOST_CHECK(!result);
    return std::distance(options.begin(), iter);
}

BOOST_AUTO_TEST_CASE(invalid_route_urls)
{
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("a;3,4"), 0UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("120;3,4"), 3UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("90000000,2;3,4"), 0UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3,4?overview=false&bla=foo"), 22UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3,4?overview=false&bearings=foo"),
                      32UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<RouteParameters>("1,2;3,4?overview=false&continue_straight=foo"), 41UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3,4?overview=false&radiuses=foo"),
                      32UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3,4?overview=false&hints=foo"),
                      29UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3,4?overview=false&hints=;;; ;"),
                      32UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3,4?generate_hints=notboolean"),
                      23UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3,4?overview=false&geometries=foo"),
                      34UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3,4?overview=false&overview=foo"),
                      32L);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<RouteParameters>("1,2;3,4?overview=false&alternatives=foo"), 36UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>(""), 0);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3.4.unsupported"), 7);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3,4.json?nooptions"), 13);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3,4..json?nooptions"), 14);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3,4.0.json?nooptions"), 15);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>(std::string{"1,2;3,4"} + '\0' + ".json"),
                      7);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>(std::string{"1,2;3,"} + '\0'), 6);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3,4?annotations=distances"), 28UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3,4?annotations="), 20UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>("1,2;3,4?annotations=true,false"), 24UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<RouteParameters>("1,2;3,4?annotations=&overview=simplified"), 20UL);

    // BOOST_CHECK_EQUAL(testInvalidOptions<RouteParameters>(), );
}

BOOST_AUTO_TEST_CASE(invalid_table_urls)
{
    BOOST_CHECK_EQUAL(testInvalidOptions<TableParameters>("1,2;3,4?sources=1&bla=foo"), 17UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<TableParameters>("1,2;3,4?destinations=1&bla=foo"), 22UL);
    BOOST_CHECK_EQUAL(
        testInvalidOptions<TableParameters>("1,2;3,4?sources=1&destinations=1&bla=foo"), 32UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<TableParameters>("1,2;3,4?sources=foo"), 16UL);
    BOOST_CHECK_EQUAL(testInvalidOptions<TableParameters>("1,2;3,4?destinations=foo"), 21UL);
}

BOOST_AUTO_TEST_CASE(valid_route_hint)
{
    auto hint = engine::Hint::FromBase64("XAYAgP___3-"
                                         "QAAAABQAAACgAAABMAAAAJQAAAAUAAAAoAAAATAAAACUAAAB7BQAAFAAA"
                                         "AGpocQC3gpsCO2hxAMuCmwIFAAEBDJujEQ==");
    BOOST_CHECK_EQUAL(
        hint.phantom.input_location,
        util::Coordinate(util::FloatLongitude{7.432251}, util::FloatLatitude{43.745995}));
}

BOOST_AUTO_TEST_CASE(valid_route_urls)
{
    std::vector<util::Coordinate> coords_1 = {{util::FloatLongitude{1}, util::FloatLatitude{2}},
                                              {util::FloatLongitude{3}, util::FloatLatitude{4}}};

    RouteParameters reference_1{};
    reference_1.coordinates = coords_1;
    auto result_1 = parseParameters<RouteParameters>("1,2;3,4");
    BOOST_CHECK(result_1);
    BOOST_CHECK_EQUAL(reference_1.steps, result_1->steps);
    BOOST_CHECK_EQUAL(reference_1.alternatives, result_1->alternatives);
    BOOST_CHECK_EQUAL(reference_1.geometries, result_1->geometries);
    BOOST_CHECK_EQUAL(reference_1.annotations, result_1->annotations);
    BOOST_CHECK_EQUAL(reference_1.overview, result_1->overview);
    BOOST_CHECK_EQUAL(reference_1.continue_straight, result_1->continue_straight);
    CHECK_EQUAL_RANGE(reference_1.bearings, result_1->bearings);
    CHECK_EQUAL_RANGE(reference_1.radiuses, result_1->radiuses);
    CHECK_EQUAL_RANGE(reference_1.coordinates, result_1->coordinates);
    CHECK_EQUAL_RANGE(reference_1.hints, result_1->hints);

    RouteParameters reference_2{};
    reference_2.alternatives = true;
    reference_2.steps = true;
    reference_2.annotations = true;
    reference_2.coordinates = coords_1;
    auto result_2 =
        parseParameters<RouteParameters>("1,2;3,4?steps=true&alternatives=true&geometries=polyline&"
                                         "overview=simplified&annotations=true");
    BOOST_CHECK(result_2);
    BOOST_CHECK_EQUAL(reference_2.steps, result_2->steps);
    BOOST_CHECK_EQUAL(reference_2.alternatives, result_2->alternatives);
    BOOST_CHECK_EQUAL(reference_2.geometries, result_2->geometries);
    BOOST_CHECK_EQUAL(reference_2.annotations, result_2->annotations);
    BOOST_CHECK_EQUAL(reference_2.overview, result_2->overview);
    BOOST_CHECK_EQUAL(reference_2.continue_straight, result_2->continue_straight);
    CHECK_EQUAL_RANGE(reference_2.bearings, result_2->bearings);
    CHECK_EQUAL_RANGE(reference_2.radiuses, result_2->radiuses);
    CHECK_EQUAL_RANGE(reference_2.coordinates, result_2->coordinates);
    CHECK_EQUAL_RANGE(reference_2.hints, result_2->hints);
    BOOST_CHECK_EQUAL(result_2->annotations_type == RouteParameters::AnnotationsType::All, true);

    RouteParameters reference_3{false,
                                false,
                                false,
                                RouteParameters::GeometriesType::GeoJSON,
                                RouteParameters::OverviewType::False,
                                true};
    reference_3.coordinates = coords_1;
    auto result_3 = api::parseParameters<engine::api::RouteParameters>(
        "1,2;3,4?steps=false&alternatives=false&geometries=geojson&overview=false&continue_"
        "straight=true");
    BOOST_CHECK(result_3);
    BOOST_CHECK_EQUAL(reference_3.steps, result_3->steps);
    BOOST_CHECK_EQUAL(reference_3.alternatives, result_3->alternatives);
    BOOST_CHECK_EQUAL(reference_3.geometries, result_3->geometries);
    BOOST_CHECK_EQUAL(reference_3.annotations, result_3->annotations);
    BOOST_CHECK_EQUAL(reference_3.overview, result_3->overview);
    BOOST_CHECK_EQUAL(reference_3.continue_straight, result_3->continue_straight);
    CHECK_EQUAL_RANGE(reference_3.bearings, result_3->bearings);
    CHECK_EQUAL_RANGE(reference_3.radiuses, result_3->radiuses);
    CHECK_EQUAL_RANGE(reference_3.coordinates, result_3->coordinates);
    CHECK_EQUAL_RANGE(reference_3.hints, result_3->hints);

    std::vector<boost::optional<engine::Hint>> hints_4 = {
        engine::Hint::FromBase64("XAYAgP___3-"
                                 "QAAAABQAAACgAAABMAAAAJQAAAAUAAAAoAAAATAAAACUAAAB7BQAAFAAAAGpocQC3"
                                 "gpsCO2hxAMuCmwIFAAEBDJujEQ=="),
        engine::Hint::FromBase64("lgQAgP___3-"
                                 "QAAAAEwAAABgAAAAyAAAAOAAAABMAAAAYAAAAMgAAADgAAAA0AAAAFAAAAJphcQC-"
                                 "c5sC3GFxALtzmwIEAAEBDJujEQ=="),
        engine::Hint::FromBase64("OAUAgMUFAIAAAAAAHgAAAAUAAAAAAAAALQEAAB4AAAAFAAAAAAAAAC0BAAC8BAAAF"
                                 "AAAAIM-cQBQVpsCiD5xAFBWmwIAAAEBDJujEQ==")};
    RouteParameters reference_4{false,
                                false,
                                false,
                                RouteParameters::GeometriesType::Polyline,
                                RouteParameters::OverviewType::Simplified,
                                boost::optional<bool>{},
                                coords_1,
                                hints_4,
                                std::vector<boost::optional<double>>{},
                                std::vector<boost::optional<engine::Bearing>>{}};
    auto result_4 = parseParameters<RouteParameters>(
        "1,2;3,4?steps=false&hints="
        "XAYAgP___3-"
        "QAAAABQAAACgAAABMAAAAJQAAAAUAAAAoAAAATAAAACUAAAB7BQAAFAAAAGpocQC3gpsCO2hxAMuCmwIFAAEBDJujE"
        "Q==;"
        "lgQAgP___3-QAAAAEwAAABgAAAAyAAAAOAAAABMAAAAYAAAAMgAAADgAAAA0AAAAFAAAAJphcQC-"
        "c5sC3GFxALtzmwIEAAEBDJujEQ==;"
        "OAUAgMUFAIAAAAAAHgAAAAUAAAAAAAAALQEAAB4AAAAFAAAAAAAAAC0BAAC8BAAAFAAAAIM-"
        "cQBQVpsCiD5xAFBWmwIAAAEBDJujEQ==");
    BOOST_CHECK(result_4);
    BOOST_CHECK_EQUAL(reference_4.steps, result_4->steps);
    BOOST_CHECK_EQUAL(reference_4.alternatives, result_4->alternatives);
    BOOST_CHECK_EQUAL(reference_4.geometries, result_4->geometries);
    BOOST_CHECK_EQUAL(reference_4.annotations, result_4->annotations);
    BOOST_CHECK_EQUAL(reference_4.overview, result_4->overview);
    BOOST_CHECK_EQUAL(reference_4.continue_straight, result_4->continue_straight);
    CHECK_EQUAL_RANGE(reference_4.bearings, result_4->bearings);
    CHECK_EQUAL_RANGE(reference_4.radiuses, result_4->radiuses);
    CHECK_EQUAL_RANGE(reference_4.coordinates, result_4->coordinates);
    CHECK_EQUAL_RANGE(reference_4.hints, result_4->hints);

    std::vector<boost::optional<engine::Bearing>> bearings_4 = {
        boost::none, engine::Bearing{200, 10}, engine::Bearing{100, 5},
    };
    RouteParameters reference_5{false,
                                false,
                                false,
                                RouteParameters::GeometriesType::Polyline,
                                RouteParameters::OverviewType::Simplified,
                                boost::optional<bool>{},
                                coords_1,
                                std::vector<boost::optional<engine::Hint>>{},
                                std::vector<boost::optional<double>>{},
                                bearings_4};
    auto result_5 = parseParameters<RouteParameters>("1,2;3,4?steps=false&bearings=;200,10;100,5");
    BOOST_CHECK(result_5);
    BOOST_CHECK_EQUAL(reference_5.steps, result_5->steps);
    BOOST_CHECK_EQUAL(reference_5.alternatives, result_5->alternatives);
    BOOST_CHECK_EQUAL(reference_5.geometries, result_5->geometries);
    BOOST_CHECK_EQUAL(reference_5.annotations, result_5->annotations);
    BOOST_CHECK_EQUAL(reference_5.overview, result_5->overview);
    BOOST_CHECK_EQUAL(reference_5.continue_straight, result_5->continue_straight);
    CHECK_EQUAL_RANGE(reference_5.bearings, result_5->bearings);
    CHECK_EQUAL_RANGE(reference_5.radiuses, result_5->radiuses);
    CHECK_EQUAL_RANGE(reference_5.coordinates, result_5->coordinates);
    CHECK_EQUAL_RANGE(reference_5.hints, result_5->hints);

    std::vector<util::Coordinate> coords_2 = {{util::FloatLongitude{0}, util::FloatLatitude{1}},
                                              {util::FloatLongitude{2}, util::FloatLatitude{3}},
                                              {util::FloatLongitude{4}, util::FloatLatitude{5}}};

    RouteParameters reference_6{};
    reference_6.coordinates = coords_2;
    auto result_6 = parseParameters<RouteParameters>("polyline(_ibE?_seK_seK_seK_seK)");
    BOOST_CHECK(result_6);
    BOOST_CHECK_EQUAL(reference_6.steps, result_6->steps);
    BOOST_CHECK_EQUAL(reference_6.alternatives, result_6->alternatives);
    BOOST_CHECK_EQUAL(reference_6.geometries, result_6->geometries);
    BOOST_CHECK_EQUAL(reference_6.annotations, result_6->annotations);
    BOOST_CHECK_EQUAL(reference_6.overview, result_6->overview);
    BOOST_CHECK_EQUAL(reference_6.continue_straight, result_6->continue_straight);
    CHECK_EQUAL_RANGE(reference_6.bearings, result_6->bearings);
    CHECK_EQUAL_RANGE(reference_6.radiuses, result_6->radiuses);
    CHECK_EQUAL_RANGE(reference_6.coordinates, result_6->coordinates);
    CHECK_EQUAL_RANGE(reference_6.hints, result_6->hints);

    auto result_7 = parseParameters<RouteParameters>("1,2;3,4?radiuses=;unlimited");
    RouteParameters reference_7{};
    reference_7.coordinates = coords_1;
    reference_7.radiuses = {boost::none,
                            boost::make_optional(std::numeric_limits<double>::infinity())};
    BOOST_CHECK(result_7);
    BOOST_CHECK_EQUAL(reference_7.steps, result_7->steps);
    BOOST_CHECK_EQUAL(reference_7.alternatives, result_7->alternatives);
    BOOST_CHECK_EQUAL(reference_7.geometries, result_7->geometries);
    BOOST_CHECK_EQUAL(reference_7.annotations, result_7->annotations);
    BOOST_CHECK_EQUAL(reference_7.overview, result_7->overview);
    BOOST_CHECK_EQUAL(reference_7.continue_straight, result_7->continue_straight);
    CHECK_EQUAL_RANGE(reference_7.bearings, result_7->bearings);
    CHECK_EQUAL_RANGE(reference_7.radiuses, result_7->radiuses);
    CHECK_EQUAL_RANGE(reference_7.coordinates, result_7->coordinates);
    CHECK_EQUAL_RANGE(reference_7.hints, result_7->hints);

    auto result_8 = parseParameters<RouteParameters>("1,2;3,4?radiuses=;");
    RouteParameters reference_8{};
    reference_8.coordinates = coords_1;
    reference_8.radiuses = {boost::none, boost::none};
    BOOST_CHECK(result_8);
    CHECK_EQUAL_RANGE(reference_8.radiuses, result_8->radiuses);

    auto result_9 = parseParameters<RouteParameters>("1,2?radiuses=");
    RouteParameters reference_9{};
    reference_9.coordinates = coords_1;
    reference_9.radiuses = {boost::none};
    BOOST_CHECK(result_9);
    CHECK_EQUAL_RANGE(reference_9.radiuses, result_9->radiuses);

    // Some Hint's are empty
    std::vector<util::Coordinate> coords_3 = {{util::FloatLongitude{1}, util::FloatLatitude{2}},
                                              {util::FloatLongitude{3}, util::FloatLatitude{4}},
                                              {util::FloatLongitude{5}, util::FloatLatitude{6}},
                                              {util::FloatLongitude{7}, util::FloatLatitude{8}}};
    std::vector<boost::optional<engine::Hint>> hints_10 = {
        engine::Hint::FromBase64("XAYAgP___3-"
                                 "QAAAABQAAACgAAABMAAAAJQAAAAUAAAAoAAAATAAAACUAAAB7BQAAFAAAAGpocQC3"
                                 "gpsCO2hxAMuCmwIFAAEBDJujEQ=="),
        boost::none,
        engine::Hint::FromBase64("OAUAgMUFAIAAAAAAHgAAAAUAAAAAAAAALQEAAB4AAAAFAAAAAAAAAC0BAAC8BAAAF"
                                 "AAAAIM-cQBQVpsCiD5xAFBWmwIAAAEBDJujEQ=="),
        boost::none};
    RouteParameters reference_10{false,
                                 false,
                                 false,
                                 RouteParameters::GeometriesType::Polyline,
                                 RouteParameters::OverviewType::Simplified,
                                 boost::optional<bool>{},
                                 coords_3,
                                 hints_10,
                                 std::vector<boost::optional<double>>{},
                                 std::vector<boost::optional<engine::Bearing>>{}};
    auto result_10 =
        parseParameters<RouteParameters>("1,2;3,4;5,6;7,8?steps=false&hints="
                                         "XAYAgP___3-"
                                         "QAAAABQAAACgAAABMAAAAJQAAAAUAAAAoAAAATAAAACUAAAB7BQAAFAAA"
                                         "AGpocQC3gpsCO2hxAMuCmwIFAAEBDJujEQ==;;"
                                         "OAUAgMUFAIAAAAAAHgAAAAUAAAAAAAAALQEAAB4AAAAFAAAAAAAAAC0BA"
                                         "AC8BAAAFAAAAIM-cQBQVpsCiD5xAFBWmwIAAAEBDJujEQ==;");
    BOOST_CHECK(result_10);
    BOOST_CHECK_EQUAL(reference_10.steps, result_10->steps);
    BOOST_CHECK_EQUAL(reference_10.alternatives, result_10->alternatives);
    BOOST_CHECK_EQUAL(reference_10.geometries, result_10->geometries);
    BOOST_CHECK_EQUAL(reference_10.annotations, result_10->annotations);
    BOOST_CHECK_EQUAL(reference_10.overview, result_10->overview);
    BOOST_CHECK_EQUAL(reference_10.continue_straight, result_10->continue_straight);
    CHECK_EQUAL_RANGE(reference_10.bearings, result_10->bearings);
    CHECK_EQUAL_RANGE(reference_10.radiuses, result_10->radiuses);
    CHECK_EQUAL_RANGE(reference_10.coordinates, result_10->coordinates);
    CHECK_EQUAL_RANGE(reference_10.hints, result_10->hints);

    // Do not generate Hints when they are explicitly disabled
    auto result_11 = parseParameters<RouteParameters>("1,2;3,4?generate_hints=false");
    BOOST_CHECK(result_11);
    BOOST_CHECK_EQUAL(result_11->generate_hints, false);

    auto result_12 = parseParameters<RouteParameters>("1,2;3,4?generate_hints=true");
    BOOST_CHECK(result_12);
    BOOST_CHECK_EQUAL(result_12->generate_hints, true);

    auto result_13 = parseParameters<RouteParameters>("1,2;3,4");
    BOOST_CHECK(result_13);
    BOOST_CHECK_EQUAL(result_13->generate_hints, true);

    // parse none annotations value correctly
    RouteParameters reference_14{};
    reference_14.annotations_type = RouteParameters::AnnotationsType::None;
    reference_14.coordinates = coords_1;
    auto result_14 = parseParameters<RouteParameters>("1,2;3,4?geometries=polyline");
    BOOST_CHECK(result_14);
    BOOST_CHECK_EQUAL(reference_14.geometries, result_14->geometries);
    BOOST_CHECK_EQUAL(result_14->annotations_type == RouteParameters::AnnotationsType::None, true);
    BOOST_CHECK_EQUAL(result_14->annotations, false);

    // parse single annotations value correctly
    RouteParameters reference_15{};
    reference_15.annotations_type = RouteParameters::AnnotationsType::Duration;
    reference_15.coordinates = coords_1;
    auto result_15 = parseParameters<RouteParameters>("1,2;3,4?geometries=polyline&"
                                                      "overview=simplified&annotations=duration");
    BOOST_CHECK(result_15);
    BOOST_CHECK_EQUAL(reference_15.geometries, result_15->geometries);
    BOOST_CHECK_EQUAL(result_15->annotations_type == RouteParameters::AnnotationsType::Duration,
                      true);
    BOOST_CHECK_EQUAL(result_15->annotations, true);

    RouteParameters reference_speed{};
    reference_speed.annotations_type = RouteParameters::AnnotationsType::Duration;
    reference_speed.coordinates = coords_1;
    auto result_speed =
        parseParameters<RouteParameters>("1,2;3,4?geometries=polyline&"
                                         "overview=simplified&annotations=duration,distance,speed");
    BOOST_CHECK(result_speed);
    BOOST_CHECK_EQUAL(reference_speed.geometries, result_speed->geometries);
    BOOST_CHECK_EQUAL(reference_speed.overview, result_speed->overview);
    BOOST_CHECK_EQUAL(result_speed->annotations_type ==
                          (RouteParameters::AnnotationsType::Duration |
                           RouteParameters::AnnotationsType::Distance |
                           RouteParameters::AnnotationsType::Speed),
                      true);
    BOOST_CHECK_EQUAL(result_speed->annotations, true);

    // parse multiple annotations correctly
    RouteParameters reference_16{};
    reference_16.annotations_type = RouteParameters::AnnotationsType::Duration |
                                    RouteParameters::AnnotationsType::Weight |
                                    RouteParameters::AnnotationsType::Nodes;
    reference_16.coordinates = coords_1;
    auto result_16 =
        parseParameters<RouteParameters>("1,2;3,4?geometries=polyline&"
                                         "overview=simplified&annotations=duration,weight,nodes");
    BOOST_CHECK(result_16);
    BOOST_CHECK_EQUAL(reference_16.geometries, result_16->geometries);
    BOOST_CHECK_EQUAL(
        static_cast<bool>(result_2->annotations_type & (RouteParameters::AnnotationsType::Weight |
                                                        RouteParameters::AnnotationsType::Duration |
                                                        RouteParameters::AnnotationsType::Nodes)),
        true);
    BOOST_CHECK_EQUAL(result_16->annotations, true);

    // parse all annotations correctly
    RouteParameters reference_17{};
    reference_17.annotations_type = RouteParameters::AnnotationsType::All;
    reference_17.coordinates = coords_1;
    auto result_17 = parseParameters<RouteParameters>(
        "1,2;3,4?overview=simplified&annotations=duration,weight,nodes,datasources,distance");
    BOOST_CHECK(result_17);
    BOOST_CHECK_EQUAL(reference_17.geometries, result_17->geometries);
    BOOST_CHECK_EQUAL(result_2->annotations_type == RouteParameters::AnnotationsType::All, true);
    BOOST_CHECK_EQUAL(result_17->annotations, true);
}

BOOST_AUTO_TEST_CASE(valid_table_urls)
{
    std::vector<util::Coordinate> coords_1 = {{util::FloatLongitude{1}, util::FloatLatitude{2}},
                                              {util::FloatLongitude{3}, util::FloatLatitude{4}}};

    TableParameters reference_1{};
    reference_1.coordinates = coords_1;
    auto result_1 = parseParameters<TableParameters>("1,2;3,4");
    BOOST_CHECK(result_1);
    CHECK_EQUAL_RANGE(reference_1.sources, result_1->sources);
    CHECK_EQUAL_RANGE(reference_1.destinations, result_1->destinations);
    CHECK_EQUAL_RANGE(reference_1.bearings, result_1->bearings);
    CHECK_EQUAL_RANGE(reference_1.radiuses, result_1->radiuses);
    CHECK_EQUAL_RANGE(reference_1.coordinates, result_1->coordinates);

    std::vector<std::size_t> sources_2 = {1, 2, 3};
    std::vector<std::size_t> destinations_2 = {4, 5};
    TableParameters reference_2{sources_2, destinations_2};
    reference_2.coordinates = coords_1;
    auto result_2 = parseParameters<TableParameters>("1,2;3,4?sources=1;2;3&destinations=4;5");
    BOOST_CHECK(result_2);
    CHECK_EQUAL_RANGE(reference_2.sources, result_2->sources);
    CHECK_EQUAL_RANGE(reference_2.destinations, result_2->destinations);
    CHECK_EQUAL_RANGE(reference_2.bearings, result_2->bearings);
    CHECK_EQUAL_RANGE(reference_2.radiuses, result_2->radiuses);
    CHECK_EQUAL_RANGE(reference_2.coordinates, result_2->coordinates);

    auto result_3 = parseParameters<TableParameters>("1,2;3,4?sources=all&destinations=all");
    BOOST_CHECK(result_3);
    CHECK_EQUAL_RANGE(reference_1.sources, result_3->sources);
    CHECK_EQUAL_RANGE(reference_1.destinations, result_3->destinations);
    CHECK_EQUAL_RANGE(reference_1.bearings, result_3->bearings);
    CHECK_EQUAL_RANGE(reference_1.radiuses, result_3->radiuses);
    CHECK_EQUAL_RANGE(reference_1.coordinates, result_3->coordinates);
}

BOOST_AUTO_TEST_CASE(valid_match_urls)
{
    std::vector<util::Coordinate> coords_1 = {{util::FloatLongitude{1}, util::FloatLatitude{2}},
                                              {util::FloatLongitude{3}, util::FloatLatitude{4}}};

    MatchParameters reference_1{};
    reference_1.coordinates = coords_1;
    auto result_1 = parseParameters<MatchParameters>("1,2;3,4");
    BOOST_CHECK(result_1);
    CHECK_EQUAL_RANGE(reference_1.timestamps, result_1->timestamps);
    CHECK_EQUAL_RANGE(reference_1.bearings, result_1->bearings);
    CHECK_EQUAL_RANGE(reference_1.radiuses, result_1->radiuses);
    CHECK_EQUAL_RANGE(reference_1.coordinates, result_1->coordinates);

    MatchParameters reference_2{};
    reference_2.coordinates = coords_1;
    reference_2.timestamps = {5, 6};
    auto result_2 = parseParameters<MatchParameters>("1,2;3,4?timestamps=5;6");
    BOOST_CHECK(result_2);
    CHECK_EQUAL_RANGE(reference_2.timestamps, result_2->timestamps);
    CHECK_EQUAL_RANGE(reference_2.bearings, result_2->bearings);
    CHECK_EQUAL_RANGE(reference_2.radiuses, result_2->radiuses);
    CHECK_EQUAL_RANGE(reference_2.coordinates, result_2->coordinates);
}

BOOST_AUTO_TEST_CASE(valid_nearest_urls)
{
    std::vector<util::Coordinate> coords_1 = {{util::FloatLongitude{1}, util::FloatLatitude{2}}};

    NearestParameters reference_1{};
    reference_1.coordinates = coords_1;
    auto result_1 = parseParameters<NearestParameters>("1,2");
    BOOST_CHECK(result_1);
    BOOST_CHECK_EQUAL(reference_1.number_of_results, result_1->number_of_results);
    CHECK_EQUAL_RANGE(reference_1.bearings, result_1->bearings);
    CHECK_EQUAL_RANGE(reference_1.radiuses, result_1->radiuses);
    CHECK_EQUAL_RANGE(reference_1.coordinates, result_1->coordinates);

    NearestParameters reference_2{};
    reference_2.coordinates = coords_1;
    reference_2.number_of_results = 42;
    auto result_2 = parseParameters<NearestParameters>("1,2?number=42");
    BOOST_CHECK(result_2);
    BOOST_CHECK_EQUAL(reference_2.number_of_results, result_2->number_of_results);
    CHECK_EQUAL_RANGE(reference_2.bearings, result_2->bearings);
    CHECK_EQUAL_RANGE(reference_2.radiuses, result_2->radiuses);
    CHECK_EQUAL_RANGE(reference_2.coordinates, result_2->coordinates);
}

BOOST_AUTO_TEST_CASE(invalid_tile_urls)
{
    TileParameters reference_1{1, 2, 3};
    auto result_1 = parseParameters<TileParameters>("tile(1,2,3).mvt");
    BOOST_CHECK(result_1);
    BOOST_CHECK(!result_1->IsValid());
    BOOST_CHECK_EQUAL(reference_1.x, result_1->x);
    BOOST_CHECK_EQUAL(reference_1.y, result_1->y);
    BOOST_CHECK_EQUAL(reference_1.z, result_1->z);
}

BOOST_AUTO_TEST_CASE(valid_tile_urls)
{
    TileParameters reference_1{1, 2, 12};
    auto result_1 = parseParameters<TileParameters>("tile(1,2,12).mvt");
    BOOST_CHECK(result_1->IsValid());
    BOOST_CHECK(result_1);
    BOOST_CHECK_EQUAL(reference_1.x, result_1->x);
    BOOST_CHECK_EQUAL(reference_1.y, result_1->y);
    BOOST_CHECK_EQUAL(reference_1.z, result_1->z);
}

BOOST_AUTO_TEST_CASE(valid_trip_urls)
{
    std::vector<util::Coordinate> coords_1 = {{util::FloatLongitude{1}, util::FloatLatitude{2}},
                                              {util::FloatLongitude{3}, util::FloatLatitude{4}}};

    TripParameters reference_1{};
    reference_1.coordinates = coords_1;
    auto result_1 = parseParameters<TripParameters>("1,2;3,4");
    BOOST_CHECK(result_1);
    CHECK_EQUAL_RANGE(reference_1.radiuses, result_1->radiuses);
    CHECK_EQUAL_RANGE(reference_1.coordinates, result_1->coordinates);

    TripParameters reference_2{};
    reference_2.coordinates = coords_1;
    reference_2.source = TripParameters::SourceType::First;
    reference_2.destination = TripParameters::DestinationType::Last;
    auto result_2 = parseParameters<TripParameters>("1,2;3,4?source=first&destination=last");
    BOOST_CHECK(result_2);
    CHECK_EQUAL_RANGE(reference_2.radiuses, result_2->radiuses);
    CHECK_EQUAL_RANGE(reference_2.coordinates, result_2->coordinates);

    // check supported source/destination/rountrip combinations
    auto param_fse_r =
        parseParameters<TripParameters>("1,2;3,4?source=first&destination=last&roundtrip=true");
    BOOST_CHECK(param_fse_r->IsValid());
    auto param_fse_nr_ =
        parseParameters<TripParameters>("1,2;3,4?source=first&destination=last&roundtrip=false");
    BOOST_CHECK(param_fse_nr_->IsValid());
    auto param_fs_r = parseParameters<TripParameters>("1,2;3,4?source=first&roundtrip=true");
    BOOST_CHECK(param_fs_r->IsValid());
    auto param_fs_nr = parseParameters<TripParameters>("1,2;3,4?source=first&roundtrip=false");
    BOOST_CHECK(param_fs_nr->IsValid());
    auto param_fe_r = parseParameters<TripParameters>("1,2;3,4?destination=last&roundtrip=true");
    BOOST_CHECK(param_fe_r->IsValid());
    auto param_fe_nr = parseParameters<TripParameters>("1,2;3,4?destination=last&roundtrip=false");
    BOOST_CHECK(param_fe_nr->IsValid());
    auto param_r = parseParameters<TripParameters>("1,2;3,4?roundtrip=true");
    BOOST_CHECK(param_r->IsValid());
    auto param_nr = parseParameters<TripParameters>("1,2;3,4?roundtrip=false");
    BOOST_CHECK(param_nr->IsValid());

    auto param_fail_1 =
        testInvalidOptions<TripParameters>("1,2;3,4?source=blubb&destination=random");
    BOOST_CHECK_EQUAL(param_fail_1, 15UL);
    auto param_fail_2 = testInvalidOptions<TripParameters>("1,2;3,4?source=first&destination=nah");
    BOOST_CHECK_EQUAL(param_fail_2, 33UL);
}

BOOST_AUTO_TEST_SUITE_END()
