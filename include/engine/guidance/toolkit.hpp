#ifndef OSRM_ENGINE_GUIDANCE_TOOLKIT_HPP_
#define OSRM_ENGINE_GUIDANCE_TOOLKIT_HPP_

#include "extractor/guidance/turn_instruction.hpp"
#include "util/bearing.hpp"

#include <algorithm>

namespace osrm
{
namespace engine
{
namespace guidance
{

// Silent Turn Instructions are not to be mentioned to the outside world but
inline bool isSilent(const extractor::guidance::TurnInstruction instruction)
{
    return instruction.type == extractor::guidance::TurnType::NoTurn ||
           instruction.type == extractor::guidance::TurnType::Suppressed ||
           instruction.type == extractor::guidance::TurnType::StayOnRoundabout;
}

inline bool entersRoundabout(const extractor::guidance::TurnInstruction instruction)
{
    return (instruction.type == extractor::guidance::TurnType::EnterRoundabout ||
            instruction.type == extractor::guidance::TurnType::EnterRotary ||
            instruction.type == extractor::guidance::TurnType::EnterRoundaboutIntersection ||
            instruction.type == extractor::guidance::TurnType::EnterRoundaboutAtExit ||
            instruction.type == extractor::guidance::TurnType::EnterRotaryAtExit ||
            instruction.type == extractor::guidance::TurnType::EnterRoundaboutIntersectionAtExit ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRoundabout ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRotary ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRotary);
}

inline bool leavesRoundabout(const extractor::guidance::TurnInstruction instruction)
{
    return (instruction.type == extractor::guidance::TurnType::ExitRoundabout ||
            instruction.type == extractor::guidance::TurnType::ExitRotary ||
            instruction.type == extractor::guidance::TurnType::ExitRoundaboutIntersection ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRoundabout ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRotary ||
            instruction.type == extractor::guidance::TurnType::EnterAndExitRoundaboutIntersection);
}

inline bool staysOnRoundabout(const extractor::guidance::TurnInstruction instruction)
{
    return instruction.type == extractor::guidance::TurnType::StayOnRoundabout;
}

inline extractor::guidance::DirectionModifier::Enum angleToDirectionModifier(const double bearing)
{
    if (bearing < 135)
    {
        return extractor::guidance::DirectionModifier::Right;
    }

    if (bearing <= 225)
    {
        return extractor::guidance::DirectionModifier::Straight;
    }
    return extractor::guidance::DirectionModifier::Left;
}

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif /* OSRM_ENGINE_GUIDANCE_TOOLKIT_HPP_ */
