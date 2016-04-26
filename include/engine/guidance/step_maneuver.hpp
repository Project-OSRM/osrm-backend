#ifndef ENGINE_GUIDANCE_STEP_MANEUVER_HPP
#define ENGINE_GUIDANCE_STEP_MANEUVER_HPP

#include "extractor/guidance/turn_instruction.hpp"
#include "util/coordinate.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"

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

// A represenetation of intermediate intersections
struct IntermediateIntersection
{
    double duration;
    double distance;
    util::Coordinate location;
    util::guidance::EntryClass entry_class;
    util::guidance::BearingClass bearing_class;
};

struct StepManeuver
{
    util::Coordinate location;
    double bearing_before;
    double bearing_after;
    extractor::guidance::TurnInstruction instruction;
    WaypointType waypoint_type;
    unsigned exit;
    util::guidance::EntryClass entry_class;
    util::guidance::BearingClass bearing_class;
    std::vector<IntermediateIntersection> intersections;
};

inline StepManeuver getInvalidStepManeuver()
{
    return {util::Coordinate{util::FloatLongitude{0.0}, util::FloatLatitude{0.0}},
            0,
            0,
            extractor::guidance::TurnInstruction::NO_TURN(),
            WaypointType::None,
            0,
            {}};
}

} // namespace guidance
} // namespace engine
} // namespace osrmn
#endif
