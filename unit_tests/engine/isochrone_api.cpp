#include "engine/api/isochrone_api.hpp"

#include "mocks/mock_datafacade.hpp"

#include "util/json_renderer.hpp"

#include <boost/test/unit_test.hpp>

#include <limits>
#include <string>
#include <variant>

BOOST_AUTO_TEST_SUITE(isochrone_api)

BOOST_AUTO_TEST_CASE(contours_seconds_at_the_effective_decisecond_limit)
{
    osrm::test::MockBaseDataFacade facade;
    osrm::engine::api::IsochroneParameters parameters;
    parameters.contours_seconds = {1.11, 1.2};
    parameters.skip_waypoints = true;
    const osrm::engine::api::IsochroneAPI api{facade, parameters};

    // Geometry can have fractional durations after interpolation, although the
    // routing search itself is bounded at whole deciseconds.  The requested
    // 1.11 second contour therefore has an effective cutoff of 1.1 seconds.
    const osrm::engine::isochrone::WeightedGrid grid{osrm::util::FloatLongitude{7.4},
                                                     osrm::util::FloatLatitude{43.7},
                                                     100.,
                                                     1,
                                                     1,
                                                     {1.105},
                                                     100.,
                                                     -200.};
    osrm::util::json::Object response;

    BOOST_REQUIRE(api.MakeResponse(grid, {}, 100, response) ==
                  osrm::engine::api::IsochroneAPI::ResponseStatus::Ok);

    const auto &features = std::get<osrm::util::json::Array>(response.values.at("features")).values;
    BOOST_REQUIRE_EQUAL(features.size(), 2);
    const auto &first_feature = std::get<osrm::util::json::Object>(features[0]);
    const auto &second_feature = std::get<osrm::util::json::Object>(features[1]);
    const auto &first_properties =
        std::get<osrm::util::json::Object>(first_feature.values.at("properties"));
    const auto &second_properties =
        std::get<osrm::util::json::Object>(second_feature.values.at("properties"));
    const auto &first_geometry =
        std::get<osrm::util::json::Object>(first_feature.values.at("geometry"));
    const auto &second_geometry =
        std::get<osrm::util::json::Object>(second_feature.values.at("geometry"));

    BOOST_CHECK(
        std::get<osrm::util::json::Array>(first_geometry.values.at("coordinates")).values.empty());
    BOOST_CHECK_EQUAL(
        std::get<osrm::util::json::Array>(second_geometry.values.at("coordinates")).values.size(),
        1);
    const auto &second_polygons =
        std::get<osrm::util::json::Array>(second_geometry.values.at("coordinates"));
    const auto &second_polygon = std::get<osrm::util::json::Array>(second_polygons.values.front());
    const auto &second_ring = std::get<osrm::util::json::Array>(second_polygon.values.front());
    const auto &first_position = std::get<osrm::util::json::Array>(second_ring.values.front());
    BOOST_REQUIRE_EQUAL(first_position.values.size(), 2);
    BOOST_CHECK(std::get<osrm::util::json::Number>(first_position.values[0]).value > 7.4);
    BOOST_CHECK(std::get<osrm::util::json::Number>(first_position.values[1]).value < 43.7);
    BOOST_CHECK_CLOSE(
        std::get<osrm::util::json::Number>(first_properties.values.at("contour_seconds")).value,
        1.11,
        1e-9);
    BOOST_CHECK_CLOSE(
        std::get<osrm::util::json::Number>(first_properties.values.at("effective_contour_seconds"))
            .value,
        1.1,
        1e-9);
    BOOST_CHECK_CLOSE(
        std::get<osrm::util::json::Number>(second_properties.values.at("contour_seconds")).value,
        1.2,
        1e-9);
    BOOST_CHECK_CLOSE(
        std::get<osrm::util::json::Number>(second_properties.values.at("effective_contour_seconds"))
            .value,
        1.2,
        1e-9);
    BOOST_CHECK(!first_properties.values.contains("contour"));
    BOOST_CHECK(!first_properties.values.contains("effective_contour"));
    BOOST_CHECK(!response.values.contains("weight_name"));
}

BOOST_AUTO_TEST_CASE(stops_before_materializing_a_contour_that_exceeds_the_output_limit)
{
    osrm::test::MockBaseDataFacade facade;
    osrm::engine::api::IsochroneParameters parameters;
    parameters.contours_seconds = {1.};
    parameters.skip_waypoints = true;
    const osrm::engine::api::IsochroneAPI api{facade, parameters};

    // A single occupied cell has a closed boundary with five coordinates.
    const osrm::engine::isochrone::WeightedGrid grid{
        osrm::util::FloatLongitude{0.}, osrm::util::FloatLatitude{0.}, 100., 1, 1, {0.}};
    osrm::util::json::Object response;

    BOOST_CHECK(api.MakeResponse(grid, {}, 4, response) ==
                osrm::engine::api::IsochroneAPI::ResponseStatus::TooBig);
}

