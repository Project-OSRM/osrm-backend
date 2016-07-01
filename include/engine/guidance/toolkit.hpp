#ifndef OSRM_ENGINE_GUIDANCE_TOOLKIT_HPP_
#define OSRM_ENGINE_GUIDANCE_TOOLKIT_HPP_

#include "extractor/guidance/turn_instruction.hpp"
#include "util/guidance/toolkit.hpp"
#include "util/bearing.hpp"

#include <algorithm>

namespace osrm
{
namespace engine
{
namespace guidance
{

using util::guidance::entersRoundabout;
using util::guidance::leavesRoundabout;
using util::guidance::staysOnRoundabout;

// Silent Turn Instructions are not to be mentioned to the outside world but
inline bool isSilent(const extractor::guidance::TurnInstruction instruction)
{
    return instruction.type == extractor::guidance::TurnType::NoTurn ||
           instruction.type == extractor::guidance::TurnType::Suppressed ||
           instruction.type == extractor::guidance::TurnType::StayOnRoundabout;
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
