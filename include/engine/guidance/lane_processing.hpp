#ifndef OSRM_ENGINE_GUIDANCE_LANE_PROCESSING_HPP_
#define OSRM_ENGINE_GUIDANCE_LANE_PROCESSING_HPP_

#include <vector>

#include "engine/guidance/route_step.hpp"
#include "util/attributes.hpp"

namespace osrm
{
namespace engine
{
namespace guidance
{

// Constrains lanes for multi-hop situations where lane changes depend on earlier ones.
// Instead of forcing users to change lanes rapidly in a short amount of time,
// we anticipate lane changes emitting only matching lanes early on.
// the second parameter describes the duration that we feel two segments need to be apart to count
// as separate maneuvers.
OSRM_ATTR_WARN_UNUSED
std::vector<RouteStep> anticipateLaneChange(std::vector<RouteStep> steps,
                                            const double min_distance_needed_for_lane_change = 200);

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif /* OSRM_ENGINE_GUIDANCE_LANE_PROCESSING_HPP_ */
