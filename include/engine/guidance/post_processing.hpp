#ifndef ENGINE_GUIDANCE_POST_PROCESSING_HPP
#define ENGINE_GUIDANCE_POST_PROCESSING_HPP

#include "engine/guidance/route_step.hpp"
#include "engine/guidance/leg_geometry.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

// passed as none-reference to modify in-place and move out again
std::vector<RouteStep> postProcess(std::vector<RouteStep> steps);

// postProcess will break the connection between the leg geometry
// for which a segment is supposed to represent exactly the coordinates
// between routing maneuvers and the route steps itself.
// If required, we can get both in sync again using this function.
// Move in LegGeometry for modification in place.
LegGeometry resyncGeometry(LegGeometry leg_geometry, const std::vector<RouteStep> &steps);

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif // ENGINE_GUIDANCE_POST_PROCESSING_HPP
