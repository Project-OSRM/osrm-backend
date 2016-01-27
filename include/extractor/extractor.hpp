#ifndef EXTRACTOR_HPP
#define EXTRACTOR_HPP

#include "extractor/edge_based_edge.hpp"
#include "extractor/extractor_config.hpp"
#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/graph_compressor.hpp"

#include "util/typedefs.hpp"

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace osrm
{
namespace extractor
{

class Extractor
{
  public:
    Extractor(ExtractorConfig extractor_config) : config(std::move(extractor_config)) {}
    int run();

  private:
    ExtractorConfig config;

    void SetupScriptingEnvironment(lua_State *myLuaState, SpeedProfileProperties &speed_profile);
    std::pair<std::size_t, std::size_t>
    BuildEdgeExpandedGraph(std::vector<QueryNode> &internal_to_external_node_map,
                           std::vector<EdgeBasedNode> &node_based_edge_list,
                           std::vector<bool> &node_is_startpoint,
                           std::vector<EdgeWeight> &edge_based_node_weights,
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
    LoadNodeBasedGraph(std::unordered_map<NodeID,bool> &barrier_nodes,
                       std::unordered_set<NodeID> &traffic_lights,
                       std::vector<QueryNode> &internal_to_external_node_map);

    void WriteEdgeBasedGraph(const std::string &output_file_filename,
                             const size_t max_edge_id,
                             util::DeallocatingVector<EdgeBasedEdge> const &edge_based_edge_list);
};
}
}

#endif /* EXTRACTOR_HPP */
