#ifndef OSRM_EXTRACTOR_GUIDANCE_TURN_DISCOVERY_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_TURN_DISCOVERY_HPP_

#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/intersection_generator.hpp"
#include "util/typedefs.hpp"

namespace osrm
{
namespace extractor
{
namespace guidance
{
namespace lanes
{

// OSRM processes edges by looking at a via_edge, coming into an intersection. For turn lanes, we
// might require to actually look back a turn. We do so in the hope that the turn lanes match up at
// the previous intersection for all incoming lanes.
bool findPreviousIntersection(
    const NodeID node,
    const EdgeID via_edge,
    const Intersection &intersection,
    const IntersectionGenerator &intersection_generator,
    const util::NodeBasedDynamicGraph &node_based_graph, // query edge data
    // output parameters, will be in an arbitrary state on failure
    NodeID &result_node,
    EdgeID &result_via_edge,
    IntersectionView &result_intersection);

} // namespace lanes
} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /*OSRM_EXTRACTOR_GUIDANCE_TURN_DISCOVERY_HPP_*/
