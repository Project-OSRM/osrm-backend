#ifndef ENGINE_GUIDANCE_POST_PROCESSING_HPP
#define ENGINE_GUIDANCE_POST_PROCESSING_HPP

#include "engine/internal_route_result.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

std::vector<std::vector<PathData>> postProcess( std::vector<std::vector<PathData>> path_data );

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif // ENGINE_GUIDANCE_POST_PROCESSING_HPP
