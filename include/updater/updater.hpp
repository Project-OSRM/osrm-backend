#ifndef OSRM_UPDATER_UPDATER_HPP
#define OSRM_UPDATER_UPDATER_HPP

#include "updater/updater_config.hpp"
#include "util/timezones.hpp"

#include "extractor/edge_based_edge.hpp"
#include "extractor/packed_osm_ids.hpp"
#include "extractor/query_node.hpp"
#include "extractor/restriction.hpp"
#include "util/coordinate.hpp"
#include "util/packed_vector.hpp"

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

    Timezoner(std::string tz_filename, std::time_t utc_time_now_)
    {
        utc_time_now = utc_time_now_;
        GetLocalTime = LoadLocalTimesRTree(tz_filename, utc_time_now);
    }

    Timezoner(std::string tz_filename)
        : Timezoner(tz_filename,
                    std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))
    {
    }

    std::time_t utc_time_now;
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

    EdgeID
    LoadAndUpdateEdgeExpandedGraph(std::vector<extractor::EdgeBasedEdge> &edge_based_edge_list,
                                   std::vector<EdgeWeight> &node_weights,
                                   std::vector<util::Coordinate> node_coordinates,
                                   extractor::PackedOSMIDs osm_node_ids) const;

  private:
    UpdaterConfig config;
};
}
}

#endif
