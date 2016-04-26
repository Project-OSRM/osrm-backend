#ifndef OSRM_GUIDANCE_TURN_CLASSIFICATION_HPP_
#define OSRM_GUIDANCE_TURN_CLASSIFICATION_HPP_

#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/toolkit.hpp"
#include "extractor/compressed_edge_container.hpp"
#include "extractor/query_node.hpp"

#include "util/guidance/entry_class.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/coordinate.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"



#include <algorithm>
#include <cstddef>
#include <vector>
#include <utility>

namespace osrm
{
namespace extractor
{
namespace guidance
{

std::pair<util::guidance::EntryClass,util::guidance::BearingClass>
classifyIntersection(NodeID nid,
                     const Intersection &intersection,
                     const util::NodeBasedDynamicGraph &node_based_graph,
                     const extractor::CompressedEdgeContainer &compressed_geometries,
                     const std::vector<extractor::QueryNode> &query_nodes);

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif // OSRM_GUIDANCE_TURN_CLASSIFICATION_HPP_