BOOST_AUTO_TEST_CASE(zero_postprocessing_is_an_exact_output_no_op)
{
    osrm::test::MockBaseDataFacade facade;
    osrm::engine::api::IsochroneParameters raw_parameters;
    raw_parameters.contours_seconds = {1.};
    raw_parameters.skip_waypoints = true;
    auto zero_parameters = raw_parameters;
    zero_parameters.generalize = 0.;
    zero_parameters.denoise = 0.;

    const osrm::engine::isochrone::WeightedGrid grid{osrm::util::FloatLongitude{7.4},
                                                     osrm::util::FloatLatitude{43.7},
                                                     100.,
                                                     2,
                                                     2,
                                                     {0., 0., 0., 0.}};
    const osrm::engine::api::IsochroneAPI raw_api{facade, raw_parameters};
    const osrm::engine::api::IsochroneAPI zero_api{facade, zero_parameters};
    osrm::util::json::Object raw_response;
    osrm::util::json::Object zero_response;

    BOOST_REQUIRE(raw_api.MakeResponse(grid, {}, 100, raw_response) ==
                  osrm::engine::api::IsochroneAPI::ResponseStatus::Ok);
    BOOST_REQUIRE(zero_api.MakeResponse(grid, {}, 100, zero_response) ==
                  osrm::engine::api::IsochroneAPI::ResponseStatus::Ok);

    std::string raw_json;
    std::string zero_json;
    osrm::util::json::render(raw_json, raw_response);
    osrm::util::json::render(zero_json, zero_response);
    BOOST_CHECK_EQUAL(raw_json, zero_json);
}

BOOST_AUTO_TEST_CASE(denoising_removes_small_components_and_holes)
{
    osrm::test::MockBaseDataFacade facade;
    osrm::engine::api::IsochroneParameters raw_parameters;
    raw_parameters.contours_seconds = {1.};
    raw_parameters.skip_waypoints = true;
    auto denoised_parameters = raw_parameters;
    denoised_parameters.denoise = 0.1;

    const auto outside = std::numeric_limits<double>::infinity();
    const osrm::engine::isochrone::WeightedGrid grid{
        osrm::util::FloatLongitude{7.4},
        osrm::util::FloatLatitude{43.7},
        100.,
        7,
        4,
        {0., 0.,      0.,      0.,      outside, outside, outside, 0.,     outside, 0.,
         0., outside, outside, outside, 0.,      0.,      0.,      0.,     outside, outside,
         0., 0.,      0.,      0.,      0.,      outside, outside, outside}};
    const osrm::engine::api::IsochroneAPI raw_api{facade, raw_parameters};
    const osrm::engine::api::IsochroneAPI denoised_api{facade, denoised_parameters};
    osrm::util::json::Object raw_response;
    osrm::util::json::Object denoised_response;

    BOOST_REQUIRE(raw_api.MakeResponse(grid, {}, 100, raw_response) ==
                  osrm::engine::api::IsochroneAPI::ResponseStatus::Ok);
    BOOST_REQUIRE(denoised_api.MakeResponse(grid, {}, 100, denoised_response) ==
                  osrm::engine::api::IsochroneAPI::ResponseStatus::Ok);

    const auto polygon_coordinates = [](const osrm::util::json::Object &response) -> const auto &
    {
        const auto &features =
            std::get<osrm::util::json::Array>(response.values.at("features")).values;
        const auto &feature = std::get<osrm::util::json::Object>(features.front());
        const auto &geometry = std::get<osrm::util::json::Object>(feature.values.at("geometry"));
        return std::get<osrm::util::json::Array>(geometry.values.at("coordinates")).values;
    };
    const auto &raw_polygons = polygon_coordinates(raw_response);
    const auto &denoised_polygons = polygon_coordinates(denoised_response);
    BOOST_REQUIRE_EQUAL(raw_polygons.size(), 2);
    BOOST_REQUIRE_EQUAL(std::get<osrm::util::json::Array>(raw_polygons.front()).values.size(), 2);
    BOOST_REQUIRE_EQUAL(denoised_polygons.size(), 1);
    BOOST_REQUIRE_EQUAL(std::get<osrm::util::json::Array>(denoised_polygons.front()).values.size(),
                        1);

    const auto raw = grid.buildContours(1.);
    std::size_t raw_coordinate_count = 0;
    for (const auto &polygon : raw)
    {
        raw_coordinate_count += polygon.outer.size();
        for (const auto &hole : polygon.holes)
            raw_coordinate_count += hole.size();
    }
    BOOST_REQUIRE_GT(raw_coordinate_count, 0);
    osrm::util::json::Object limited_response;
    BOOST_CHECK(denoised_api.MakeResponse(grid, {}, raw_coordinate_count - 1, limited_response) ==
                osrm::engine::api::IsochroneAPI::ResponseStatus::TooBig);
}

