#ifndef OSRM_UTIL_GUIDANCE_TOOLKIT_HPP_
#define OSRM_UTIL_GUIDANCE_TOOLKIT_HPP_

#include "engine/guidance/turn_instruction.hpp"
#include "util/bearing.hpp"

namespace osrm
{
namespace engine
{
namespace guidance
{

// Silent Turn Instructions are not to be mentioned to the outside world but
inline bool isSilent(const TurnInstruction instruction)
{
    return instruction.type == TurnType::NoTurn || instruction.type == TurnType::Suppressed ||
           instruction.type == TurnType::StayOnRoundabout;
}

inline bool entersRoundabout(const TurnInstruction instruction)
{
    return (instruction.type == TurnType::EnterRoundabout ||
            instruction.type == TurnType::EnterRotary ||
            instruction.type == TurnType::EnterRoundaboutAtExit ||
            instruction.type == TurnType::EnterRotaryAtExit ||
            instruction.type == TurnType::EnterAndExitRoundabout ||
            instruction.type == TurnType::EnterAndExitRotary);
}

inline bool leavesRoundabout(const TurnInstruction instruction)
{
    return (instruction.type == TurnType::ExitRoundabout ||
            instruction.type == TurnType::ExitRotary ||
            instruction.type == TurnType::EnterAndExitRoundabout ||
            instruction.type == TurnType::EnterAndExitRotary);
}

inline bool staysOnRoundabout(const TurnInstruction instruction)
{
    return instruction.type == TurnType::StayOnRoundabout;
}

inline DirectionModifier angleToDirectionModifier(const double bearing)
{
    if (bearing < 135)
    {
        return DirectionModifier::Right;
    }

    if (bearing <= 225)
    {
        return DirectionModifier::Straight;
    }
    return DirectionModifier::Left;
}

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif /* OSRM_UTIL_GUIDANCE_TOOLKIT_HPP_ */
