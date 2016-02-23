#ifndef ENGINE_GUIDANCE_STEP_MANEUVER_HPP
#define ENGINE_GUIDANCE_STEP_MANEUVER_HPP

#include "util/coordinate.hpp"
#include "extractor/turn_instructions.hpp"

namespace osrm
{
namespace engine
{
namespace guidance
{

struct StepManeuver
{
    util::Coordinate location;
    double bearing_before;
    double bearing_after;
    extractor::TurnInstruction instruction;
};
}
}
}
#endif
