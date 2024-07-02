#ifndef VIA_ROUTE_HPP
#define VIA_ROUTE_HPP

#include "engine/plugins/plugin_base.hpp"

#include "engine/api/route_parameters.hpp"
#include "engine/routing_algorithms.hpp"

#include "util/json_container.hpp"

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace osrm::engine::plugins
{

class ViaRoutePlugin final : public BasePlugin
{
  private:
    const int max_locations_viaroute;
    const int max_alternatives;

  public:
    explicit ViaRoutePlugin(int max_locations_viaroute,
                            int max_alternatives,
                            std::optional<double> default_radius);

    Status HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                         const api::RouteParameters &route_parameters,
                         osrm::engine::api::ResultT &json_result) const;
};
} // namespace osrm::engine::plugins

#endif // VIA_ROUTE_HPP
