#ifndef EXTRACTOR_HPP
#define EXTRACTOR_HPP

#include "extractor/edge_based_edge.hpp"
#include "extractor/extractor_options.hpp"
#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/graph_compressor.hpp"

namespace osrm
{
namespace extractor
{

class extractor
{
  public:
    extractor(ExtractorConfig extractor_config) : config(std::move(extractor_config)) {}
    int run();

  private:
    ExtractorConfig config;
    void SetupScriptingEnvironment(lua_State *myLuaState, SpeedProfileProperties &speed_profile);
    std::pair<std::size_t, std::size_t>
    BuildEdgeExpandedGraph(std::vector<QueryNode> &internal_to_external_node_map,
                           std::vector<EdgeBasedNode> &node_based_edge_list,
                           std::vector<bool> &node_is_startpoint,
                           util::DeallocatingVector<EdgeBasedEdge> &edge_based_edge_list);
    void WriteNodeMapping(const std::vector<QueryNode> &internal_to_external_node_map);
    void FindComponents(unsigned max_edge_id,
                        const util::DeallocatingVector<EdgeBasedEdge> &edges,
                        std::vector<EdgeBasedNode> &nodes) const;
    void BuildRTree(std::vector<EdgeBasedNode> node_based_edge_list,
                    std::vector<bool> node_is_startpoint,
                    const std::vector<QueryNode> &internal_to_external_node_map);
    std::shared_ptr<RestrictionMap> LoadRestrictionMap();
    std::shared_ptr<util::NodeBasedDynamicGraph>
    LoadNodeBasedGraph(std::unordered_set<NodeID> &barrier_nodes,
                       std::unordered_set<NodeID> &traffic_lights,
                       std::vector<QueryNode> &internal_to_external_node_map);

    void WriteEdgeBasedGraph(std::string const &output_file_filename,
                             size_t const max_edge_id,
                             util::DeallocatingVector<EdgeBasedEdge> const &edge_based_edge_list);
};
}
}

#endif /* EXTRACTOR_HPP */
