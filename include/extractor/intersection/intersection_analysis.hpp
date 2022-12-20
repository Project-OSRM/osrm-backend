#ifndef OSRM_EXTRACTOR_INTERSECTION_INTERSECTION_ANALYSIS_HPP
#define OSRM_EXTRACTOR_INTERSECTION_INTERSECTION_ANALYSIS_HPP

#include "extractor/compressed_edge_container.hpp"
#include "extractor/intersection/intersection_edge.hpp"
#include "extractor/intersection/intersection_view.hpp"
#include "extractor/intersection/mergable_road_detector.hpp"
#include "extractor/node_restriction_map.hpp"
#include "extractor/turn_lane_types.hpp"

#include "util/coordinate.hpp"
#include "util/node_based_graph.hpp"

#include <unordered_set>
#include <vector>

namespace osrm::extractor::intersection
{

IntersectionEdges getIncomingEdges(const util::NodeBasedDynamicGraph &graph,
                                   const NodeID intersection);

IntersectionEdges getOutgoingEdges(const util::NodeBasedDynamicGraph &graph,
                                   const NodeID intersection);

bool isTurnAllowed(const util::NodeBasedDynamicGraph &graph,
                   const EdgeBasedNodeDataContainer &node_data_container,
                   const RestrictionMap &restriction_map,
                   const std::unordered_set<NodeID> &barrier_nodes,
                   const IntersectionEdgeGeometries &geometries,
                   const TurnLanesIndexedArray &turn_lanes_data,
                   const IntersectionEdge &from,
                   const IntersectionEdge &to);

double findEdgeBearing(const IntersectionEdgeGeometries &geometries, const EdgeID &edge);

double findEdgeLength(const IntersectionEdgeGeometries &geometries, const EdgeID &edge);

std::pair<IntersectionEdgeGeometries, std::unordered_set<EdgeID>>
getIntersectionGeometries(const util::NodeBasedDynamicGraph &graph,
                          const extractor::CompressedEdgeContainer &compressed_geometries,
                          const std::vector<util::Coordinate> &node_coordinates,
                          const MergableRoadDetector &detector,
                          const NodeID intersection);

IntersectionView convertToIntersectionView(const util::NodeBasedDynamicGraph &graph,
                                           const EdgeBasedNodeDataContainer &node_data_container,
                                           const RestrictionMap &restriction_map,
                                           const std::unordered_set<NodeID> &barrier_nodes,
                                           const IntersectionEdgeGeometries &edge_geometries,
                                           const TurnLanesIndexedArray &turn_lanes_data,
                                           const IntersectionEdge &incoming_edge,
                                           const IntersectionEdges &outgoing_edges,
                                           const std::unordered_set<EdgeID> &merged_edges);

// Check for restrictions/barriers and generate a list of valid and invalid turns present at
// the node reached from `incoming_edge`. The resulting candidates have to be analyzed
// for their actual instructions later on.
template <bool USE_CLOSE_COORDINATE>
IntersectionView getConnectedRoads(const util::NodeBasedDynamicGraph &graph,
                                   const EdgeBasedNodeDataContainer &node_data_container,
                                   const std::vector<util::Coordinate> &node_coordinates,
                                   const extractor::CompressedEdgeContainer &compressed_geometries,
                                   const RestrictionMap &node_restriction_map,
                                   const std::unordered_set<NodeID> &barrier_nodes,
                                   const TurnLanesIndexedArray &turn_lanes_data,
                                   const IntersectionEdge &incoming_edge);

IntersectionView
getConnectedRoadsForEdgeGeometries(const util::NodeBasedDynamicGraph &graph,
                                   const EdgeBasedNodeDataContainer &node_data_container,
                                   const RestrictionMap &node_restriction_map,
                                   const std::unordered_set<NodeID> &barrier_nodes,
                                   const TurnLanesIndexedArray &turn_lanes_data,
                                   const IntersectionEdge &incoming_edge,
                                   const IntersectionEdgeGeometries &edge_geometries,
                                   const std::unordered_set<EdgeID> &merged_edge_ids);

// Graph Compression cannot compress every setting. For example any barrier/traffic light cannot
// be compressed. As a result, a simple road of the form `a ----- b` might end up as having an
// intermediate intersection, if there is a traffic light in between. If we want to look farther
// down a road, finding the next actual decision requires the look at multiple intersections.
// Here we follow the road until we either reach a dead end or find the next intersection with
// more than a single next road. This function skips over degree two nodes to find correct input
// for getConnectedRoads.
IntersectionEdge skipDegreeTwoNodes(const util::NodeBasedDynamicGraph &graph,
                                    IntersectionEdge road);
} // namespace osrm::extractor::intersection

#endif
