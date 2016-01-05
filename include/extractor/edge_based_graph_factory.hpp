//  This class constructs the edge-expanded routing graph

#ifndef EDGE_BASED_GRAPH_FACTORY_HPP_
#define EDGE_BASED_GRAPH_FACTORY_HPP_

#include "extractor/speed_profile.hpp"
#include "util/typedefs.hpp"
#include "extractor/compressed_edge_container.hpp"
#include "util/deallocating_vector.hpp"
#include "extractor/edge_based_node.hpp"
#include "extractor/original_edge_data.hpp"
#include "extractor/query_node.hpp"
#include "extractor/turn_instructions.hpp"
#include "util/node_based_graph.hpp"
#include "extractor/restriction_map.hpp"

#include <algorithm>
#include <iosfwd>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/filesystem/fstream.hpp>

struct lua_State;

namespace osrm
{
namespace extractor
{

class EdgeBasedGraphFactory
{
  public:
    EdgeBasedGraphFactory() = delete;
    EdgeBasedGraphFactory(const EdgeBasedGraphFactory &) = delete;

    explicit EdgeBasedGraphFactory(std::shared_ptr<util::NodeBasedDynamicGraph> node_based_graph,
                                   const CompressedEdgeContainer &compressed_edge_container,
                                   const std::unordered_set<NodeID> &barrier_nodes,
                                   const std::unordered_set<NodeID> &traffic_lights,
                                   std::shared_ptr<const RestrictionMap> restriction_map,
                                   const std::vector<QueryNode> &node_info_list,
                                   SpeedProfileProperties speed_profile);

#ifdef DEBUG_GEOMETRY
    void Run(const std::string &original_edge_data_filename,
             lua_State *lua_state,
             const std::string &edge_segment_lookup_filename,
             const std::string &edge_penalty_filename,
             const bool generate_edge_lookup,
             const std::string &debug_turns_path);
#else
    void Run(const std::string &original_edge_data_filename,
             lua_State *lua_state,
             const std::string &edge_segment_lookup_filename,
             const std::string &edge_penalty_filename,
             const bool generate_edge_lookup);
#endif

    void GetEdgeBasedEdges(util::DeallocatingVector<EdgeBasedEdge> &edges);

    void GetEdgeBasedNodes(std::vector<EdgeBasedNode> &nodes);
    void GetStartPointMarkers(std::vector<bool> &node_is_startpoint);

    unsigned GetHighestEdgeID();

    TurnInstruction
    AnalyzeTurn(const NodeID u, const NodeID v, const NodeID w, const double angle) const;

    int GetTurnPenalty(double angle, lua_State *lua_state) const;

  private:
    using EdgeData = util::NodeBasedDynamicGraph::EdgeData;

    //! maps index from m_edge_based_node_list to ture/false if the node is an entry point to the
    //! graph
    std::vector<bool> m_edge_based_node_is_startpoint;
    //! list of edge based nodes (compressed segments)
    std::vector<EdgeBasedNode> m_edge_based_node_list;
    util::DeallocatingVector<EdgeBasedEdge> m_edge_based_edge_list;
    unsigned m_max_edge_id;

    const std::vector<QueryNode> &m_node_info_list;
    std::shared_ptr<util::NodeBasedDynamicGraph> m_node_based_graph;
    std::shared_ptr<RestrictionMap const> m_restriction_map;

    const std::unordered_set<NodeID> &m_barrier_nodes;
    const std::unordered_set<NodeID> &m_traffic_lights;
    const CompressedEdgeContainer &m_compressed_edge_container;

    SpeedProfileProperties speed_profile;

    void CompressGeometry();
    unsigned RenumberEdges();
    void GenerateEdgeExpandedNodes();
#ifdef DEBUG_GEOMETRY
    void GenerateEdgeExpandedEdges(const std::string &original_edge_data_filename,
                                   lua_State *lua_state,
                                   const std::string &edge_segment_lookup_filename,
                                   const std::string &edge_fixed_penalties_filename,
                                   const bool generate_edge_lookup,
                                   const std::string &debug_turns_path);
#else
    void GenerateEdgeExpandedEdges(const std::string &original_edge_data_filename,
                                   lua_State *lua_state,
                                   const std::string &edge_segment_lookup_filename,
                                   const std::string &edge_fixed_penalties_filename,
                                   const bool generate_edge_lookup);
#endif

    void InsertEdgeBasedNode(const NodeID u, const NodeID v);

    void FlushVectorToStream(std::ofstream &edge_data_file,
                             std::vector<OriginalEdgeData> &original_edge_data_vector) const;
};

}
}

#endif /* EDGE_BASED_GRAPH_FACTORY_HPP_ */
