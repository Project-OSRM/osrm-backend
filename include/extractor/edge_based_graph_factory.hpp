//  This class constructs the edge-expanded routing graph

#ifndef EDGE_BASED_GRAPH_FACTORY_HPP_
#define EDGE_BASED_GRAPH_FACTORY_HPP_

#include "extractor/compressed_edge_container.hpp"
#include "extractor/edge_based_edge.hpp"
#include "extractor/edge_based_node_segment.hpp"
#include "extractor/extraction_turn.hpp"
#include "extractor/guidance/turn_analysis.hpp"
#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/guidance/turn_lane_types.hpp"
#include "extractor/nbg_to_ebg.hpp"
#include "extractor/node_data_container.hpp"
#include "extractor/original_edge_data.hpp"
#include "extractor/packed_osm_ids.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/query_node.hpp"
#include "extractor/restriction_map.hpp"
#include "extractor/way_restriction_map.hpp"

#include "util/concurrent_id_map.hpp"
#include "util/deallocating_vector.hpp"
#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"
#include "util/name_table.hpp"
#include "util/node_based_graph.hpp"
#include "util/packed_vector.hpp"
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

#include <boost/filesystem/fstream.hpp>

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>

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

    explicit EdgeBasedGraphFactory(std::shared_ptr<util::NodeBasedDynamicGraph> node_based_graph,
                                   CompressedEdgeContainer &compressed_edge_container,
                                   const std::unordered_set<NodeID> &barrier_nodes,
                                   const std::unordered_set<NodeID> &traffic_lights,
                                   const std::vector<util::Coordinate> &coordinates,
                                   const extractor::PackedOSMIDs &osm_node_ids,
                                   ProfileProperties profile_properties,
                                   const util::NameTable &name_table,
                                   guidance::LaneDescriptionMap &lane_description_map);

    void Run(ScriptingEnvironment &scripting_environment,
             const std::string &turn_data_filename,
             const std::string &turn_lane_data_filename,
             const std::string &turn_weight_penalties_filename,
             const std::string &turn_duration_penalties_filename,
             const std::string &turn_penalties_index_filename,
             const std::string &cnbg_ebg_mapping_path,
             const RestrictionMap &restriction_map,
             const WayRestrictionMap &way_restriction_map);

    // The following get access functions destroy the content in the factory
    void GetEdgeBasedEdges(util::DeallocatingVector<EdgeBasedEdge> &edges);
    void GetEdgeBasedNodes(EdgeBasedNodeDataContainer &data_container);
    void GetEdgeBasedNodeSegments(std::vector<EdgeBasedNodeSegment> &nodes);
    void GetStartPointMarkers(std::vector<bool> &node_is_startpoint);
    void GetEdgeBasedNodeWeights(std::vector<EdgeWeight> &output_node_weights);

    // These access functions don't destroy the content
    const std::vector<BearingClassID> &GetBearingClassIds() const;
    std::vector<BearingClassID> &GetBearingClassIds();
    std::vector<util::guidance::BearingClass> GetBearingClasses() const;
    std::vector<util::guidance::EntryClass> GetEntryClasses() const;

    std::uint64_t GetNumberOfEdgeBasedNodes() const;

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
    std::vector<EdgeBasedNodeSegment> m_edge_based_node_segments;
    EdgeBasedNodeDataContainer m_edge_based_node_container;
    util::DeallocatingVector<EdgeBasedEdge> m_edge_based_edge_list;

    // the number of edge-based nodes is mostly made up out of the edges in the node-based graph.
    // Any edge in the node-based graph represents a node in the edge-based graph. In addition, we
    // add a set of artificial edge-based nodes into the mix to model via-way turn restrictions.
    std::uint64_t m_number_of_edge_based_nodes;

    const std::vector<util::Coordinate> &m_coordinates;
    const extractor::PackedOSMIDs &m_osm_node_ids;
    std::shared_ptr<util::NodeBasedDynamicGraph> m_node_based_graph;

    const std::unordered_set<NodeID> &m_barrier_nodes;
    const std::unordered_set<NodeID> &m_traffic_lights;
    CompressedEdgeContainer &m_compressed_edge_container;

    ProfileProperties profile_properties;

    const util::NameTable &name_table;
    guidance::LaneDescriptionMap &lane_description_map;

    unsigned RenumberEdges();

    std::vector<NBGToEBG> GenerateEdgeExpandedNodes(const WayRestrictionMap &way_restriction_map);

    void GenerateEdgeExpandedEdges(ScriptingEnvironment &scripting_environment,
                                   const std::string &original_edge_data_filename,
                                   const std::string &turn_lane_data_filename,
                                   const std::string &turn_weight_penalties_filename,
                                   const std::string &turn_duration_penalties_filename,
                                   const std::string &turn_penalties_index_filename,
                                   const RestrictionMap &restriction_map,
                                   const WayRestrictionMap &way_restriction_map);

    NBGToEBG InsertEdgeBasedNode(const NodeID u, const NodeID v);

    std::size_t restricted_turns_counter;
    std::size_t skipped_uturns_counter;
    std::size_t skipped_barrier_turns_counter;

    util::ConcurrentIDMap<util::guidance::BearingClass, BearingClassID> bearing_class_hash;
    std::vector<BearingClassID> bearing_class_by_node_based_node;
    util::ConcurrentIDMap<util::guidance::EntryClass, EntryClassID> entry_class_hash;
};
} // namespace extractor
} // namespace osrm

#endif /* EDGE_BASED_GRAPH_FACTORY_HPP_ */
