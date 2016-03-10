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
                                      const LegGeometry &leg_geometry,
                                      const std::size_t segment_index,
                                      const unsigned exit)
{
    auto turn_index = leg_geometry.BackIndex(segment_index);
    BOOST_ASSERT(turn_index > 0);
    BOOST_ASSERT(turn_index < leg_geometry.locations.size() - 1);

    // TODO chose a bigger look-a-head to smooth complex geometry
    const auto pre_turn_coordinate = leg_geometry.locations[turn_index - 1];
    const auto turn_coordinate = leg_geometry.locations[turn_index];
    const auto post_turn_coordinate = leg_geometry.locations[turn_index + 1];

    const double pre_turn_bearing =
        util::coordinate_calculation::bearing(pre_turn_coordinate, turn_coordinate);
    const double post_turn_bearing =
        util::coordinate_calculation::bearing(turn_coordinate, post_turn_coordinate);

    return StepManeuver{turn_coordinate, pre_turn_bearing, post_turn_bearing, instruction, waypoint_type, exit};
}
} // ns detail
} // ns engine
} // ns guidance
} // ns detail
