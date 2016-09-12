#ifndef ENGINE_GUIDANCE_POST_PROCESSING_HPP
#define ENGINE_GUIDANCE_POST_PROCESSING_HPP

#include "engine/guidance/leg_geometry.hpp"
#include "engine/guidance/route_step.hpp"
#include "engine/phantom_node.hpp"
#include "util/attributes.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{
// passed as none-reference to modify in-place and move out again
OSRM_ATTR_WARN_UNUSED
std::vector<RouteStep> postProcess(std::vector<RouteStep> steps);

// Multiple possible reasons can result in unnecessary/confusing instructions
// A prime example would be a segregated intersection. Turning around at this
// intersection would result in two instructions to turn left.
// Collapsing such turns into a single turn instruction, we give a clearer
// set of instructionst that is not cluttered by unnecessary turns/name changes.
OSRM_ATTR_WARN_UNUSED
std::vector<RouteStep> collapseTurns(std::vector<RouteStep> steps);

// A check whether two instructions can be treated as one. This is only the case for very short
// maneuvers that can, in some form, be seen as one. Lookahead of one step.
bool collapsable(const RouteStep &step, const RouteStep &next);

// Elongate a step by another. the data is added either at the front, or the back
OSRM_ATTR_WARN_UNUSED
RouteStep elongate(RouteStep step, const RouteStep &by_step);

// trim initial/final segment of very short length.
// This function uses in/out parameter passing to modify both steps and geometry in place.
// We use this method since both steps and geometry are closely coupled logically but
// are not coupled in the same way in the background. To avoid the additional overhead
// of introducing intermediate structions, we resolve to the in/out scheme at this point.
void trimShortSegments(std::vector<RouteStep> &steps, LegGeometry &geometry);

// assign relative locations to depart/arrive instructions
OSRM_ATTR_WARN_UNUSED
std::vector<RouteStep> assignRelativeLocations(std::vector<RouteStep> steps,
                                               const LegGeometry &geometry,
                                               const PhantomNode &source_node,
                                               const PhantomNode &target_node);

// collapse suppressed instructions remaining into intersections array
OSRM_ATTR_WARN_UNUSED
std::vector<RouteStep> buildIntersections(std::vector<RouteStep> steps);

// remove steps invalidated by post-processing
OSRM_ATTR_WARN_UNUSED
std::vector<RouteStep> removeNoTurnInstructions(std::vector<RouteStep> steps);

// remove use lane information that is not actually a turn. For post-processing, we need to
// associate lanes with every turn. Some of these use-lane instructions are not required after lane
// anticipation anymore. This function removes all use lane instructions that are not actually used
// anymore since all lanes going straight are used anyhow.
// FIXME this is currently only a heuristic. We need knowledge on which lanes actually might become
// turn lanes. If a straight lane becomes a turn lane, this might be something to consider. Right
// now we bet on lane-anticipation to catch this.
OSRM_ATTR_WARN_UNUSED
std::vector<RouteStep> collapseUseLane(std::vector<RouteStep> steps);

// postProcess will break the connection between the leg geometry
// for which a segment is supposed to represent exactly the coordinates
// between routing maneuvers and the route steps itself.
// If required, we can get both in sync again using this function.
// Move in LegGeometry for modification in place.
OSRM_ATTR_WARN_UNUSED
LegGeometry resyncGeometry(LegGeometry leg_geometry, const std::vector<RouteStep> &steps);

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif // ENGINE_GUIDANCE_POST_PROCESSING_HPP
