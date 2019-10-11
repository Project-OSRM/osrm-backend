#ifndef OSRM_UPDATER_UPDATER_HPP
#define OSRM_UPDATER_UPDATER_HPP

#include "updater/updater_config.hpp"
#include "extractor/edge_based_edge.hpp"
#include "tbb/concurrent_unordered_set.h"

#include <chrono>
#include <vector>

namespace osrm
{
namespace updater
{
// https://www.threadingbuildingblocks.org/docs/help/reference/containers_overview/concurrent_unordered_set_cls.html
using NodeSet = tbb::concurrent_unordered_set<NodeID>;
using NodeSetPtr = std::shared_ptr<NodeSet>;
using NodeSetViewerPtr = std::shared_ptr<const NodeSet>;

class Updater
{
  public:
    Updater(UpdaterConfig config_) : config(std::move(config_)) {}

    EdgeID
    LoadAndUpdateEdgeExpandedGraph(std::vector<extractor::EdgeBasedEdge> &edge_based_edge_list,
                                   std::vector<EdgeWeight> &node_weights,
                                   std::uint32_t &connectivity_checksum,
                                   NodeSetPtr node_updated) const;

    EdgeID LoadAndUpdateEdgeExpandedGraph(
        std::vector<extractor::EdgeBasedEdge> &edge_based_edge_list,
        std::vector<EdgeWeight> &node_weights,
        std::vector<EdgeDuration> &node_durations, // TODO: remove when optional
        std::uint32_t &connectivity_checksum,
        NodeSetPtr node_updated) const;
    EdgeID LoadAndUpdateEdgeExpandedGraph(
        std::vector<extractor::EdgeBasedEdge> &edge_based_edge_list,
        std::vector<EdgeWeight> &node_weights,
        std::vector<EdgeDuration> &node_durations, // TODO: remove when optional
        std::vector<EdgeDistance> &node_distances, // TODO: remove when optional
        std::uint32_t &connectivity_checksum,
        NodeSetPtr node_updated) const;

  private:
    UpdaterConfig config;
};
}
}

#endif
