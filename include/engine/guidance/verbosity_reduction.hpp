#ifndef OSRM_ENGINE_GUIDANCE_VERBOSITY_REDUCTION_HPP_
#define OSRM_ENGINE_GUIDANCE_VERBOSITY_REDUCTION_HPP_

#include "engine/guidance/route_step.hpp"
#include "util/attributes.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

// Name changes on roads are posing relevant information. However if they are short, we don't want
// to announce them. All these that are not collapsed into a single turn (think segregated
// intersection) have to be checked for the length they are active in. If they are active for a
// short distance only, we don't announce them
OSRM_ATTR_WARN_UNUSED
std::vector<RouteStep> suppressShortNameSegments(std::vector<RouteStep> steps);

// remove use lane information that is not actually a turn. For post-processing, we need to
// associate lanes with every turn. Some of these use-lane instructions are not required after lane
// anticipation anymore. This function removes all use lane instructions that are not actually used
// anymore since all lanes going straight are used anyhow.
// FIXME this is currently only a heuristic. We need knowledge on which lanes actually might become
// turn lanes. If a straight lane becomes a turn lane, this might be something to consider. Right
// now we bet on lane-anticipation to catch this.
OSRM_ATTR_WARN_UNUSED
std::vector<RouteStep> collapseUseLane(std::vector<RouteStep> steps);

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif /* OSRM_ENGINE_GUIDANCE_VERBOSITY_REDUCTION_HPP_ */
