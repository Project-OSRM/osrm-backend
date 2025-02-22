#include <boost/test/unit_test.hpp>

#include <cmath>

#include "coordinates.hpp"
#include "equal_json.hpp"
#include "fixture.hpp"

#include "engine/api/flatbuffers/fbresult_generated.h"
#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/exception.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/status.hpp"

osrm::Status run_route_json(const osrm::OSRM &osrm,
                            const osrm::RouteParameters &params,
                            osrm::json::Object &json_result,
                            bool use_json_only_api)
{
    if (use_json_only_api)
    {
        return osrm.Route(params, json_result);
    }
    osrm::engine::api::ResultT result = osrm::json::Object();
    auto rc = osrm.Route(params, result);
    json_result = std::get<osrm::json::Object>(result);
    return rc;
}

BOOST_AUTO_TEST_SUITE(route)

void test_route_same_coordinates_fixture(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    RouteParameters params;
    params.steps = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    json::Object json_result;
    const auto rc = run_route_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    // unset snapping dependent hint
    for (auto &itr : std::get<json::Array>(json_result.values["waypoints"]).values)
    {
        // Hint values aren't stable, so blank it out
        std::get<json::Object>(itr).values["hint"] = "";

        // Round value to 6 decimal places for double comparison later
        std::get<json::Object>(itr).values["distance"] = std::round(
            std::get<json::Number>(std::get<json::Object>(itr).values["distance"]).value * 1000000);
    }

    const auto location = json::Array{{{7.437070}, {43.749248}}};

    json::Object reference{
        {{"code", "Ok"},
         {"waypoints",
          json::Array{{json::Object{{{"name", "Boulevard du Larvotto"},
                                     {"location", location},
                                     {"distance", std::round(0.137249 * 1000000)},
                                     {"hint", ""}}},
                       json::Object{{{"name", "Boulevard du Larvotto"},
                                     {"location", location},
                                     {"distance", std::round(0.137249 * 1000000)},
                                     {"hint", ""}}}}}},
         {"routes",
          json::Array{{json::Object{
              {{"distance", 0.},
               {"duration", 0.},
               {"weight", 0.},
               {"weight_name", "routability"},
               {"geometry", "yw_jGupkl@??"},
               {"legs",
                json::Array{{json::Object{
                    {{"distance", 0.},
                     {"duration", 0.},
                     {"weight", 0.},
                     {"summary", "Boulevard du Larvotto"},
                     {"steps",
                      json::Array{{{json::Object{{{"duration", 0.},
                                                  {"distance", 0.},
                                                  {"weight", 0.},
                                                  {"geometry", "yw_jGupkl@??"},
                                                  {"name", "Boulevard du Larvotto"},
                                                  {"mode", "driving"},
                                                  {"driving_side", "right"},
                                                  {"maneuver",
                                                   json::Object{{
                                                       {"location", location},
                                                       {"bearing_before", 0},
                                                       {"bearing_after", 238},
                                                       {"type", "depart"},
                                                   }}},
                                                  {"intersections",
                                                   json::Array{{json::Object{
                                                       {{"location", location},
                                                        {"bearings", json::Array{{238}}},
                                                        {"entry", json::Array{{json::True()}}},
                                                        {"out", 0}}}}}}}}},

                                   json::Object{{{"duration", 0.},
                                                 {"distance", 0.},
                                                 {"weight", 0.},
                                                 {"geometry", "yw_jGupkl@"},
                                                 {"name", "Boulevard du Larvotto"},
                                                 {"mode", "driving"},
                                                 {"driving_side", "right"},
                                                 {"maneuver",
                                                  json::Object{{{"location", location},
                                                                {"bearing_before", 238},
                                                                {"bearing_after", 0},
                                                                {"type", "arrive"}}}},
                                                 {"intersections",
                                                  json::Array{{json::Object{
                                                      {{"location", location},
                                                       {"bearings", json::Array{{58}}},
                                                       {"entry", json::Array{{json::True()}}},
                                                       {"in", 0}}}}}}

                                   }}}}}}}}}}}}}}}}};

    CHECK_EQUAL_JSON(reference, json_result);
}
BOOST_AUTO_TEST_CASE(test_route_same_coordinates_fixture_old_api)
{
    test_route_same_coordinates_fixture(true);
}
BOOST_AUTO_TEST_CASE(test_route_same_coordinates_fixture_new_api)
{
    test_route_same_coordinates_fixture(false);
}

