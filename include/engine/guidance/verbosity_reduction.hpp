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

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif /* OSRM_ENGINE_GUIDANCE_VERBOSITY_REDUCTION_HPP_ */
