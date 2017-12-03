#ifndef OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_GENERATOR_HPP_
#define OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_GENERATOR_HPP_

#include "extractor/compressed_edge_container.hpp"
#include "extractor/guidance/coordinate_extractor.hpp"
#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/intersection_normalization_operation.hpp"
#include "extractor/query_node.hpp"
#include "extractor/restriction_index.hpp"
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
                          const EdgeBasedNodeDataContainer &node_data_container,
                          const RestrictionMap &restriction_map,
                          const std::unordered_set<NodeID> &barrier_nodes,
                          const std::vector<util::Coordinate> &coordinates,
                          const CompressedEdgeContainer &compressed_edge_container);

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

  private:
    const util::NodeBasedDynamicGraph &node_based_graph;
    const EdgeBasedNodeDataContainer &node_data_container;
    const RestrictionMap &restriction_map;
    const std::unordered_set<NodeID> &barrier_nodes;
    const std::vector<util::Coordinate> &coordinates;
};

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif /* OSRM_EXTRACTOR_GUIDANCE_INTERSECTION_GENERATOR_HPP_ */
