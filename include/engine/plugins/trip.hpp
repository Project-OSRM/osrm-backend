#ifndef TRIP_HPP
#define TRIP_HPP

#include "engine/plugins/plugin_base.hpp"

#include "engine/api/trip_parameters.hpp"
#include "engine/routing_algorithms.hpp"

#include "util/json_container.hpp"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <vector>

namespace osrm::engine::plugins
{

class TripPlugin final : public BasePlugin
{
  private:
    const int max_locations_trip;

    //! Not const: a leg solved inside an open area reports the coordinate that was asked
    //! for, so the candidates it answers to are corrected too.
    InternalRouteResult ComputeRoute(const RoutingAlgorithmsInterface &algorithms,
                                     std::vector<PhantomNodeCandidates> &candidates_list,
                                     const std::vector<NodeID> &trip,
                                     const bool roundtrip) const;

  public:
    explicit TripPlugin(const int max_locations_trip_, std::optional<double> default_radius)
        : BasePlugin(default_radius), max_locations_trip(max_locations_trip_)
    {
    }

    Status HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                         const api::TripParameters &parameters,
                         osrm::engine::api::ResultT &json_result) const;
};
} // namespace osrm::engine::plugins

#endif // TRIP_HPP
