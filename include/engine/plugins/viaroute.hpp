#ifndef VIA_ROUTE_HPP
#define VIA_ROUTE_HPP

#include "engine/api/route_api.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "engine/plugins/plugin_base.hpp"

#include "engine/routing_algorithms/alternative_path.hpp"
#include "engine/routing_algorithms/direct_shortest_path.hpp"
#include "engine/routing_algorithms/shortest_path.hpp"
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
    mutable SearchEngineData heaps;
    mutable routing_algorithms::ShortestPathRouting<datafacade::BaseDataFacade> shortest_path;
    mutable routing_algorithms::AlternativeRouting<datafacade::BaseDataFacade> alternative_path;
    mutable routing_algorithms::DirectShortestPathRouting<datafacade::BaseDataFacade> direct_shortest_path;
    const int max_locations_viaroute;

  public:
    explicit ViaRoutePlugin(int max_locations_viaroute);

    Status HandleRequest(const std::shared_ptr<datafacade::BaseDataFacade> facade,
                         const api::RouteParameters &route_parameters,
                         util::json::Object &json_result) const;
};
}
}
}

#endif // VIA_ROUTE_HPP
