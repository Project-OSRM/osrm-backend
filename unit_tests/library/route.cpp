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

#include "util/timing_util.hpp"

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
                     {"steps", json::Array{{json::Object{{{"duration", 0.},
                                                          {"distance", 0.},
                                                          {"geometry", "yw_jGupkl@??"},
                                                          {"name", "Boulevard du Larvotto"},
                                                          {"mode", "driving"},
                                                          {"maneuver", json::Object{{
                                                                           {"type", "depart"},
                                                                           {"location", location},
                                                                           {"bearing_before", 0.},
                                                                           {"bearing_after", 0.},
                                                                       }}}}},

                                            json::Object{{{"duration", 0.},
                                                          {"distance", 0.},
                                                          {"geometry", "yw_jGupkl@"},
                                                          {"name", "Boulevard du Larvotto"},
                                                          {"mode", "driving"},
                                                          {"maneuver", json::Object{{
                                                                           {"type", "arrive"},
                                                                           {"location", location},
                                                                           {"bearing_before", 0.},
                                                                           {"bearing_after", 0.},
                                                                       }}}}}}}}}}}}}}}}}}}};

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

                const auto location = maneuver.at("location").get<json::Array>().values;
                const auto longitude = location[0].get<json::Number>().value;
                const auto latitude = location[1].get<json::Number>().value;
                BOOST_CHECK(longitude >= -180. && longitude <= 180.);
                BOOST_CHECK(latitude >= -90. && latitude <= 90.);

                const auto bearing_before = maneuver.at("bearing_before").get<json::Number>().value;
                const auto bearing_after = maneuver.at("bearing_after").get<json::Number>().value;
                BOOST_CHECK(bearing_before >= 0. && bearing_before <= 360.);
                BOOST_CHECK(bearing_after >= 0. && bearing_after <= 360.);

                const auto type = maneuver.at("type").get<json::String>().value;
                BOOST_CHECK(!type.empty());

                // modifier is optional
                // TODO(daniel-j-h):

                // exit is optional
                // TODO(daniel-j-h):
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

