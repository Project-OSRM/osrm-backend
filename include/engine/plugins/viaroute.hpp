#ifndef VIA_ROUTE_HPP
#define VIA_ROUTE_HPP

#include "engine/plugins/plugin_base.hpp"

#include "engine/api/route_api.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "engine/routing_algorithms.hpp"
#include "engine/search_engine_data.hpp"
#include "util/json_container.hpp"

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace osrm
{
namespace engine
{
namespace plugins
{

class ViaRoutePlugin final : public BasePlugin
{
  private:
    const int max_locations_viaroute;

  public:
    explicit ViaRoutePlugin(int max_locations_viaroute);

    Status HandleRequest(const datafacade::ContiguousInternalMemoryDataFacadeBase &facade,
                         const RoutingAlgorithmsInterface &algorithms,
                         const api::RouteParameters &route_parameters,
                         util::json::Object &json_result) const;
};
}
}
}

#endif // VIA_ROUTE_HPP
