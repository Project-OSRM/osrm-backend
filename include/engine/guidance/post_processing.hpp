#ifndef ENGINE_GUIDANCE_POST_PROCESSING_HPP
#define ENGINE_GUIDANCE_POST_PROCESSING_HPP

#include "engine/guidance/route_step.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

//passed as none-reference to modify in-place and move out again
std::vector<RouteStep> postProcess(std::vector<RouteStep> steps);

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif // ENGINE_GUIDANCE_POST_PROCESSING_HPP