void test_route_same_coordinates(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    RouteParameters params;
    params.steps = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    json::Object json_result;
    const auto rc = run_route_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_CHECK(waypoints.size() == params.coordinates.size());

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = std::get<json::Object>(waypoint);

        // nothing can be said about name, empty or contains name of the street
        const auto name = std::get<json::String>(waypoint_object.values.at("name")).value;
        BOOST_CHECK(((void)name, true));

        const auto location = std::get<json::Array>(waypoint_object.values.at("location")).values;
        const auto longitude = std::get<json::Number>(location[0]).value;
        const auto latitude = std::get<json::Number>(location[1]).value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);

        const auto hint = std::get<json::String>(waypoint_object.values.at("hint")).value;
        BOOST_CHECK(!hint.empty());
    }

    const auto &routes = std::get<json::Array>(json_result.values.at("routes")).values;
    BOOST_REQUIRE_GT(routes.size(), 0);

    for (const auto &route : routes)
    {
        const auto &route_object = std::get<json::Object>(route);

        const auto distance = std::get<json::Number>(route_object.values.at("distance")).value;
        BOOST_CHECK_EQUAL(distance, 0);

        const auto duration = std::get<json::Number>(route_object.values.at("duration")).value;
        BOOST_CHECK_EQUAL(duration, 0);

        // geometries=polyline by default
        const auto geometry = std::get<json::String>(route_object.values.at("geometry")).value;
        BOOST_CHECK(!geometry.empty());

        const auto &legs = std::get<json::Array>(route_object.values.at("legs")).values;
        BOOST_CHECK(!legs.empty());

        for (const auto &leg : legs)
        {
            const auto &leg_object = std::get<json::Object>(leg);

            const auto distance = std::get<json::Number>(leg_object.values.at("distance")).value;
            BOOST_CHECK_EQUAL(distance, 0);

            const auto duration = std::get<json::Number>(leg_object.values.at("duration")).value;
            BOOST_CHECK_EQUAL(duration, 0);

            // nothing can be said about summary, empty or contains human readable summary
            const auto summary = std::get<json::String>(leg_object.values.at("summary")).value;
            BOOST_CHECK(((void)summary, true));

            const auto &steps = std::get<json::Array>(leg_object.values.at("steps")).values;
            BOOST_CHECK(!steps.empty());

            std::size_t step_count = 0;

            for (const auto &step : steps)
            {
                const auto &step_object = std::get<json::Object>(step);

                const auto distance =
                    std::get<json::Number>(step_object.values.at("distance")).value;
                BOOST_CHECK_EQUAL(distance, 0);

                const auto duration =
                    std::get<json::Number>(step_object.values.at("duration")).value;
                BOOST_CHECK_EQUAL(duration, 0);

                // geometries=polyline by default
                const auto geometry =
                    std::get<json::String>(step_object.values.at("geometry")).value;
                BOOST_CHECK(!geometry.empty());

                // nothing can be said about name, empty or contains way name
                const auto name = std::get<json::String>(step_object.values.at("name")).value;
                BOOST_CHECK(((void)name, true));

                // nothing can be said about mode, contains mode of transportation
                BOOST_CHECK(!name.empty());

                const auto &maneuver =
                    std::get<json::Object>(step_object.values.at("maneuver")).values;

                const auto type = std::get<json::String>(maneuver.at("type")).value;
                BOOST_CHECK(!type.empty());

                const auto &intersections =
                    std::get<json::Array>(step_object.values.at("intersections")).values;

                for (auto &intersection : intersections)
                {
                    const auto &intersection_object = std::get<json::Object>(intersection).values;
                    const auto location =
                        std::get<json::Array>(intersection_object.at("location")).values;
                    const auto longitude = std::get<json::Number>(location[0]).value;
                    const auto latitude = std::get<json::Number>(location[1]).value;
                    BOOST_CHECK(longitude >= -180. && longitude <= 180.);
                    BOOST_CHECK(latitude >= -90. && latitude <= 90.);

                    const auto &bearings =
                        std::get<json::Array>(intersection_object.at("bearings")).values;
                    BOOST_CHECK(!bearings.empty());
                    const auto &entries =
                        std::get<json::Array>(intersection_object.at("entry")).values;
                    BOOST_CHECK(bearings.size() == entries.size());

                    for (const auto &bearing : bearings)
                        BOOST_CHECK(0. <= std::get<json::Number>(bearing).value &&
                                    std::get<json::Number>(bearing).value <= 360.);

                    if (step_count > 0)
                    {
                        const auto in = std::get<json::Number>(intersection_object.at("in")).value;
                        BOOST_CHECK(in < bearings.size());
                    }
                    if (step_count + 1 < steps.size())
                    {
                        const auto out =
                            std::get<json::Number>(intersection_object.at("out")).value;
                        BOOST_CHECK(out < bearings.size());
                    }
                }

                // modifier is optional
                // TODO(daniel-j-h):

                // exit is optional
                // TODO(daniel-j-h):
                ++step_count;
            }
        }
    }
}
BOOST_AUTO_TEST_CASE(test_route_same_coordinates_old_api) { test_route_same_coordinates(true); }
BOOST_AUTO_TEST_CASE(test_route_same_coordinates_new_api) { test_route_same_coordinates(false); }

