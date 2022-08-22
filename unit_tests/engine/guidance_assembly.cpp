#include "extractor/travel_mode.hpp"

#include "engine/guidance/assemble_geometry.hpp"
#include "engine/guidance/assemble_leg.hpp"
#include "engine/guidance/assemble_overview.hpp"
#include "engine/guidance/assemble_route.hpp"
#include "engine/guidance/assemble_steps.hpp"
#include "engine/guidance/post_processing.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(guidance_assembly)

BOOST_AUTO_TEST_CASE(trim_short_segments)
{
    using namespace osrm::extractor;
    using namespace osrm::guidance;
    using namespace osrm::engine::guidance;
    using namespace osrm::engine;
    using namespace osrm::util;

    IntermediateIntersection intersection1{{FloatLongitude{-73.981154}, FloatLatitude{40.767762}},
                                           {302},
                                           {1},
                                           IntermediateIntersection::NO_INDEX,
                                           0,
                                           {0, 255},
                                           {},
                                           {}};
    IntermediateIntersection intersection2{{FloatLongitude{-73.981495}, FloatLatitude{40.768275}},
                                           {180},
                                           {1},
                                           0,
                                           IntermediateIntersection::NO_INDEX,
                                           {0, 255},
                                           {},
                                           {}};

    // Check that duplicated coordinate in the end is removed
    std::vector<RouteStep> steps = {{0,
                                     324,
                                     false,
                                     "Central Park West",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     0.2,
                                     1.9076601161280742,
                                     0.2,
                                     TRAVEL_MODE_DRIVING,
                                     {{FloatLongitude{-73.981492}, FloatLatitude{40.768258}},
                                      329,
                                      348,
                                      {TurnType::ExitRotary, DirectionModifier::Straight},
                                      WaypointType::Depart,
                                      0},
                                     0,
                                     3,
                                     {intersection1},
                                     false},
                                    {0,
                                     324,
                                     false,
                                     "Central Park West",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     0,
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
                                     {intersection2},
                                     false}};

    LegGeometry geometry;
    geometry.locations = {{FloatLongitude{-73.981492}, FloatLatitude{40.768258}},
                          {FloatLongitude{-73.981495}, FloatLatitude{40.768275}},
                          {FloatLongitude{-73.981495}, FloatLatitude{40.768275}}};
    geometry.segment_offsets = {0, 2};
    geometry.segment_distances = {1.9076601161280742};
    geometry.node_ids = {NodeID{0}, NodeID{1}, NodeID{2}};
    geometry.annotations = {{1.9076601161280742, 0.2, 0.2, 0}, {0, 0, 0, 0}};

    trimShortSegments(steps, geometry);

    BOOST_CHECK_EQUAL(geometry.segment_distances.size(), 1);
    BOOST_CHECK_EQUAL(geometry.segment_offsets.size(), 2);
    BOOST_CHECK_EQUAL(geometry.segment_offsets.back(), 1);
    BOOST_CHECK_EQUAL(geometry.annotations.size(), 1);
    BOOST_CHECK_EQUAL(geometry.locations.size(), 2);
    BOOST_CHECK_EQUAL(geometry.node_ids.size(), 2);
}

BOOST_AUTO_TEST_SUITE_END()
