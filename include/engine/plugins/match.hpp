#ifndef MATCH_HPP
#define MATCH_HPP

#include "engine/api/match_parameters.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "engine/routing_algorithms.hpp"

#include "util/json_util.hpp"

#include <vector>

namespace osrm::engine::plugins
{

class MatchPlugin : public BasePlugin
{
  public:
    using SubMatching = map_matching::SubMatching;
    using SubMatchingList = routing_algorithms::SubMatchingList;
    using CandidateLists = routing_algorithms::CandidateLists;
    static const constexpr double RADIUS_MULTIPLIER = 3;

    MatchPlugin(const int max_locations_map_matching,
                const double max_radius_map_matching,
                const boost::optional<double> default_radius)
        : BasePlugin(default_radius), max_locations_map_matching(max_locations_map_matching),
          max_radius_map_matching(max_radius_map_matching)
    {
    }

    Status HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                         const api::MatchParameters &parameters,
                         osrm::engine::api::ResultT &json_result) const;

  private:
    const int max_locations_map_matching;
    const double max_radius_map_matching;
};
} // namespace osrm::engine::plugins

#endif // MATCH_HPP