void test_route_same_coordinates_no_waypoints(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    RouteParameters params;
    params.skip_waypoints = true;
    params.steps = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    json::Object json_result;
    const auto rc = run_route_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    BOOST_CHECK(json_result.values.find("waypoints") == json_result.values.end());

    const auto &routes = std::get<json::Array>(json_result.values.at("routes")).values;
    BOOST_REQUIRE_GT(routes.size(), 0);

    for (const auto &route : routes)
    {
        const auto &route_object = std::get<json::Object>(route);

        const auto distance = std::get<json::Number>(route_object.values.at("distance")).value;
        BOOST_CHECK_EQUAL(distance, 0);

        const auto duration = std::get<json::Number>(route_object.values.at("duration")).value;
        BOOST_CHECK_EQUAL(duration, 0);

        // geometries=polyline by default
        const auto geometry = std::get<json::String>(route_object.values.at("geometry")).value;
        BOOST_CHECK(!geometry.empty());

        const auto &legs = std::get<json::Array>(route_object.values.at("legs")).values;
        BOOST_CHECK(!legs.empty());

        // The rest of legs contents is verified by test_route_same_coordinates
    }
}
BOOST_AUTO_TEST_CASE(test_route_same_coordinates_no_waypoints_old_api)
{
    test_route_same_coordinates_no_waypoints(true);
}
BOOST_AUTO_TEST_CASE(test_route_same_coordinates_no_waypoints_new_api)
{
    test_route_same_coordinates_no_waypoints(false);
}

void test_route_response_for_locations_in_small_component(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    const auto locations = get_locations_in_small_component();

    RouteParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    json::Object json_result;
    const auto rc = run_route_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = std::get<json::Object>(waypoint);

        const auto location = std::get<json::Array>(waypoint_object.values.at("location")).values;
        const auto longitude = std::get<json::Number>(location[0]).value;
        const auto latitude = std::get<json::Number>(location[1]).value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);
    }
}
BOOST_AUTO_TEST_CASE(test_route_response_for_locations_in_small_component_old_api)
{
    test_route_response_for_locations_in_small_component(true);
}
BOOST_AUTO_TEST_CASE(test_route_response_for_locations_in_small_component_new_api)
{
    test_route_response_for_locations_in_small_component(false);
}

void test_route_response_for_locations_in_big_component(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    const auto locations = get_locations_in_big_component();

    RouteParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    json::Object json_result;
    const auto rc = run_route_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = std::get<json::Object>(waypoint);

        const auto location = std::get<json::Array>(waypoint_object.values.at("location")).values;
        const auto longitude = std::get<json::Number>(location[0]).value;
        const auto latitude = std::get<json::Number>(location[1]).value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);
    }
}
BOOST_AUTO_TEST_CASE(test_route_response_for_locations_in_big_component_old_api)
{
    test_route_response_for_locations_in_big_component(true);
}
BOOST_AUTO_TEST_CASE(test_route_response_for_locations_in_big_component_new_api)
{
    test_route_response_for_locations_in_big_component(false);
}

