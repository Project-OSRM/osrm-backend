#ifndef OSRM_ENGINE_GUIDANCE_TOOLKIT_HPP_
#define OSRM_ENGINE_GUIDANCE_TOOLKIT_HPP_

#include "extractor/guidance/turn_instruction.hpp"
#include "engine/guidance/route_step.hpp"
#include "util/bearing.hpp"
#include "util/guidance/toolkit.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

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

// Runs fn on RouteStep sub-ranges determined to be roundabouts.
// The function fn is getting called with a roundabout range as in: [enter, .., leave].
//
// The following situations are taken care for (i.e. we discard them):
//  - partial roundabout: enter without exit or exit without enter
//  - data issues: no roundabout, exit before enter
template <typename Iter, typename Fn> inline Fn forEachRoundabout(Iter first, Iter last, Fn fn)
{
    while (first != last)
    {
        const auto enter = std::find_if(first, last, [](const RouteStep &step) {
            return entersRoundabout(step.maneuver.instruction);
        });

        // enter has to come before leave, otherwise: faulty data / partial roundabout, skip those
        const auto leave = std::find_if(enter, last, [](const RouteStep &step) {
            return leavesRoundabout(step.maneuver.instruction);
        });

        // No roundabouts, or partial one (like start / end inside a roundabout)
        if (enter == last || leave == last)
            break;

        (void)fn(std::make_pair(enter, leave));

        // Skip to first step after the currently handled enter / leave pair
        first = std::next(leave);
    }

    return fn;
}

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif /* OSRM_ENGINE_GUIDANCE_TOOLKIT_HPP_ */
