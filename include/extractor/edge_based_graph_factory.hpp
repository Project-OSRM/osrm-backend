//  This class constructs the edge-expanded routing graph

#ifndef EDGE_BASED_GRAPH_FACTORY_HPP_
#define EDGE_BASED_GRAPH_FACTORY_HPP_

#include "extractor/compressed_edge_container.hpp"
#include "extractor/conditional_turn_penalty.hpp"
#include "extractor/edge_based_edge.hpp"
#include "extractor/edge_based_node_segment.hpp"
#include "extractor/extraction_turn.hpp"
#include "extractor/maneuver_override.hpp"
#include "extractor/name_table.hpp"
#include "extractor/nbg_to_ebg.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/query_node.hpp"
#include "extractor/restriction_index.hpp"
#include "extractor/turn_lane_types.hpp"
#include "extractor/way_restriction_map.hpp"

#include "util/concurrent_id_map.hpp"
#include "util/deallocating_vector.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include "storage/io.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace osrm
{
namespace extractor
{

class ScriptingEnvironment;

namespace lookup
{
#pragma pack(push, 1)
struct TurnIndexBlock
{
    NodeID from_id;
    NodeID via_id;
    NodeID to_id;
};
#pragma pack(pop)
static_assert(std::is_trivial<TurnIndexBlock>::value, "TurnIndexBlock is not trivial");
static_assert(sizeof(TurnIndexBlock) == 12, "TurnIndexBlock is not packed correctly");
} // ns lookup

struct NodeBasedGraphToEdgeBasedGraphMappingWriter; // fwd. decl

class EdgeBasedGraphFactory
{
  public:
    EdgeBasedGraphFactory(const EdgeBasedGraphFactory &) = delete;
    EdgeBasedGraphFactory &operator=(const EdgeBasedGraphFactory &) = delete;

    explicit EdgeBasedGraphFactory(const util::NodeBasedDynamicGraph &node_based_graph,
                                   EdgeBasedNodeDataContainer &node_data_container,
                                   const CompressedEdgeContainer &compressed_edge_container,
                                   const std::unordered_set<NodeID> &barrier_nodes,
                                   const std::unordered_set<NodeID> &traffic_lights,
                                   const std::vector<util::Coordinate> &coordinates,
                                   const NameTable &name_table,
                                   const std::unordered_set<EdgeID> &segregated_edges,
                                   const LaneDescriptionMap &lane_description_map);

    void Run(ScriptingEnvironment &scripting_environment,
             const std::string &turn_weight_penalties_filename,
             const std::string &turn_duration_penalties_filename,
             const std::string &turn_penalties_index_filename,
             const std::string &cnbg_ebg_mapping_path,
             const std::string &conditional_penalties_filename,
             const std::string &maneuver_overrides_filename,
             const RestrictionMap &node_restriction_map,
             const ConditionalRestrictionMap &conditional_restriction_map,
             const WayRestrictionMap &way_restriction_map,
             const std::vector<UnresolvedManeuverOverride> &maneuver_overrides);

    // The following get access functions destroy the content in the factory
    void GetEdgeBasedEdges(util::DeallocatingVector<EdgeBasedEdge> &edges);
    void GetEdgeBasedNodeSegments(std::vector<EdgeBasedNodeSegment> &nodes);
    void GetEdgeBasedNodeWeights(std::vector<EdgeWeight> &output_node_weights);
    void GetEdgeBasedNodeDurations(std::vector<EdgeWeight> &output_node_durations);
    void GetEdgeBasedNodeDistances(std::vector<EdgeDistance> &output_node_distances);
    std::uint32_t GetConnectivityChecksum() const;

    std::uint64_t GetNumberOfEdgeBasedNodes() const;

  private:
    using EdgeData = util::NodeBasedDynamicGraph::EdgeData;

    struct Conditional
    {
        // the edge based nodes allow for a unique identification of conditionals
        NodeID from_node;
        NodeID to_node;
        ConditionalTurnPenalty penalty;
    };

    // assign the correct index to the penalty value stored in the conditional
    std::vector<ConditionalTurnPenalty>
    IndexConditionals(std::vector<Conditional> &&conditionals) const;

    //! node weights that indicate the length of the segment (node based) represented by the
    //! edge-based node
    std::vector<EdgeWeight> m_edge_based_node_weights;
    std::vector<EdgeDuration> m_edge_based_node_durations;
    std::vector<EdgeDistance> m_edge_based_node_distances;

    //! list of edge based nodes (compressed segments)
    std::vector<EdgeBasedNodeSegment> m_edge_based_node_segments;
    EdgeBasedNodeDataContainer &m_edge_based_node_container;
    util::DeallocatingVector<EdgeBasedEdge> m_edge_based_edge_list;
    std::uint32_t m_connectivity_checksum;

    // The number of edge-based nodes is mostly made up out of the edges in the node-based graph.
    // Any edge in the node-based graph represents a node in the edge-based graph. In addition, we
    // add a set of artificial edge-based nodes into the mix to model via-way turn restrictions.
    // See https://github.com/Project-OSRM/osrm-backend/issues/2681#issuecomment-313080353 for
    // reference
    std::uint64_t m_number_of_edge_based_nodes;

    const std::vector<util::Coordinate> &m_coordinates;
    const util::NodeBasedDynamicGraph &m_node_based_graph;

    const std::unordered_set<NodeID> &m_barrier_nodes;
    const std::unordered_set<NodeID> &m_traffic_lights;
    const CompressedEdgeContainer &m_compressed_edge_container;

    const NameTable &name_table;
    const std::unordered_set<EdgeID> &segregated_edges;
    const LaneDescriptionMap &lane_description_map;

    // In the edge based graph, any traversable (non reversed) edge of the node-based graph forms a
    // node of the edge-based graph. To be able to name these nodes, we loop over the node-based
    // graph and create a mapping from edges (node-based) to nodes (edge-based). The mapping is
    // essentially a prefix-sum over all previous non-reversed edges of the node-based graph.
    unsigned LabelEdgeBasedNodes();

    // During the generation of the edge-expanded nodes, we need to also generate duplicates that
    // represent state during via-way restrictions (see
    // https://github.com/Project-OSRM/osrm-backend/issues/2681#issuecomment-313080353). Access to
    // the information on what to duplicate and how is provided via the way_restriction_map
    std::vector<NBGToEBG> GenerateEdgeExpandedNodes(const WayRestrictionMap &way_restriction_map);

    // Edge-expanded edges are generate for all valid turns. The validity can be checked via the
    // restriction maps
    void
    GenerateEdgeExpandedEdges(ScriptingEnvironment &scripting_environment,
                              const std::string &turn_weight_penalties_filename,
                              const std::string &turn_duration_penalties_filename,
                              const std::string &turn_penalties_index_filename,
                              const std::string &conditional_turn_penalties_filename,
                              const std::string &maneuver_overrides_filename,
                              const RestrictionMap &node_restriction_map,
                              const ConditionalRestrictionMap &conditional_restriction_map,
                              const WayRestrictionMap &way_restriction_map,
                              const std::vector<UnresolvedManeuverOverride> &maneuver_overrides);

    NBGToEBG InsertEdgeBasedNode(const NodeID u, const NodeID v);

    // mapping of node-based edges to edge-based nodes
    std::vector<NodeID> nbe_to_ebn_mapping;
};
} // namespace extractor
} // namespace osrm

#endif /* EDGE_BASED_GRAPH_FACTORY_HPP_ */
