#ifndef OSRM_UPDATER_UPDATER_HPP
#define OSRM_UPDATER_UPDATER_HPP

#include "updater/updater_config.hpp"

#include "extractor/edge_based_edge.hpp"
#include "extractor/query_node.hpp"

#include <vector>

namespace osrm
{
namespace updater
{
class Updater
{
  public:
    Updater(UpdaterConfig config_) : config(std::move(config_)) {}

    using NumNodesAndEdges = std::tuple<EdgeID, std::vector<extractor::EdgeBasedEdge>>;
    NumNodesAndEdges LoadAndUpdateEdgeExpandedGraph() const;

    EdgeID LoadAndUpdateEdgeExpandedGraph(
        std::vector<extractor::EdgeBasedEdge> &edge_based_edge_list,
        std::vector<EdgeWeight> &node_weights,
        std::vector<extractor::QueryNode> internal_to_external_node_map) const;

  private:
    UpdaterConfig config;
};
}
}

#endif
