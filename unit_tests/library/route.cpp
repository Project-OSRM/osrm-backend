#include <boost/test/test_case_template.hpp>
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

BOOST_AUTO_TEST_SUITE(route)

BOOST_AUTO_TEST_CASE(test_route_same_coordinates_fixture)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    RouteParameters params;
    params.steps = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    engine::api::ResultT result = json::Object();
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &json_result = result.get<json::Object>();
    // unset snapping dependent hint
    for (auto &itr : json_result.values["waypoints"].get<json::Array>().values)
    {
        // Hint values aren't stable, so blank it out
        itr.get<json::Object>().values["hint"] = "";

        // Round value to 6 decimal places for double comparison later
        itr.get<json::Object>().values["distance"] =
            round(itr.get<json::Object>().values["distance"].get<json::Number>().value * 1000000);
    }

    const auto location = json::Array{{{7.437070}, {43.749248}}};

    json::Object reference{
        {{"code", "Ok"},
         {"waypoints",
          json::Array{{json::Object{{{"name", "Boulevard du Larvotto"},
                                     {"location", location},
                                     {"distance", round(0.137249 * 1000000)},
                                     {"hint", ""}}},
                       json::Object{{{"name", "Boulevard du Larvotto"},
                                     {"location", location},
                                     {"distance", round(0.137249 * 1000000)},
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

BOOST_AUTO_TEST_CASE(test_route_same_coordinates)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    RouteParameters params;
    params.steps = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    engine::api::ResultT result = json::Object();
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = json_result.values.at("waypoints").get<json::Array>().values;
    BOOST_CHECK(waypoints.size() == params.coordinates.size());

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = waypoint.get<json::Object>();

        // nothing can be said about name, empty or contains name of the street
        const auto name = waypoint_object.values.at("name").get<json::String>().value;
        BOOST_CHECK(((void)name, true));

        const auto location = waypoint_object.values.at("location").get<json::Array>().values;
        const auto longitude = location[0].get<json::Number>().value;
        const auto latitude = location[1].get<json::Number>().value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);

        const auto hint = waypoint_object.values.at("hint").get<json::String>().value;
        BOOST_CHECK(!hint.empty());
    }

    const auto &routes = json_result.values.at("routes").get<json::Array>().values;
    BOOST_REQUIRE_GT(routes.size(), 0);

    for (const auto &route : routes)
    {
        const auto &route_object = route.get<json::Object>();

        const auto distance = route_object.values.at("distance").get<json::Number>().value;
        BOOST_CHECK_EQUAL(distance, 0);

        const auto duration = route_object.values.at("duration").get<json::Number>().value;
        BOOST_CHECK_EQUAL(duration, 0);

        // geometries=polyline by default
        const auto geometry = route_object.values.at("geometry").get<json::String>().value;
        BOOST_CHECK(!geometry.empty());

        const auto &legs = route_object.values.at("legs").get<json::Array>().values;
        BOOST_CHECK(!legs.empty());

        for (const auto &leg : legs)
        {
            const auto &leg_object = leg.get<json::Object>();

            const auto distance = leg_object.values.at("distance").get<json::Number>().value;
            BOOST_CHECK_EQUAL(distance, 0);

            const auto duration = leg_object.values.at("duration").get<json::Number>().value;
            BOOST_CHECK_EQUAL(duration, 0);

            // nothing can be said about summary, empty or contains human readable summary
            const auto summary = leg_object.values.at("summary").get<json::String>().value;
            BOOST_CHECK(((void)summary, true));

            const auto &steps = leg_object.values.at("steps").get<json::Array>().values;
            BOOST_CHECK(!steps.empty());

            std::size_t step_count = 0;

            for (const auto &step : steps)
            {
                const auto &step_object = step.get<json::Object>();

                const auto distance = step_object.values.at("distance").get<json::Number>().value;
                BOOST_CHECK_EQUAL(distance, 0);

                const auto duration = step_object.values.at("duration").get<json::Number>().value;
                BOOST_CHECK_EQUAL(duration, 0);

                // geometries=polyline by default
                const auto geometry = step_object.values.at("geometry").get<json::String>().value;
                BOOST_CHECK(!geometry.empty());

                // nothing can be said about name, empty or contains way name
                const auto name = step_object.values.at("name").get<json::String>().value;
                BOOST_CHECK(((void)name, true));

                // nothing can be said about mode, contains mode of transportation
                const auto mode = step_object.values.at("mode").get<json::String>().value;
                BOOST_CHECK(!name.empty());

                const auto &maneuver = step_object.values.at("maneuver").get<json::Object>().values;

                const auto type = maneuver.at("type").get<json::String>().value;
                BOOST_CHECK(!type.empty());

                const auto &intersections =
                    step_object.values.at("intersections").get<json::Array>().values;

                for (auto &intersection : intersections)
                {
                    const auto &intersection_object = intersection.get<json::Object>().values;
                    const auto location =
                        intersection_object.at("location").get<json::Array>().values;
                    const auto longitude = location[0].get<json::Number>().value;
                    const auto latitude = location[1].get<json::Number>().value;
                    BOOST_CHECK(longitude >= -180. && longitude <= 180.);
                    BOOST_CHECK(latitude >= -90. && latitude <= 90.);

                    const auto &bearings =
                        intersection_object.at("bearings").get<json::Array>().values;
                    BOOST_CHECK(!bearings.empty());
                    const auto &entries = intersection_object.at("entry").get<json::Array>().values;
                    BOOST_CHECK(bearings.size() == entries.size());

                    for (const auto bearing : bearings)
                        BOOST_CHECK(0. <= bearing.get<json::Number>().value &&
                                    bearing.get<json::Number>().value <= 360.);

                    if (step_count > 0)
                    {
                        const auto in = intersection_object.at("in").get<json::Number>().value;
                        BOOST_CHECK(in < bearings.size());
                    }
                    if (step_count + 1 < steps.size())
                    {
                        const auto out = intersection_object.at("out").get<json::Number>().value;
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

BOOST_AUTO_TEST_CASE(test_route_response_for_locations_in_small_component)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    const auto locations = get_locations_in_small_component();

    RouteParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    engine::api::ResultT result = json::Object();
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = json_result.values.at("waypoints").get<json::Array>().values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = waypoint.get<json::Object>();

        const auto location = waypoint_object.values.at("location").get<json::Array>().values;
        const auto longitude = location[0].get<json::Number>().value;
        const auto latitude = location[1].get<json::Number>().value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);
    }
}

BOOST_AUTO_TEST_CASE(test_route_response_for_locations_in_big_component)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    const auto locations = get_locations_in_big_component();

    RouteParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    engine::api::ResultT result = json::Object();
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = json_result.values.at("waypoints").get<json::Array>().values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = waypoint.get<json::Object>();

        const auto location = waypoint_object.values.at("location").get<json::Array>().values;
        const auto longitude = location[0].get<json::Number>().value;
        const auto latitude = location[1].get<json::Number>().value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);
    }
}

BOOST_AUTO_TEST_CASE(test_route_response_for_locations_across_components)
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

    engine::api::ResultT result = json::Object();
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = json_result.values.at("waypoints").get<json::Array>().values;
    BOOST_CHECK_EQUAL(waypoints.size(), params.coordinates.size());

    for (const auto &waypoint : waypoints)
    {
        const auto &waypoint_object = waypoint.get<json::Object>();

        const auto location = waypoint_object.values.at("location").get<json::Array>().values;
        const auto longitude = location[0].get<json::Number>().value;
        const auto latitude = location[1].get<json::Number>().value;
        BOOST_CHECK(longitude >= -180. && longitude <= 180.);
        BOOST_CHECK(latitude >= -90. && latitude <= 90.);
    }
}

BOOST_AUTO_TEST_CASE(test_route_user_disables_generating_hints)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    RouteParameters params;
    params.steps = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.generate_hints = false;

    engine::api::ResultT result = json::Object();
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &json_result = result.get<json::Object>();
    for (auto waypoint : json_result.values["waypoints"].get<json::Array>().values)
        BOOST_CHECK_EQUAL(waypoint.get<json::Object>().values.count("hint"), 0);
}

BOOST_AUTO_TEST_CASE(speed_annotation_matches_duration_and_distance)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    RouteParameters params;
    params.annotations_type = RouteParameters::AnnotationsType::Duration |
                              RouteParameters::AnnotationsType::Distance |
                              RouteParameters::AnnotationsType::Speed;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    engine::api::ResultT result = json::Object();
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &json_result = result.get<json::Object>();
    const auto &routes = json_result.values["routes"].get<json::Array>().values;
    const auto &legs = routes[0].get<json::Object>().values.at("legs").get<json::Array>().values;
    const auto &annotation =
        legs[0].get<json::Object>().values.at("annotation").get<json::Object>();
    const auto &speeds = annotation.values.at("speed").get<json::Array>().values;
    const auto &durations = annotation.values.at("duration").get<json::Array>().values;
    const auto &distances = annotation.values.at("distance").get<json::Array>().values;
    int length = speeds.size();

    BOOST_CHECK_EQUAL(length, 1);
    for (int i = 0; i < length; i++)
    {
        auto speed = speeds[i].get<json::Number>().value;
        auto duration = durations[i].get<json::Number>().value;
        auto distance = distances[i].get<json::Number>().value;
        auto calc = std::round(distance / duration * 10.) / 10.;
        BOOST_CHECK_EQUAL(speed, std::isnan(calc) ? 0 : calc);

        // Because we route from/to the same location, all annotations should be 0;
        BOOST_CHECK_EQUAL(speed, 0);
        BOOST_CHECK_EQUAL(distance, 0);
        BOOST_CHECK_EQUAL(duration, 0);
    }
}

BOOST_AUTO_TEST_CASE(test_manual_setting_of_annotations_property)
{
    auto osrm = getOSRM(OSRM_TEST_DATA_DIR "/ch/monaco.osrm");

    using namespace osrm;

    RouteParameters params{};
    params.annotations = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    engine::api::ResultT result = json::Object();
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    auto &json_result = result.get<json::Object>();
    const auto code = json_result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    auto annotations = json_result.values["routes"]
                           .get<json::Array>()
                           .values[0]
                           .get<json::Object>()
                           .values["legs"]
                           .get<json::Array>()
                           .values[0]
                           .get<json::Object>()
                           .values["annotation"]
                           .get<json::Object>()
                           .values;
    BOOST_CHECK_EQUAL(annotations.size(), 6);
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

    auto &fb_result = result.get<flatbuffers::FlatBufferBuilder>();
    auto fb = engine::api::fbresult::GetFBResult(fb_result.GetBufferPointer());
    BOOST_CHECK(!fb->error());

    BOOST_CHECK(fb->waypoints() != nullptr);
    const auto waypoints = fb->waypoints();
    BOOST_CHECK(waypoints->size() == params.coordinates.size());

    for (const auto &waypoint : *waypoints)
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

    for (const auto &route : *routes)
    {
        BOOST_CHECK_EQUAL(route->distance(), 0);
        BOOST_CHECK_EQUAL(route->duration(), 0);

        const auto &legs = route->legs();
        BOOST_CHECK(legs->size() > 0);

        for (const auto &leg : *legs)
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

BOOST_AUTO_TEST_SUITE_END()
