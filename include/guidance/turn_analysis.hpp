#ifndef OSRM_GUIDANCE_TURN_ANALYSIS
#define OSRM_GUIDANCE_TURN_ANALYSIS

#include "extractor/compressed_edge_container.hpp"
#include "extractor/intersection/intersection_view.hpp"
#include "extractor/name_table.hpp"
#include "extractor/restriction_index.hpp"
#include "extractor/suffix_table.hpp"

#include "guidance/driveway_handler.hpp"
#include "guidance/intersection.hpp"
#include "guidance/motorway_handler.hpp"
#include "guidance/roundabout_handler.hpp"
#include "guidance/sliproad_handler.hpp"
#include "guidance/statistics_handler.hpp"
#include "guidance/suppress_mode_handler.hpp"
#include "guidance/turn_classification.hpp"
#include "guidance/turn_handler.hpp"

#include "util/attributes.hpp"
#include "util/node_based_graph.hpp"

#include <cstdint>

#include <memory>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace osrm
{
namespace guidance
{

class TurnAnalysis
{
  public:
    TurnAnalysis(const util::NodeBasedDynamicGraph &node_based_graph,
                 const extractor::EdgeBasedNodeDataContainer &node_data_container,
                 const std::vector<util::Coordinate> &node_coordinates,
                 const extractor::CompressedEdgeContainer &compressed_edge_container,
                 const extractor::RestrictionMap &restriction_map,
                 const std::unordered_set<NodeID> &barrier_nodes,
                 const extractor::TurnLanesIndexedArray &turn_lanes_data,
                 const extractor::NameTable &name_table,
                 const extractor::SuffixTable &street_name_suffix_table);

    /* Full Analysis Process for a single node/edge combination. Use with caution, as the process is
     * relatively expensive */
    OSRM_ATTR_WARN_UNUSED
    Intersection operator()(const NodeID node_prior_to_intersection,
                            const EdgeID entering_via_edge) const;

    // Select turn types based on the intersection shape
    OSRM_ATTR_WARN_UNUSED
    Intersection
    AssignTurnTypes(const NodeID from_node,
                    const EdgeID via_eid,
                    const extractor::intersection::IntersectionView &intersection) const;

  private:
    const util::NodeBasedDynamicGraph &node_based_graph;
    const RoundaboutHandler roundabout_handler;
    const MotorwayHandler motorway_handler;
    const TurnHandler turn_handler;
    const SliproadHandler sliproad_handler;
    const SuppressModeHandler suppress_mode_handler;
    const DrivewayHandler driveway_handler;
    const StatisticsHandler statistics_handler;

    // Utility function, setting basic turn types. Prepares for normal turn handling.
    Intersection
    setTurnTypes(const NodeID from, const EdgeID via_edge, Intersection intersection) const;
}; // class TurnAnalysis

} // namespace guidance
} // namespace osrm

#endif // OSRM_GUIDANCE_TURN_ANALYSIS
