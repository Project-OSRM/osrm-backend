#ifndef OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_GENERATOR_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_GENERATOR_HPP_

#include "extractor/compressed_edge_container.hpp"
#include "extractor/guidance/coordinate_extractor.hpp"
#include "extractor/guidance/intersection.hpp"
#include "extractor/query_node.hpp"
#include "extractor/restriction_map.hpp"
#include "util/attributes.hpp"
#include "util/name_table.hpp"
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

  private:
    const util::NodeBasedDynamicGraph &node_based_graph;
    const RestrictionMap &restriction_map;
    const std::unordered_set<NodeID> &barrier_nodes;
    const std::vector<QueryNode> &node_info_list;
    const CoordinateExtractor coordinate_extractor;

    // Check for restrictions/barriers and generate a list of valid and invalid turns present at
    // the
    // node reached
    // from `from_node` via `via_eid`
    // The resulting candidates have to be analysed for their actual instructions later on.
    OSRM_ATTR_WARN_UNUSED
    Intersection GetConnectedRoads(const NodeID from_node, const EdgeID via_eid) const;

    // check if two indices in an intersection can be seen as a single road in the perceived
    // intersection representation See below for an example. Utility function for
    // MergeSegregatedRoads
    bool CanMerge(const NodeID intersection_node,
                  const Intersection &intersection,
                  std::size_t first_index,
                  std::size_t second_index) const;

    // Merge segregated roads to omit invalid turns in favor of treating segregated roads as
    // one.
    // This function combines roads the following way:
    //
    //     *                           *
    //     *        is converted to    *
    //   v   ^                         +
    //   v   ^                         +
    //
    // The treatment results in a straight turn angle of 180º rather than a turn angle of approx
    // 160
    OSRM_ATTR_WARN_UNUSED
    Intersection MergeSegregatedRoads(const NodeID intersection_node,
                                      Intersection intersection) const;

    // The counterpiece to mergeSegregatedRoads. While we can adjust roads that split up at the
    // intersection itself, it can also happen that intersections are connected to joining roads.
    //
    //     *                           *
    //     *        is converted to    *
    //   v   a ---                     a ---
    //   v   ^                         +
    //   v   ^                         +
    //       b
    //
    // for the local view of b at a.
    OSRM_ATTR_WARN_UNUSED
    Intersection AdjustForJoiningRoads(const NodeID node_at_intersection,
                                       Intersection intersection) const;
};

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /* OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_GENERATOR_HPP_ */
