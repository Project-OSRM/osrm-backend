//  This class constructs the edge-expanded routing graph

#ifndef EDGE_BASED_GRAPH_FACTORY_HPP_
#define EDGE_BASED_GRAPH_FACTORY_HPP_

#include "extractor/compressed_edge_container.hpp"
#include "extractor/edge_based_edge.hpp"
#include "extractor/edge_based_node.hpp"
#include "extractor/extraction_turn.hpp"
#include "extractor/original_edge_data.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/query_node.hpp"
#include "extractor/restriction_map.hpp"

#include "extractor/guidance/turn_analysis.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/guidance/turn_lane_types.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"

#include "util/deallocating_vector.hpp"
#include "util/name_table.hpp"
#include "util/node_based_graph.hpp"
#include "util/typedefs.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <queue>
#include <string>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/filesystem/fstream.hpp>

namespace osrm
{
namespace extractor
{

class EdgeBasedGraphFactory
{
  public:
    EdgeBasedGraphFactory(const EdgeBasedGraphFactory &) = delete;
    EdgeBasedGraphFactory &operator=(const EdgeBasedGraphFactory &) = delete;

    explicit EdgeBasedGraphFactory(std::shared_ptr<util::NodeBasedDynamicGraph> node_based_graph,
                                   CompressedEdgeContainer &compressed_edge_container,
                                   const std::unordered_set<NodeID> &barrier_nodes,
                                   const std::unordered_set<NodeID> &traffic_lights,
                                   std::shared_ptr<const RestrictionMap> restriction_map,
                                   const std::vector<QueryNode> &node_info_list,
                                   ProfileProperties profile_properties,
                                   const util::NameTable &name_table,
                                   std::vector<std::uint32_t> &turn_lane_offsets,
                                   std::vector<guidance::TurnLaneType::Mask> &turn_lane_masks,
                                   guidance::LaneDescriptionMap &lane_description_map);

    void Run(ScriptingEnvironment &scripting_environment,
             const std::string &original_edge_data_filename,
             const std::string &turn_lane_data_filename,
             const std::string &edge_segment_lookup_filename,
             const std::string &turn_weight_penalties_filename,
             const std::string &turn_duration_penalties_filename,
             const std::string &turn_penalties_index_filename,
             const bool generate_edge_lookup);

    // The following get access functions destroy the content in the factory
    void GetEdgeBasedEdges(util::DeallocatingVector<EdgeBasedEdge> &edges);
    void GetEdgeBasedNodes(std::vector<EdgeBasedNode> &nodes);
    void GetStartPointMarkers(std::vector<bool> &node_is_startpoint);
    void GetEdgeBasedNodeWeights(std::vector<EdgeWeight> &output_node_weights);

    // These access functions don't destroy the content
    const std::vector<BearingClassID> &GetBearingClassIds() const;
    std::vector<BearingClassID> &GetBearingClassIds();
    std::vector<util::guidance::BearingClass> GetBearingClasses() const;
    std::vector<util::guidance::EntryClass> GetEntryClasses() const;

    unsigned GetHighestEdgeID();

    // Basic analysis of a turn (u --(e1)-- v --(e2)-- w)
    // with known angle.
    // Handles special cases like u-turns and roundabouts
    // For basic turns, the turn based on the angle-classification is returned
    guidance::TurnInstruction AnalyzeTurn(const NodeID u,
                                          const EdgeID e1,
                                          const NodeID v,
                                          const EdgeID e2,
                                          const NodeID w,
                                          const double angle) const;

  private:
    using EdgeData = util::NodeBasedDynamicGraph::EdgeData;

    //! maps index from m_edge_based_node_list to ture/false if the node is an entry point to the
    //! graph
    std::vector<bool> m_edge_based_node_is_startpoint;

    //! node weights that indicate the length of the segment (node based) represented by the
    //! edge-based node
    std::vector<EdgeWeight> m_edge_based_node_weights;

    //! list of edge based nodes (compressed segments)
    std::vector<EdgeBasedNode> m_edge_based_node_list;
    util::DeallocatingVector<EdgeBasedEdge> m_edge_based_edge_list;
    EdgeID m_max_edge_id;

    const std::vector<QueryNode> &m_node_info_list;
    std::shared_ptr<util::NodeBasedDynamicGraph> m_node_based_graph;
    std::shared_ptr<RestrictionMap const> m_restriction_map;

    const std::unordered_set<NodeID> &m_barrier_nodes;
    const std::unordered_set<NodeID> &m_traffic_lights;
    CompressedEdgeContainer &m_compressed_edge_container;

    ProfileProperties profile_properties;
    bool fallback_to_duration;

    const util::NameTable &name_table;
    std::vector<std::uint32_t> &turn_lane_offsets;
    std::vector<guidance::TurnLaneType::Mask> &turn_lane_masks;
    guidance::LaneDescriptionMap &lane_description_map;

    unsigned RenumberEdges();
    void GenerateEdgeExpandedNodes();
    void GenerateEdgeExpandedEdges(ScriptingEnvironment &scripting_environment,
                                   const std::string &original_edge_data_filename,
                                   const std::string &turn_lane_data_filename,
                                   const std::string &edge_segment_lookup_filename,
                                   const std::string &turn_weight_penalties_filename,
                                   const std::string &turn_duration_penalties_filename,
                                   const std::string &turn_penalties_index_filename,
                                   const bool generate_edge_lookup);

    void InsertEdgeBasedNode(const NodeID u, const NodeID v);

    void FlushVectorToStream(std::ofstream &edge_data_file,
                             std::vector<OriginalEdgeData> &original_edge_data_vector) const;

    std::unordered_map<util::guidance::BearingClass, BearingClassID> bearing_class_hash;
    std::vector<BearingClassID> bearing_class_by_node_based_node;
    std::unordered_map<util::guidance::EntryClass, EntryClassID> entry_class_hash;
};
} // namespace extractor
} // namespace osrm

#endif /* EDGE_BASED_GRAPH_FACTORY_HPP_ */
