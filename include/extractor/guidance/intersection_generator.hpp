#ifndef OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_GENERATOR_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_GENERATOR_HPP_

#include "extractor/compressed_edge_container.hpp"
#include "extractor/guidance/coordinate_extractor.hpp"
#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/intersection_normalization_operation.hpp"
#include "extractor/query_node.hpp"
#include "extractor/restriction_map.hpp"
#include "util/attributes.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

namespace osrm
{
namespace extractor
{
namespace guidance
{

struct IntersectionGenerationParameters
{
    NodeID nid;
    EdgeID via_eid;
};

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

    // For a source node `a` and a via edge `ab` creates an intersection at target `b`.
    //
    // a . . . b . .
    //         .
    //         .
    //
    IntersectionView operator()(const NodeID nid, const EdgeID via_eid) const;

    /*
     * Compute the shape of an intersection, returning a set of connected roads, without any further
     * concern for which of the entries are actually allowed.
     * The shape also only comes with turn bearings, not with turn angles. All turn angles will be
     * set to zero
     */
    OSRM_ATTR_WARN_UNUSED
    IntersectionShape
    ComputeIntersectionShape(const NodeID center_node,
                             const boost::optional<NodeID> sorting_base = boost::none,
                             bool use_low_precision_angles = false) const;

    // Graph Compression cannot compress every setting. For example any barrier/traffic light cannot
    // be compressed. As a result, a simple road of the form `a ----- b` might end up as having an
    // intermediate intersection, if there is a traffic light in between. If we want to look farther
    // down a road, finding the next actual decision requires the look at multiple intersections.
    // Here we follow the road until we either reach a dead end or find the next intersection with
    // more than a single next road. This function skips over degree two nodes to find coorect input
    // for GetConnectedRoads.
    OSRM_ATTR_WARN_UNUSED
    IntersectionGenerationParameters SkipDegreeTwoNodes(const NodeID starting_node,
                                                        const EdgeID via_edge) const;

    // Allow access to the coordinate extractor for all owners
    const CoordinateExtractor &GetCoordinateExtractor() const;

    // Check for restrictions/barriers and generate a list of valid and invalid turns present at
    // the node reached from `from_node` via `via_eid`. The resulting candidates have to be analysed
    // for their actual instructions later on.
    // The switch for `use_low_precision_angles` enables a faster mode that will procude less
    // accurate coordinates. It should be good enough to check order of turns, find straightmost
    // turns. Even good enough to do some simple angle verifications. It is mostly available to
    // allow for faster graph traversal in the extraction phase.
    OSRM_ATTR_WARN_UNUSED
    IntersectionView GetConnectedRoads(const NodeID from_node,
                                       const EdgeID via_eid,
                                       const bool use_low_precision_angles = false) const;

    /*
     * To be used in the road network, we need to check for valid/restricted turns. These two
     * functions transform a basic intersection / a normalised intersection into the
     * correct view when entering via a given edge.
     */
    OSRM_ATTR_WARN_UNUSED
    IntersectionView
    TransformIntersectionShapeIntoView(const NodeID previous_node,
                                       const EdgeID entering_via_edge,
                                       const IntersectionShape &intersection) const;
    // version for normalised intersection
    OSRM_ATTR_WARN_UNUSED
    IntersectionView TransformIntersectionShapeIntoView(
        const NodeID previous_node,
        const EdgeID entering_via_edge,
        const IntersectionShape &normalised_intersection,
        const IntersectionShape &intersection,
        const std::vector<IntersectionNormalizationOperation> &merging_map) const;

  private:
    const util::NodeBasedDynamicGraph &node_based_graph;
    const RestrictionMap &restriction_map;
    const std::unordered_set<NodeID> &barrier_nodes;
    const std::vector<QueryNode> &node_info_list;

    // own state, used to find the correct coordinates along a road
    const CoordinateExtractor coordinate_extractor;

    // check turn restrictions to find a node that is the only allowed target when coming from a
    // node to an intersection
    //     d
    //     |
    // a - b - c  and `only_straight_on ab | bc would return `c` for `a,b`
    boost::optional<NodeID> GetOnlyAllowedTurnIfExistent(const NodeID coming_from_node,
                                                         const NodeID node_at_intersection) const;
};

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /* OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_GENERATOR_HPP_ */
