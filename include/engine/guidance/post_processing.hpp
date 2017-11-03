#ifndef ENGINE_GUIDANCE_POST_PROCESSING_HPP
#define ENGINE_GUIDANCE_POST_PROCESSING_HPP

#include "engine/datafacade/datafacade_base.hpp"
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
std::vector<RouteStep> handleRoundabouts(std::vector<RouteStep> steps);

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

// postProcess will break the connection between the leg geometry
// for which a segment is supposed to represent exactly the coordinates
// between routing maneuvers and the route steps itself.
// If required, we can get both in sync again using this function.
// Move in LegGeometry for modification in place.
OSRM_ATTR_WARN_UNUSED
LegGeometry resyncGeometry(LegGeometry leg_geometry, const std::vector<RouteStep> &steps);

/**
 * Apply maneuver override relations to the selected route.
 * Should be called before any other post-processing is performed
 * to ensure that all sequences of edge-based-nodes are still in the
 * steps list.
 *
 * @param steps the steps of the route
 */
void applyOverrides(const datafacade::BaseDataFacade &facade,
                    std::vector<RouteStep> &steps,
                    const LegGeometry &geometry);

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif // ENGINE_GUIDANCE_POST_PROCESSING_HPP
