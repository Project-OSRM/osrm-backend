#ifndef ENGINE_GUIDANCE_POST_PROCESSING_HPP
#define ENGINE_GUIDANCE_POST_PROCESSING_HPP

#include "engine/guidance/leg_geometry.hpp"
#include "engine/guidance/route_step.hpp"
#include "engine/phantom_node.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

// passed as none-reference to modify in-place and move out again
std::vector<RouteStep> postProcess(std::vector<RouteStep> steps);

// Multiple possible reasons can result in unnecessary/confusing instructions
// A prime example would be a segregated intersection. Turning around at this
// intersection would result in two instructions to turn left.
// Collapsing such turns into a single turn instruction, we give a clearer
// set of instructionst that is not cluttered by unnecessary turns/name changes.
std::vector<RouteStep> collapseTurns(std::vector<RouteStep> steps);

// trim initial/final segment of very short length.
// This function uses in/out parameter passing to modify both steps and geometry in place.
// We use this method since both steps and geometry are closely coupled logically but
// are not coupled in the same way in the background. To avoid the additional overhead
// of introducing intermediate structions, we resolve to the in/out scheme at this point.
void trimShortSegments(std::vector<RouteStep> &steps, LegGeometry &geometry);

// assign relative locations to depart/arrive instructions
std::vector<RouteStep> assignRelativeLocations(std::vector<RouteStep> steps,
                                               const LegGeometry &geometry,
                                               const PhantomNode &source_node,
                                               const PhantomNode &target_node);

// collapse suppressed instructions remaining into intersections array
std::vector<RouteStep> buildIntersections(std::vector<RouteStep> steps);

// remove steps invalidated by post-processing
std::vector<RouteStep> removeNoTurnInstructions(std::vector<RouteStep> steps);

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