BOOST_AUTO_TEST_CASE(generalization_is_applied_in_metres_after_raw_contouring)
{
    osrm::test::MockBaseDataFacade facade;
    osrm::engine::api::IsochroneParameters raw_parameters;
    raw_parameters.contours_seconds = {1.};
    raw_parameters.skip_waypoints = true;
    auto generalized_parameters = raw_parameters;
    generalized_parameters.generalize = 100000.;

    const auto outside = std::numeric_limits<double>::infinity();
    // This L-shaped coverage has a seven-coordinate raw boundary. A large tolerance is measured
    // in metres, converted to the 100-metre grid, and can safely reduce that boundary.
    const osrm::engine::isochrone::WeightedGrid grid{
        osrm::util::FloatLongitude{7.4},
        osrm::util::FloatLatitude{43.7},
        100.,
        3,
        3,
        {0., outside, outside, 0., outside, outside, 0., 0., 0.}};
    const osrm::engine::api::IsochroneAPI raw_api{facade, raw_parameters};
    const osrm::engine::api::IsochroneAPI generalized_api{facade, generalized_parameters};
    osrm::util::json::Object raw_response;
    osrm::util::json::Object generalized_response;

    BOOST_REQUIRE(raw_api.MakeResponse(grid, {}, 100, raw_response) ==
                  osrm::engine::api::IsochroneAPI::ResponseStatus::Ok);
    BOOST_REQUIRE(generalized_api.MakeResponse(grid, {}, 100, generalized_response) ==
                  osrm::engine::api::IsochroneAPI::ResponseStatus::Ok);

    const auto outer_ring_size = [](const osrm::util::json::Object &response)
    {
        const auto &features =
            std::get<osrm::util::json::Array>(response.values.at("features")).values;
        const auto &feature = std::get<osrm::util::json::Object>(features.front());
        const auto &geometry = std::get<osrm::util::json::Object>(feature.values.at("geometry"));
        const auto &multipolygon =
            std::get<osrm::util::json::Array>(geometry.values.at("coordinates")).values;
        const auto &polygon = std::get<osrm::util::json::Array>(multipolygon.front()).values;
        return std::get<osrm::util::json::Array>(polygon.front()).values.size();
    };
    BOOST_CHECK_LT(outer_ring_size(generalized_response), outer_ring_size(raw_response));
}

BOOST_AUTO_TEST_CASE(generalization_cannot_bypass_the_raw_output_limit)
{
    osrm::test::MockBaseDataFacade facade;
    osrm::engine::api::IsochroneParameters parameters;
    parameters.contours_seconds = {1.};
    parameters.generalize = std::numeric_limits<double>::max();
    parameters.skip_waypoints = true;
    const osrm::engine::api::IsochroneAPI api{facade, parameters};

    // The raw contour for one occupied cell needs five coordinates. Generalization is applied
    // only after that raw result satisfies the configured output limit.
    const osrm::engine::isochrone::WeightedGrid grid{
        osrm::util::FloatLongitude{0.}, osrm::util::FloatLatitude{0.}, 100., 1, 1, {0.}};
    osrm::util::json::Object response;

    BOOST_CHECK(api.MakeResponse(grid, {}, 4, response) ==
                osrm::engine::api::IsochroneAPI::ResponseStatus::TooBig);
}

BOOST_AUTO_TEST_CASE(generalization_cannot_bypass_the_cumulative_raw_output_limit)
{
    osrm::test::MockBaseDataFacade facade;
    osrm::engine::api::IsochroneParameters parameters;
    parameters.contours_seconds = {1., 2.};
    parameters.generalize = std::numeric_limits<double>::max();
    parameters.skip_waypoints = true;
    const osrm::engine::api::IsochroneAPI api{facade, parameters};

    const auto outside = std::numeric_limits<double>::infinity();
    const osrm::engine::isochrone::WeightedGrid grid{
        osrm::util::FloatLongitude{0.},
        osrm::util::FloatLatitude{0.},
        100.,
        3,
        3,
        {0., outside, outside, 0., outside, outside, 0., 0., 0.}};
    const auto raw = grid.buildContours(1.);
    const auto generalized =
        grid.buildContours(1., std::numeric_limits<std::size_t>::max(), *parameters.generalize);
    BOOST_REQUIRE_EQUAL(raw.size(), 1);
    BOOST_REQUIRE(generalized);
    BOOST_REQUIRE_EQUAL(generalized->size(), 1);
    BOOST_REQUIRE_LT(generalized->front().outer.size(), raw.front().outer.size());

    const auto limit = raw.front().outer.size() * 2 - 1;
    osrm::util::json::Object response;
    BOOST_CHECK(api.MakeResponse(grid, {}, limit, response) ==
                osrm::engine::api::IsochroneAPI::ResponseStatus::TooBig);
}

BOOST_AUTO_TEST_SUITE_END()