void test_route_response_for_locations_across_components(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    const auto big_component = get_locations_in_big_component();
    const auto small_component = get_locations_in_small_component();

    RouteParameters params;
    params.coordinates.push_back(small_component.at(0));
    params.coordinates.push_back(big_component.at(0));
    params.coordinates.push_back(small_component.at(1));
    params.coordinates.push_back(big_component.at(1));

    json::Object json_result;
    const auto rc = run_route_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = std::get<json::Array>(json_result.values.at("waypoints")).values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = std::get<json::Object>(waypoint);

        const auto location = std::get<json::Array>(waypoint_object.values.at("location")).values;
        const auto longitude = std::get<json::Number>(location[0]).value;
        const auto latitude = std::get<json::Number>(location[1]).value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);
    }
}
BOOST_AUTO_TEST_CASE(test_route_response_for_locations_across_components_old_api)
{
    test_route_response_for_locations_across_components(true);
}
BOOST_AUTO_TEST_CASE(test_route_response_for_locations_across_components_new_api)
{
    test_route_response_for_locations_across_components(false);
}

void test_route_user_disables_generating_hints(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    RouteParameters params;
    params.steps = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.generate_hints = false;

    json::Object json_result;
    const auto rc = run_route_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    for (auto waypoint : std::get<json::Array>(json_result.values["waypoints"]).values)
        BOOST_CHECK_EQUAL(std::get<json::Object>(waypoint).values.count("hint"), 0);
}
BOOST_AUTO_TEST_CASE(test_route_user_disables_generating_hints_old_api)
{
    test_route_user_disables_generating_hints(true);
}
BOOST_AUTO_TEST_CASE(test_route_user_disables_generating_hints_new_api)
{
    test_route_user_disables_generating_hints(false);
}

void speed_annotation_matches_duration_and_distance(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    RouteParameters params;
    params.annotations_type = RouteParameters::AnnotationsType::Duration |
                              RouteParameters::AnnotationsType::Distance |
                              RouteParameters::AnnotationsType::Speed;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    json::Object json_result;
    const auto rc = run_route_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    const auto &routes = std::get<json::Array>(json_result.values["routes"]).values;
    const auto &legs =
        std::get<json::Array>(std::get<json::Object>(routes[0]).values.at("legs")).values;
    const auto &annotation =
        std::get<json::Object>(std::get<json::Object>(legs[0]).values.at("annotation"));
    const auto &speeds = std::get<json::Array>(annotation.values.at("speed")).values;
    const auto &durations = std::get<json::Array>(annotation.values.at("duration")).values;
    const auto &distances = std::get<json::Array>(annotation.values.at("distance")).values;
    int length = speeds.size();

    BOOST_CHECK_EQUAL(length, 1);
    for (int i = 0; i < length; i++)
    {
        auto speed = std::get<json::Number>(speeds[i]).value;
        auto duration = std::get<json::Number>(durations[i]).value;
        auto distance = std::get<json::Number>(distances[i]).value;
        auto calc = std::round(distance / duration * 10.) / 10.;
        BOOST_CHECK_EQUAL(speed, std::isnan(calc) ? 0 : calc);

        // Because we route from/to the same location, all annotations should be 0;
        BOOST_CHECK_EQUAL(speed, 0);
        BOOST_CHECK_EQUAL(distance, 0);
        BOOST_CHECK_EQUAL(duration, 0);
    }
}
BOOST_AUTO_TEST_CASE(speed_annotation_matches_duration_and_distance_old_api)
{
    speed_annotation_matches_duration_and_distance(true);
}
BOOST_AUTO_TEST_CASE(speed_annotation_matches_duration_and_distance_new_api)
{
    speed_annotation_matches_duration_and_distance(false);
}

void test_manual_setting_of_annotations_property(bool use_json_only_api)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    RouteParameters params{};
    params.annotations = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    json::Object json_result;
    const auto rc = run_route_json(osrm, params, json_result, use_json_only_api);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = std::get<json::String>(json_result.values.at("code")).value;
    BOOST_CHECK_EQUAL(code, "Ok");

    auto annotations =
        std::get<json::Object>(
            std::get<json::Object>(
                std::get<json::Array>(
                    std::get<json::Object>(
                        std::get<json::Array>(json_result.values["routes"]).values[0])
                        .values["legs"])
                    .values[0])
                .values["annotation"])
            .values;
    BOOST_CHECK_EQUAL(annotations.size(), 7);
}
BOOST_AUTO_TEST_CASE(test_manual_setting_of_annotations_property_old_api)
{
    test_manual_setting_of_annotations_property(true);
}
BOOST_AUTO_TEST_CASE(test_manual_setting_of_annotations_property_new_api)
{
    test_manual_setting_of_annotations_property(false);
}

