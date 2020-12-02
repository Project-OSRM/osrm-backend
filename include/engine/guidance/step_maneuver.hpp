#ifndef ENGINE_GUIDANCE_STEP_MANEUVER_HPP
#define ENGINE_GUIDANCE_STEP_MANEUVER_HPP

#include "guidance/turn_instruction.hpp"
#include "util/coordinate.hpp"

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
    None = 0,
    Arrive = 1,
    Depart = 2,
    MaxWaypointType = 3
};

struct StepManeuver
{
    util::Coordinate location;
    short bearing_before;
    short bearing_after;
    osrm::guidance::TurnInstruction instruction;

    WaypointType waypoint_type;
    unsigned exit;
};

inline StepManeuver getInvalidStepManeuver()
{
    return {util::Coordinate{util::FloatLongitude{0.0}, util::FloatLatitude{0.0}},
            0,
            0,
            osrm::guidance::TurnInstruction::NO_TURN(),
            WaypointType::None,
            0};
}

} // namespace guidance
} // namespace engine
} // namespace osrm
#endif
