#ifndef ENGINE_GUIDANCE_STEP_MANEUVER_HPP
#define ENGINE_GUIDANCE_STEP_MANEUVER_HPP

#include "extractor/guidance/turn_instruction.hpp"
#include "util/coordinate.hpp"

#include <cstdint>
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
    extractor::guidance::TurnInstruction instruction;
    WaypointType waypoint_type;
    unsigned exit;
};

inline StepManeuver getInvalidStepManeuver()
{
    return {extractor::guidance::TurnInstruction::NO_TURN(), WaypointType::None, 0};
}

} // namespace guidance
} // namespace engine
} // namespace osrmn
#endif
