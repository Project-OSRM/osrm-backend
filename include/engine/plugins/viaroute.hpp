#ifndef VIA_ROUTE_HPP
#define VIA_ROUTE_HPP

#include "engine/datafacade/datafacade_base.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "engine/api/route_api.hpp"

#include "engine/object_encoder.hpp"
#include "engine/search_engine_data.hpp"
#include "engine/routing_algorithms/shortest_path.hpp"
#include "engine/routing_algorithms/alternative_path.hpp"
#include "engine/routing_algorithms/direct_shortest_path.hpp"
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
    SearchEngineData heaps;
    routing_algorithms::ShortestPathRouting<datafacade::BaseDataFacade> shortest_path;
    routing_algorithms::AlternativeRouting<datafacade::BaseDataFacade> alternative_path;
    routing_algorithms::DirectShortestPathRouting<datafacade::BaseDataFacade> direct_shortest_path;
    int max_locations_viaroute;

  public:
    explicit ViaRoutePlugin(datafacade::BaseDataFacade &facade, int max_locations_viaroute);

    Status HandleRequest(const api::RouteParameters &route_parameters,
                         util::json::Object &json_result);
};
}
}
}

#endif // VIA_ROUTE_HPP
