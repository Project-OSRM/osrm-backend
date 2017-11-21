#ifndef OSRM_EXTRACTOR_TURN_ANALYSIS
#define OSRM_EXTRACTOR_TURN_ANALYSIS

#include "extractor/compressed_edge_container.hpp"
#include "extractor/guidance/driveway_handler.hpp"
#include "extractor/guidance/intersection.hpp"
#include "extractor/guidance/intersection_generator.hpp"
#include "extractor/guidance/intersection_normalization_operation.hpp"
#include "extractor/guidance/motorway_handler.hpp"
#include "extractor/guidance/roundabout_handler.hpp"
#include "extractor/guidance/sliproad_handler.hpp"
#include "extractor/guidance/statistics_handler.hpp"
#include "extractor/guidance/suppress_mode_handler.hpp"
#include "extractor/guidance/turn_classification.hpp"
#include "extractor/guidance/turn_handler.hpp"
#include "extractor/query_node.hpp"
#include "extractor/restriction_index.hpp"
#include "extractor/suffix_table.hpp"

#include "util/attributes.hpp"
#include "util/name_table.hpp"
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
namespace extractor
{
namespace guidance
{

class TurnAnalysis
{
  public:
    TurnAnalysis(const util::NodeBasedDynamicGraph &node_based_graph,
                 const EdgeBasedNodeDataContainer &node_data_container,
                 const std::vector<util::Coordinate> &coordinates,
                 const RestrictionMap &restriction_map,
                 const std::unordered_set<NodeID> &barrier_nodes,
                 const CompressedEdgeContainer &compressed_edge_container,
                 const util::NameTable &name_table,
                 const SuffixTable &street_name_suffix_table);

    /* Full Analysis Process for a single node/edge combination. Use with caution, as the process is
     * relatively expensive */
    OSRM_ATTR_WARN_UNUSED
    Intersection operator()(const NodeID node_prior_to_intersection,
                            const EdgeID entering_via_edge) const;

    // Select turn types based on the intersection shape
    OSRM_ATTR_WARN_UNUSED
    Intersection AssignTurnTypes(const NodeID from_node,
                                 const EdgeID via_eid,
                                 const IntersectionView &intersection) const;

    const IntersectionGenerator &GetIntersectionGenerator() const;

  private:
    const util::NodeBasedDynamicGraph &node_based_graph;
    const IntersectionGenerator intersection_generator;
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
} // namespace extractor
} // namespace osrm

#endif // OSRM_EXTRACTOR_TURN_ANALYSIS
