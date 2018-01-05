#ifndef OSRM_GUIDANCE_TURN_DISCOVERY_HPP_
#define OSRM_GUIDANCE_TURN_DISCOVERY_HPP_

#include "extractor/restriction_index.hpp"
#include "guidance/intersection.hpp"
#include "guidance/turn_lane_data.hpp"
#include "util/typedefs.hpp"

#include <unordered_set>

namespace osrm
{
namespace util
{
struct Coordinate;
}

namespace extractor
{
class CompressedEdgeContainer;
}

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
    const util::NodeBasedDynamicGraph &node_based_graph, // query edge data
    const extractor::EdgeBasedNodeDataContainer &node_data_container,
    const std::vector<util::Coordinate> &node_coordinates,
    const extractor::CompressedEdgeContainer &compressed_geometries,
    const extractor::RestrictionMap &node_restriction_map,
    const std::unordered_set<NodeID> &barrier_nodes,
    const extractor::TurnLanesIndexedArray &turn_lanes_data,
    // output parameters, will be in an arbitrary state on failure
    NodeID &result_node,
    EdgeID &result_via_edge,
    extractor::intersection::IntersectionView &result_intersection);

} // namespace lanes
} // namespace guidance
} // namespace osrm

#endif /*OSRM_GUIDANCE_TURN_DISCOVERY_HPP_*/
