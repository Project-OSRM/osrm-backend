#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "args.hpp"
#include "coordinates.hpp"
#include "equal_json.hpp"
#include "fixture.hpp"

#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/status.hpp"

BOOST_AUTO_TEST_SUITE(route)

BOOST_AUTO_TEST_CASE(test_route_same_coordinates_fixture)
{
    const auto args = get_args();
    auto osrm = getOSRM(args.at(0));

    using namespace osrm;

    RouteParameters params;
    params.steps = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    json::Object result;
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    // unset snapping dependent hint
    for (auto &itr : result.values["waypoints"].get<json::Array>().values)
        itr.get<json::Object>().values["hint"] = "";

    const auto location = json::Array{{{7.437070}, {43.749247}}};

    json::Object reference{
        {{"code", "Ok"},
         {"waypoints",
          json::Array{
              {json::Object{
                   {{"name", "Boulevard du Larvotto"}, {"location", location}, {"hint", ""}}},
               json::Object{
                   {{"name", "Boulevard du Larvotto"}, {"location", location}, {"hint", ""}}}}}},
         {"routes",
          json::Array{{json::Object{
              {{"distance", 0.},
               {"duration", 0.},
               {"geometry", "yw_jGupkl@??"},
               {"legs",
                json::Array{{json::Object{
                    {{"distance", 0.},
                     {"duration", 0.},
                     {"summary", ""},
                     {"steps",
                      json::Array{{{json::Object{{{"duration", 0.},
                                                  {"distance", 0.},
                                                  {"geometry", "yw_jGupkl@??"},
                                                  {"name", "Boulevard du Larvotto"},
                                                  {"mode", "driving"},
                                                  {"maneuver", json::Object{{
                                                                   {"type", "depart"},
                                                               }}},
                                                  {"intersections",
                                                   json::Array{{json::Object{
                                                       {{"location", location},
                                                        {"bearings", json::Array{{0, 180}}},
                                                        {"entry", json::Array{{"true", "true"}}},
                                                        {"out", 0}}}}}}}}},

                                   json::Object{{{"duration", 0.},
                                                 {"distance", 0.},
                                                 {"geometry", "yw_jGupkl@"},
                                                 {"name", "Boulevard du Larvotto"},
                                                 {"mode", "driving"},
                                                 {"maneuver", json::Object{{{"type", "arrive"}}}},
                                                 {"intersections",
                                                  json::Array{{json::Object{
                                                      {{"location", location},
                                                       {"bearings", json::Array{{0, 180}}},
                                                       {"entry", json::Array{{"true", "true"}}},
                                                       {"in", 1}}}}}}

                                   }}}}}}}}}}}}}}}}};

    CHECK_EQUAL_JSON(reference, result);
}

BOOST_AUTO_TEST_CASE(test_route_same_coordinates)
{
    const auto args = get_args();
    auto osrm = getOSRM(args.at(0));

    using namespace osrm;

    RouteParameters params;
    params.steps = true;
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());
    params.coordinates.push_back(get_dummy_location());

    json::Object result;
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = result.values.at("waypoints").get<json::Array>().values;
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

    const auto &routes = result.values.at("routes").get<json::Array>().values;
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
                    const auto location = intersection_object.at("location").get<json::Array>().values;
                    const auto longitude = location[0].get<json::Number>().value;
                    const auto latitude = location[1].get<json::Number>().value;
                    BOOST_CHECK(longitude >= -180. && longitude <= 180.);
                    BOOST_CHECK(latitude >= -90. && latitude <= 90.);

                    const auto &bearings = intersection_object.at("bearings").get<json::Array>().values;
                    BOOST_CHECK(!bearings.empty());
                    const auto &entries = intersection_object.at("entry").get<json::Array>().values;
                    BOOST_CHECK(bearings.size() == entries.size());

                    for( const auto bearing : bearings )
                        BOOST_CHECK( 0. <= bearing.get<json::Number>().value && bearing.get<json::Number>().value <= 360. );

                    if( step_count > 0 )
                    {
                        const auto in = intersection_object.at("in").get<json::Number>().value;
                        BOOST_CHECK(in < bearings.size());
                    }
                    if( step_count + 1 < steps.size() )
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
    const auto args = get_args();
    auto osrm = getOSRM(args.at(0));

    using namespace osrm;

    const auto locations = get_locations_in_small_component();

    RouteParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    json::Object result;
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = result.values.at("waypoints").get<json::Array>().values;
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
    const auto args = get_args();
    auto osrm = getOSRM(args.at(0));

    using namespace osrm;

    const auto locations = get_locations_in_big_component();

    RouteParameters params;
    params.coordinates.push_back(locations.at(0));
    params.coordinates.push_back(locations.at(1));
    params.coordinates.push_back(locations.at(2));

    json::Object result;
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = result.values.at("waypoints").get<json::Array>().values;
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
    const auto args = get_args();
    auto osrm = getOSRM(args.at(0));

    using namespace osrm;

    const auto big_component = get_locations_in_big_component();
    const auto small_component = get_locations_in_small_component();

    RouteParameters params;
    params.coordinates.push_back(small_component.at(0));
    params.coordinates.push_back(big_component.at(0));
    params.coordinates.push_back(small_component.at(1));
    params.coordinates.push_back(big_component.at(1));

    json::Object result;
    const auto rc = osrm.Route(params, result);
    BOOST_CHECK(rc == Status::Ok);

    const auto code = result.values.at("code").get<json::String>().value;
    BOOST_CHECK_EQUAL(code, "Ok");

    const auto &waypoints = result.values.at("waypoints").get<json::Array>().values;
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

BOOST_AUTO_TEST_SUITE_END()
