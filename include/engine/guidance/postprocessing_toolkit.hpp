#ifndef OSRM_ENGINE_GUIDANCE_POSTPROCESSING_TOOLKIT_HPP_
#define OSRM_ENGINE_GUIDANCE_POSTPROCESSING_TOOLKIT_HPP_

#include "guidance/turn_instruction.hpp"
#include "engine/guidance/route_step.hpp"

#include <iterator>
#include <utility>

namespace osrm
{
namespace engine
{
namespace guidance
{

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

#endif /* OSRM_ENGINE_GUIDANCE_POSTPROCESSING_TOOLKIT_HPP_ */
