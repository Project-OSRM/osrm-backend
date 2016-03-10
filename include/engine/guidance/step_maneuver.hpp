#ifndef ENGINE_GUIDANCE_STEP_MANEUVER_HPP
#define ENGINE_GUIDANCE_STEP_MANEUVER_HPP

#include "util/coordinate.hpp"
#include "extractor/guidance/turn_instruction.hpp"

#include <cstdint>

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
    double bearing_before;
    double bearing_after;
    extractor::guidance::TurnInstruction instruction;
    WaypointType waypoint_type;
    unsigned exit;
};
} // namespace guidance
} // namespace engine
} // namespace osrmn
#endif
