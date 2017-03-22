#ifndef OSRM_UPDATER_UPDATER_HPP
#define OSRM_UPDATER_UPDATER_HPP

#include "updater/updater_config.hpp"
#include "util/timezones.hpp"

#include "extractor/edge_based_edge.hpp"
#include "extractor/query_node.hpp"
#include "extractor/restriction.hpp"
#include "util/coordinate.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <chrono>
#include <vector>

namespace osrm
{
namespace updater
{
class Timezoner
{
  public:
    Timezoner() = default;
    Timezoner(std::string tz_filename)
    {
        std::time_t utc_time_now =
            std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        // Load R-tree with local times - returns a lambda that finds the local time of a timezone
        GetLocalTime = LoadLocalTimesRTree(tz_filename, utc_time_now);
    };

    std::function<struct tm(const point_t &)> GetLocalTime;

  private:
    unsigned now;
};

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
