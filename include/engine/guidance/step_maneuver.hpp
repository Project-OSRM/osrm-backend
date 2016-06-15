#ifndef ENGINE_GUIDANCE_STEP_MANEUVER_HPP
#define ENGINE_GUIDANCE_STEP_MANEUVER_HPP

#include "extractor/guidance/turn_instruction.hpp"
#include "util/coordinate.hpp"
#include "util/guidance/turn_lanes.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

enum class WaypointType : std::uint8_t
{
    None,
    Arrive,
    Depart,
};

struct StepManeuver
{
    util::Coordinate location;
    short bearing_before;
    short bearing_after;
    extractor::guidance::TurnInstruction instruction;

    WaypointType waypoint_type;
    unsigned exit;

    util::guidance::LaneTupel lanes;
    std::string turn_lane_string;
};

inline StepManeuver getInvalidStepManeuver()
{
    return {util::Coordinate{util::FloatLongitude{0.0}, util::FloatLatitude{0.0}},
            0,
            0,
            extractor::guidance::TurnInstruction::NO_TURN(),
            WaypointType::None,
            0,
            util::guidance::LaneTupel(),
            ""};
}

} // namespace guidance
} // namespace engine
} // namespace osrmn
#endif
