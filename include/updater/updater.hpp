#ifndef OSRM_UPDATER_UPDATER_HPP
#define OSRM_UPDATER_UPDATER_HPP

#include "updater/updater_config.hpp"

#include "extractor/edge_based_edge.hpp"

#include <vector>

namespace osrm
{
namespace updater
{
class Updater
{
public:
    Updater(const UpdaterConfig &config) :config(config) {}

    EdgeID LoadAndUpdateEdgeExpandedGraph(
        std::vector<extractor::EdgeBasedEdge> &edge_based_edge_list,
        std::vector<EdgeWeight> &node_weights) const;

private:
    UpdaterConfig config;
};
}
}

#endif
