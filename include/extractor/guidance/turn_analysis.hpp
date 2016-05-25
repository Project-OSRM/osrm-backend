#ifndef OSRM_EXTRACTOR_TURN_ANALYSIS
#define OSRM_EXTRACTOR_TURN_ANALYSIS

#include "extractor/compressed_edge_container.hpp"
#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/intersection_generator.hpp"
#include "extractor/guidance/motorway_handler.hpp"
#include "extractor/guidance/roundabout_handler.hpp"
#include "extractor/guidance/toolkit.hpp"
#include "extractor/guidance/turn_classification.hpp"
#include "extractor/guidance/turn_handler.hpp"
#include "extractor/query_node.hpp"
#include "extractor/restriction_map.hpp"
#include "extractor/suffix_table.hpp"

#include "util/name_table.hpp"
#include "util/node_based_graph.hpp"

#include <cstdint>

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace osrm
{
namespace extractor
{
namespace guidance
{

class TurnAnalysis
{

  public:
    TurnAnalysis(const util::NodeBasedDynamicGraph &node_based_graph,
                 const std::vector<QueryNode> &node_info_list,
                 const RestrictionMap &restriction_map,
                 const std::unordered_set<NodeID> &barrier_nodes,
                 const CompressedEdgeContainer &compressed_edge_container,
                 const util::NameTable &name_table,
                 const SuffixTable &street_name_suffix_table);

    // the entry into the turn analysis
    std::vector<TurnOperation> getTurns(const NodeID from_node, const EdgeID via_eid) const;

    // access to the intersection representation for classification purposes
    Intersection getIntersection(const NodeID from_node, const EdgeID via_eid) const;

  private:
    const util::NodeBasedDynamicGraph &node_based_graph;
    const IntersectionGenerator intersection_generator;
    const RoundaboutHandler roundabout_handler;
    const MotorwayHandler motorway_handler;
    const TurnHandler turn_handler;

    // Utility function, setting basic turn types. Prepares for normal turn handling.
    Intersection
    setTurnTypes(const NodeID from, const EdgeID via_edge, Intersection intersection) const;
}; // class TurnAnalysis

} // namespace guidance
} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_TURN_ANALYSIS
