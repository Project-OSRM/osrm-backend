#ifndef ENGINE_GUIDANCE_ASSEMBLE_ROUTE_HPP
#define ENGINE_GUIDANCE_ASSEMBLE_ROUTE_HPP

#include "engine/guidance/route.hpp"
#include "engine/guidance/route_leg.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

Route assembleRoute(const std::vector<RouteLeg> &route_legs);

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif
