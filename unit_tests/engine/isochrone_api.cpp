#include "engine/api/isochrone_api.hpp"

#include "mocks/mock_datafacade.hpp"

#include <boost/test/unit_test.hpp>

#include <variant>

BOOST_AUTO_TEST_SUITE(isochrone_api)

BOOST_AUTO_TEST_CASE(contours_at_the_effective_decisecond_limit)
{
    osrm::test::MockBaseDataFacade facade;
    osrm::engine::api::IsochroneParameters parameters;
    parameters.contours = {1.11, 1.2};
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
        std::get<osrm::util::json::Number>(first_properties.values.at("effective_contour")).value,
        1.1,
        1e-9);
    BOOST_CHECK_CLOSE(
        std::get<osrm::util::json::Number>(second_properties.values.at("effective_contour")).value,
        1.2,
        1e-9);
    BOOST_CHECK(!response.values.contains("weight_name"));
}

BOOST_AUTO_TEST_CASE(stops_before_materializing_a_contour_that_exceeds_the_output_limit)
{
    osrm::test::MockBaseDataFacade facade;
    osrm::engine::api::IsochroneParameters parameters;
    parameters.contours = {1.};
    parameters.skip_waypoints = true;
    const osrm::engine::api::IsochroneAPI api{facade, parameters};

    // A single occupied cell has a closed boundary with five coordinates.
    const osrm::engine::isochrone::WeightedGrid grid{
        osrm::util::FloatLongitude{0.}, osrm::util::FloatLatitude{0.}, 100., 1, 1, {0.}};
    osrm::util::json::Object response;

    BOOST_CHECK(api.MakeResponse(grid, {}, 4, response) ==
                osrm::engine::api::IsochroneAPI::ResponseStatus::TooBig);
}

BOOST_AUTO_TEST_SUITE_END()