BOOST_AUTO_TEST_CASE(test_benchmark)
{
    const auto args = get_args();
    auto osrm = getOSRM(args.at(0));

    using namespace osrm;

    RouteParameters params;

    using osrm::util::FloatCoordinate;
    using osrm::util::FloatLatitude;
    using osrm::util::FloatLongitude;

    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.413308443209877},FloatLatitude{43.726427577246234}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.419132303497943},FloatLatitude{43.74625561548354}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.436112336625515},FloatLatitude{43.75065992944102}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.415080246296296},FloatLatitude{43.724245951217426}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.419321958436214},FloatLatitude{43.738738372170786}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.431798954526749},FloatLatitude{43.73885669847394}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.425249604938272},FloatLatitude{43.74384414000343}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.4191465022633745},FloatLatitude{43.72780983256173}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.444645794650206},FloatLatitude{43.74568439343279}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.417528519135803},FloatLatitude{43.73798881027092}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.431749934979424},FloatLatitude{43.75125666332305}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.448364856995885},FloatLatitude{43.72390968098423}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.444492988888888},FloatLatitude{43.74446662868656}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.430844256584362},FloatLatitude{43.7335055311214}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.416668141563786},FloatLatitude{43.73965922779493}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.44042301419753},FloatLatitude{43.74438426191701}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.429573467078189},FloatLatitude{43.74068310262346}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.44156364835391},FloatLatitude{43.74569970053155}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.418462595061729},FloatLatitude{43.74718837662895}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.441081566460905},FloatLatitude{43.72555604449589}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.417899377366255},FloatLatitude{43.75147776586077}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.431187731481481},FloatLatitude{43.73398563948903}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.43908055473251},FloatLatitude{43.72591102340535}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.437683328600823},FloatLatitude{43.73810009044925}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.41287842345679},FloatLatitude{43.74256271716393}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.436714431893004},FloatLatitude{43.751194706018524}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.435195502057613},FloatLatitude{43.74335334096365}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.4161106709876545},FloatLatitude{43.7256862763203}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.447127197942387},FloatLatitude{43.725833759002064}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.423579897736626},FloatLatitude{43.748344184070646}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.414596474074074},FloatLatitude{43.74948298362483}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.444030852880658},FloatLatitude{43.74429484902264}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.41598051563786},FloatLatitude{43.734179286436905}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.42147983271605},FloatLatitude{43.73837950574417}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.440569396707819},FloatLatitude{43.73100464274692}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.425541355761316},FloatLatitude{43.738645314729084}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.41047274691358},FloatLatitude{43.730162509345}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.410454829423868},FloatLatitude{43.72374980684157}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.425974418106996},FloatLatitude{43.74284893561386}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.434151216666667},FloatLatitude{43.73427866109397}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.445695151028806},FloatLatitude{43.731977007973256}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.430423702674897},FloatLatitude{43.727896815757894}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.4267952419753085},FloatLatitude{43.731834627657754}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.441138361522634},FloatLatitude{43.736792912808646}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.412601209465021},FloatLatitude{43.73157562182785}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.440980822839506},FloatLatitude{43.732277075703024}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.40992846090535},FloatLatitude{43.747644188014405}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.425082938477366},FloatLatitude{43.7349920204904}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.410987959259259},FloatLatitude{43.74505267189644}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.444073449176955},FloatLatitude{43.7456375002572}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.4415808897119335},FloatLatitude{43.74759267841221}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.428824651234568},FloatLatitude{43.751268082904666}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.438421326337449},FloatLatitude{43.74021951620371}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.435807063168724},FloatLatitude{43.750484262259945}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.428770898765432},FloatLatitude{43.73202997539438}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.445548092386831},FloatLatitude{43.74083690252058}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.420905458847737},FloatLatitude{43.724773438700275}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.423970701851852},FloatLatitude{43.72512501603224}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.439165747325102},FloatLatitude{43.72859632587449}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.410020076748971},FloatLatitude{43.73658687439987}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.427568060493827},FloatLatitude{43.73999987148491}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.432418291152263},FloatLatitude{43.7412844529321}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.416294916872428},FloatLatitude{43.73869123602538}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.413274974691358},FloatLatitude{43.72942120841907}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.438449723868312},FloatLatitude{43.731667950360084}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.413441979218107},FloatLatitude{43.74053319024349}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.433667444444445},FloatLatitude{43.73117569350138}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.430972045473251},FloatLatitude{43.73753348482511}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.414605263786008},FloatLatitude{43.751559403720854}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.423025469753086},FloatLatitude{43.72636999339849}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.423129255967078},FloatLatitude{43.73998772299383}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.432928770576131},FloatLatitude{43.724536543124145}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.433925050617284},FloatLatitude{43.7244507747771}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.428065355349794},FloatLatitude{43.748477331532925}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.442124499588477},FloatLatitude{43.72559127512003}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.440078187037037},FloatLatitude{43.734864704303845}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.419492343621399},FloatLatitude{43.74410897710906}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.415904450823045},FloatLatitude{43.735830266375174}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.4151968790123455},FloatLatitude{43.73871844864541}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.410554220781893},FloatLatitude{43.73632932638889}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.446276624279835},FloatLatitude{43.73636018355624}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.41185712654321},FloatLatitude{43.74545867446845}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.414394986831276},FloatLatitude{43.7345850460391}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.428089019958848},FloatLatitude{43.73662769332991}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.44320292962963},FloatLatitude{43.73619204843965}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.444158641769547},FloatLatitude{43.74832280272634}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.413917637860083},FloatLatitude{43.74607946236283}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.443514288271604},FloatLatitude{43.74870523722566}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.434125185596708},FloatLatitude{43.730309263117285}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.416906477983539},FloatLatitude{43.745822157321676}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.425935202469136},FloatLatitude{43.73576490749314}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.436870618312757},FloatLatitude{43.746671093878604}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.410767540329218},FloatLatitude{43.74530244487312}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.443041672222222},FloatLatitude{43.735157725908785}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.4261069399176955},FloatLatitude{43.75017496167696}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.448076824897119},FloatLatitude{43.72562699168382}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.429113697530864},FloatLatitude{43.727990359139234}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.414978150411523},FloatLatitude{43.75026753317901}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.4236823316872425},FloatLatitude{43.72458246442044}});
    params.coordinates.push_back(FloatCoordinate{FloatLongitude{7.436727278395062},FloatLatitude{43.7237094738511}});

    TIMER_START(routes);
    auto NUM = 100;
    for (int i = 0; i < NUM; ++i)
    {
        json::Object result;
        const auto rc = osrm.Route(params, result);
        BOOST_CHECK(rc == Status::Ok);
        auto& geometry = result.values.at("routes").get<json::Array>().values[0].get<json::Object>().values.at("geometry").get<json::String>().value;
        BOOST_CHECK_EQUAL(geometry.size(), 9972);
    }
    TIMER_STOP(routes);
    std::cout << (TIMER_MSEC(routes) / NUM) << "ms/req" << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
