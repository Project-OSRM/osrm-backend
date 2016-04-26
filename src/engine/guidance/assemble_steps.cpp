#include "engine/guidance/assemble_steps.hpp"

#include <boost/assert.hpp>

#include <cstddef>

namespace osrm
{
namespace engine
{
namespace guidance
{
namespace detail
{

StepManeuver stepManeuverFromGeometry(extractor::guidance::TurnInstruction instruction,
                                      const WaypointType waypoint_type,
                                      const LegGeometry &leg_geometry)
{
    BOOST_ASSERT(waypoint_type != WaypointType::None);
    BOOST_ASSERT(leg_geometry.locations.size() >= 2);

    double pre_turn_bearing = 0, post_turn_bearing = 0;
    Coordinate turn_coordinate;
    if (waypoint_type == WaypointType::Depart)
    {
        turn_coordinate = leg_geometry.locations.front();
        const auto post_turn_coordinate = *(leg_geometry.locations.begin() + 1);
        post_turn_bearing =
            util::coordinate_calculation::bearing(turn_coordinate, post_turn_coordinate);
    }
    else
    {
        BOOST_ASSERT(waypoint_type == WaypointType::Arrive);
        turn_coordinate = leg_geometry.locations.back();
        const auto pre_turn_coordinate = *(leg_geometry.locations.end() - 2);
        pre_turn_bearing =
            util::coordinate_calculation::bearing(pre_turn_coordinate, turn_coordinate);
    }
    return StepManeuver{std::move(turn_coordinate),
                        pre_turn_bearing,
                        post_turn_bearing,
                        std::move(instruction),
                        waypoint_type,
                        INVALID_EXIT_NR,
                        // BearingClass,EntryClass, and Intermediate intersections are unknown yet
                        {},
                        {},
                        {}};
}

StepManeuver stepManeuverFromGeometry(extractor::guidance::TurnInstruction instruction,
                                      const LegGeometry &leg_geometry,
                                      const std::size_t segment_index,
                                      util::guidance::EntryClass entry_class,
                                      util::guidance::BearingClass bearing_class)
{
    auto turn_index = leg_geometry.BackIndex(segment_index);
    BOOST_ASSERT(turn_index > 0);
    BOOST_ASSERT(turn_index + 1 < leg_geometry.locations.size());

    // TODO chose a bigger look-a-head to smooth complex geometry
    const auto pre_turn_coordinate = leg_geometry.locations[turn_index - 1];
    const auto turn_coordinate = leg_geometry.locations[turn_index];
    const auto post_turn_coordinate = leg_geometry.locations[turn_index + 1];

    const double pre_turn_bearing =
        util::coordinate_calculation::bearing(pre_turn_coordinate, turn_coordinate);
    const double post_turn_bearing =
        util::coordinate_calculation::bearing(turn_coordinate, post_turn_coordinate);

    // add a step without intermediate intersections
    return StepManeuver{std::move(turn_coordinate), pre_turn_bearing,         post_turn_bearing,
                        std::move(instruction),     WaypointType::None,       INVALID_EXIT_NR,
                        std::move(entry_class),     std::move(bearing_class), {}};
}
} // ns detail
} // ns engine
} // ns guidance
} // ns detail
