#ifndef OSRM_EXTRACTOR_INTERSECTION_INTERSECTION_ANALYSIS_HPP
#define OSRM_EXTRACTOR_INTERSECTION_INTERSECTION_ANALYSIS_HPP

#include "extractor/compressed_edge_container.hpp"
#include "extractor/guidance/turn_lane_types.hpp"
#include "extractor/intersection/intersection_edge.hpp"
#include "extractor/restriction_index.hpp"

#include "util/coordinate.hpp"
#include "util/node_based_graph.hpp"

#include <unordered_set>
#include <vector>

namespace osrm
{
namespace extractor
{
namespace intersection
{

IntersectionEdges getIncomingEdges(const util::NodeBasedDynamicGraph &graph,
                                   const NodeID intersection);

IntersectionEdges getOutgoingEdges(const util::NodeBasedDynamicGraph &graph,
                                   const NodeID intersection);

IntersectionEdgeBearings
getIntersectionBearings(const util::NodeBasedDynamicGraph &graph,
                        const extractor::CompressedEdgeContainer &compressed_geometries,
                        const std::vector<util::Coordinate> &node_coordinates,
                        const NodeID intersection);

bool isTurnAllowed(const util::NodeBasedDynamicGraph &graph,
                   const EdgeBasedNodeDataContainer &node_data_container,
                   const RestrictionMap &restriction_map,
                   const std::unordered_set<NodeID> &barrier_nodes,
                   const IntersectionEdgeBearings &bearings,
                   const guidance::TurnLanesIndexedArray &turn_lanes_data,
                   const IntersectionEdge &from,
                   const IntersectionEdge &to);

double computeTurnAngle(const IntersectionEdgeBearings &bearings,
                        const IntersectionEdge &from,
                        const IntersectionEdge &to);
}
}
}

#endif
