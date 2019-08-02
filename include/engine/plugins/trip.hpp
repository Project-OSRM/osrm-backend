#ifndef TRIP_HPP
#define TRIP_HPP

#include "engine/plugins/plugin_base.hpp"

#include "engine/api/trip_parameters.hpp"
#include "engine/routing_algorithms.hpp"

#include "util/json_container.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{
namespace plugins
{

class TripPlugin final : public BasePlugin
{
  private:
    const int max_locations_trip;

    InternalRouteResult ComputeRoute(const RoutingAlgorithmsInterface &algorithms,
                                     const std::vector<PhantomNode> &phantom_node_list,
                                     const std::vector<NodeID> &trip,
                                     const bool roundtrip) const;

  public:
    explicit TripPlugin(const int max_locations_trip_) : max_locations_trip(max_locations_trip_) {}

    Status HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                         const api::TripParameters &parameters,
                         osrm::engine::api::ResultT &json_result) const;
};
}
}
}

#endif // TRIP_HPP