BOOST_AUTO_TEST_CASE(test_route_serialize_fb)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    RouteParameters params;
    params.steps = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    engine::api::ResultT result = flatbuffers::FlatBufferBuilder();
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &fb_result = std::get<flatbuffers::FlatBufferBuilder>(result);
    auto fb = engine::api::fbresult::GetFBResult(fb_result.GetBufferPointer());
    BOOST_CHECK(!fb->error());

    BOOST_CHECK(fb->waypoints() != nullptr);
    const auto waypoints = fb->waypoints();
    BOOST_CHECK(waypoints->size() == params.coordinates.size());

    for (const auto waypoint : *waypoints)
    {
        const auto longitude = waypoint->location()->longitude();
        const auto latitude = waypoint->location()->latitude();
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);

        BOOST_CHECK(!waypoint->hint()->str().empty());
    }

    BOOST_CHECK(fb->routes() != nullptr);
    const auto routes = fb->routes();
    BOOST_REQUIRE_GT(routes->size(), 0);

    for (const auto route : *routes)
    {
        BOOST_CHECK_EQUAL(route->distance(), 0);
        BOOST_CHECK_EQUAL(route->duration(), 0);

        const auto &legs = route->legs();
        BOOST_CHECK(legs->size() > 0);

        for (const auto leg : *legs)
        {
            BOOST_CHECK_EQUAL(leg->distance(), 0);

            BOOST_CHECK_EQUAL(leg->duration(), 0);

            BOOST_CHECK(leg->steps() != nullptr);
            const auto steps = leg->steps();
            BOOST_CHECK(steps->size() > 0);

            std::size_t step_count = 0;

            for (const auto step : *steps)
            {
                BOOST_CHECK_EQUAL(step->distance(), 0);

                BOOST_CHECK_EQUAL(step->duration(), 0);

                BOOST_CHECK(step->maneuver() != nullptr);

                BOOST_CHECK(step->intersections() != nullptr);
                const auto intersections = step->intersections();

                for (auto intersection : *intersections)
                {
                    const auto longitude = intersection->location()->longitude();
                    const auto latitude = intersection->location()->latitude();
                    BOOST_CHECK(longitude >= -180. && longitude <= 180.);
                    BOOST_CHECK(latitude >= -90. && latitude <= 90.);

                    BOOST_CHECK(intersection->bearings() != nullptr);
                    const auto bearings = intersection->bearings();
                    BOOST_CHECK(bearings->size() > 0);

                    for (const auto bearing : *bearings)
                        BOOST_CHECK(0. <= bearing && bearing <= 360.);

                    if (step_count > 0)
                    {
                        BOOST_CHECK(intersection->in_bearing() < bearings->size());
                    }
                    if (step_count + 1 < steps->size())
                    {
                        BOOST_CHECK(intersection->out_bearing() < bearings->size());
                    }
                }
                ++step_count;
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_route_serialize_fb_skip_waypoints)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    RouteParameters params;
    params.skip_waypoints = true;
    params.steps = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    engine::api::ResultT result = flatbuffers::FlatBufferBuilder();
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &fb_result = std::get<flatbuffers::FlatBufferBuilder>(result);
    auto fb = engine::api::fbresult::GetFBResult(fb_result.GetBufferPointer());
    BOOST_CHECK(!fb->error());

    BOOST_CHECK(fb->waypoints() == nullptr);

    BOOST_CHECK(fb->routes() != nullptr);
    const auto routes = fb->routes();
    BOOST_REQUIRE_GT(routes->size(), 0);

    for (const auto route : *routes)
    {
        BOOST_CHECK_EQUAL(route->distance(), 0);
        BOOST_CHECK_EQUAL(route->duration(), 0);

        const auto &legs = route->legs();
        BOOST_CHECK(legs->size() > 0);

        // Rest of the content is verified by test_route_serialize_fb
    }
}

BOOST_AUTO_TEST_SUITE_END()
