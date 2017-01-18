#include "engine/guidance/assemble_geometry.hpp"
#include "engine/guidance/assemble_leg.hpp"
#include "engine/guidance/assemble_overview.hpp"
#include "engine/guidance/assemble_route.hpp"
#include "engine/guidance/assemble_steps.hpp"
#include "engine/guidance/post_processing.hpp"

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(guidance_assembly)

BOOST_AUTO_TEST_CASE(trim_short_segments)
{
    using namespace osrm::extractor::guidance;
    using namespace osrm::engine::guidance;
    using namespace osrm::engine;
    using namespace osrm::util;

    IntermediateIntersection intersection1{{FloatLongitude{-73.981154}, FloatLatitude{40.767762}},
                                           {302},
                                           {1},
                                           IntermediateIntersection::NO_INDEX,
                                           0,
                                           {0, 255},
                                           {}};
    IntermediateIntersection intersection2{{FloatLongitude{-73.981495}, FloatLatitude{40.768275}},
                                           {180},
                                           {1},
                                           0,
                                           IntermediateIntersection::NO_INDEX,
                                           {0, 255},
                                           {}};

    // Check that duplicated coordinate in the end is removed
    std::vector<RouteStep> steps //
        = {{324,
            "Central Park West",
            "",
            "",
            "",
            "",
            "",
            0.2,
            1.9076601161280742,
            TRAVEL_MODE_DRIVING,
            {{FloatLongitude{-73.981492}, FloatLatitude{40.768258}},
             329,
             348,
             {TurnType::ExitRotary, DirectionModifier::Straight},
             WaypointType::Depart,
             0},
            0,
            3,
            {intersection1}},
           {324,
            "Central Park West",
            "",
            "",
            "",
            "",
            "",
            0,
            0,
            TRAVEL_MODE_DRIVING,
            {{FloatLongitude{-73.981495}, FloatLatitude{40.768275}},
             0,
             0,
             {TurnType::NoTurn, DirectionModifier::UTurn},
             WaypointType::Arrive,
             0},
            2,
            3,
            {intersection2}}};

    LegGeometry geometry;
    geometry.locations = {{FloatLongitude{-73.981492}, FloatLatitude{40.768258}},
                          {FloatLongitude{-73.981495}, FloatLatitude{40.768275}},
                          {FloatLongitude{-73.981495}, FloatLatitude{40.768275}}};
    geometry.segment_offsets = {0, 2};
    geometry.segment_distances = {1.9076601161280742};
    geometry.osm_node_ids = {OSMNodeID{0}, OSMNodeID{1}, OSMNodeID{2}};
    geometry.annotations = {{1.9076601161280742, 0.2, 0}, {0, 0, 0}};

    trimShortSegments(steps, geometry);

    BOOST_CHECK_EQUAL(geometry.segment_distances.size(), 1);
    BOOST_CHECK_EQUAL(geometry.segment_offsets.size(), 2);
    BOOST_CHECK_EQUAL(geometry.segment_offsets.back(), 1);
    BOOST_CHECK_EQUAL(geometry.annotations.size(), 1);
    BOOST_CHECK_EQUAL(geometry.locations.size(), 2);
    BOOST_CHECK_EQUAL(geometry.osm_node_ids.size(), 2);
}

BOOST_AUTO_TEST_CASE(trim_short_segments_keep_first_location)
{
    using namespace osrm::extractor::guidance;
    using namespace osrm::engine::guidance;
    using namespace osrm::engine;
    using namespace osrm::util;

    std::vector<RouteStep> steps = //
        {{1508,
          "West 35 Street",
          "",
          "",
          "",
          "",
          "",
          7.3,
          0.111, //
          TRAVEL_MODE_DRIVING,
          {{FloatLongitude{-73.990134}, FloatLatitude{40.751657}},
           0,
           0,
           {TurnType::NoTurn, DirectionModifier::UTurn},
           osrm::engine::guidance::WaypointType::Depart,
           0},
          0,
          2,
          {{{FloatLongitude{-73.990134}, FloatLatitude{40.751657}},
            {0},
            {1},
            IntermediateIntersection::NO_INDEX,
            0,
            {0, 255},
            {}}}},

         {568,
          "7th Avenue",
          "",
          "",
          "",
          "",
          "",
          2.5,
          22.488,
          TRAVEL_MODE_DRIVING,
          {{FloatLongitude{-73.990134}, FloatLatitude{40.751658}},
           298,
           208,
           {TurnType::Turn, DirectionModifier::Left},
           osrm::engine::guidance::WaypointType::None,
           0},
          1,
          3,
          {{{FloatLongitude{-73.990134}, FloatLatitude{40.751658}},
            {30, 120, 210, 300},
            {0, 0, 1, 1},
            1,
            2,
            {0, 255},
            {}}}},

         {568,
          "7th Avenue",
          "",
          "",
          "",
          "",
          "",
          0,
          0,
          TRAVEL_MODE_DRIVING,
          {{FloatLongitude{-73.990263}, FloatLatitude{40.751481}},
           209,
           0,
           {TurnType::NoTurn, DirectionModifier::UTurn},
           osrm::engine::guidance::WaypointType::Arrive,
           0},
          2,
          3,
          {{{FloatLongitude{-73.990263}, FloatLatitude{40.751481}},
            {29},
            {1},
            0,
            IntermediateIntersection::NO_INDEX,
            {0, 255},
            {}}}}};

    LegGeometry geometry = {{{FloatLongitude{-73.990134}, FloatLatitude{40.751657}},
                             {FloatLongitude{-73.990134}, FloatLatitude{40.751658}},
                             {FloatLongitude{-73.990263}, FloatLatitude{40.751481}}},
                            {0, 1, 2},
                            {0.111, 22.488},
                            {OSMNodeID{1329098617}, OSMNodeID{42439952}, OSMNodeID{42437654}},
                            {{0.111, 7.3, 0}, {22.488, 2.5, 0}}};

    trimShortSegments(steps, geometry);

    BOOST_REQUIRE_EQUAL(steps.size(), 2);
    BOOST_REQUIRE_EQUAL(geometry.segment_distances.size(), 1);
    BOOST_REQUIRE_EQUAL(geometry.segment_offsets.size(), 2);
    BOOST_REQUIRE_EQUAL(geometry.annotations.size(), 1);
    BOOST_REQUIRE_EQUAL(geometry.locations.size(), 2);
    BOOST_REQUIRE_EQUAL(geometry.osm_node_ids.size(), 2);

    Coordinate start_location{FloatLongitude{-73.990134}, FloatLatitude{40.751657}};
    BOOST_CHECK_EQUAL(geometry.locations.front(), start_location);
    BOOST_CHECK_EQUAL(geometry.segment_distances.front(), 0.111 + 22.488);
    BOOST_CHECK_EQUAL(geometry.annotations.front().distance, 0.111 + 22.488);
    BOOST_CHECK_EQUAL(geometry.annotations.front().duration, 7.3 + 2.5);
    BOOST_CHECK_EQUAL(steps.front().maneuver.location, start_location);
    BOOST_CHECK(steps.front().maneuver.waypoint_type == WaypointType::Depart);
    BOOST_CHECK_EQUAL(steps.front().name, "West 35 Street");
}

BOOST_AUTO_TEST_SUITE_END()
