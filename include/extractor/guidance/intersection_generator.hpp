#ifndef OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_GENERATOR_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_GENERATOR_HPP_

#include "extractor/compressed_edge_container.hpp"
#include "extractor/guidance/coordinate_extractor.hpp"
#include "extractor/guidance/intersection.hpp"
#include "extractor/query_node.hpp"
#include "extractor/restriction_map.hpp"
#include "util/attributes.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include <unordered_set>
#include <vector>

namespace osrm
{
namespace extractor
{
namespace guidance
{
// The Intersection Generator is given a turn location and generates an intersection representation
// from it. For this all turn possibilities are analysed.
// We consider turn restrictions to indicate possible turns. U-turns are generated based on profile
// decisions.
class IntersectionGenerator
{
  public:
    IntersectionGenerator(const util::NodeBasedDynamicGraph &node_based_graph,
                          const RestrictionMap &restriction_map,
                          const std::unordered_set<NodeID> &barrier_nodes,
                          const std::vector<QueryNode> &node_info_list,
                          const CompressedEdgeContainer &compressed_edge_container);

    Intersection operator()(const NodeID nid, const EdgeID via_eid) const;

    // Graph Compression cannot compress every setting. For example any barrier/traffic light cannot
    // be compressed. As a result, a simple road of the form `a ----- b` might end up as having an
    // intermediate intersection, if there is a traffic light in between. If we want to look farther
    // down a road, finding the next actual decision requires the look at multiple intersections.
    // Here we follow the road until we either reach a dead end or find the next intersection with
    // more than a single next road.
    Intersection GetActualNextIntersection(const NodeID starting_node,
                                           const EdgeID via_edge,
                                           NodeID *resulting_from_node,
                                           EdgeID *resulting_via_edge) const;

    // Allow access to the coordinate extractor for all owners
    const CoordinateExtractor &GetCoordinateExtractor() const;

    // Check for restrictions/barriers and generate a list of valid and invalid turns present at
    // the node reached from `from_node` via `via_eid`. The resulting candidates have to be analysed
    // for their actual instructions later on.
    // The switch for `use_low_precision_angles` enables a faster mode that will procude less
    // accurate coordinates. It should be good enough to check order of turns, find striaghtmost
    // turns. Even good enough to do some simple angle verifications. It is mostly available to
    // allow for faster graph traversal in the extraction phase.
    OSRM_ATTR_WARN_UNUSED
    Intersection GetConnectedRoads(const NodeID from_node,
                                   const EdgeID via_eid,
                                   const bool use_low_precision_angles = false) const;

  private:
    const util::NodeBasedDynamicGraph &node_based_graph;
    const RestrictionMap &restriction_map;
    const std::unordered_set<NodeID> &barrier_nodes;
    const std::vector<QueryNode> &node_info_list;

    // own state, used to find the correct coordinates along a road
    const CoordinateExtractor coordinate_extractor;
};

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /* OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_GENERATOR_HPP_ */
