#ifndef TILEPLUGIN_HPP
#define TILEPLUGIN_HPP

#include "engine/api/tile_parameters.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "engine/routing_algorithms.hpp"

#include <utility>
#include <vector>

/*
 * This plugin generates Mapbox Vector tiles that show the internal
 * routing geometry and speed values on all road segments.
 * You can use this along with a vector-tile viewer, like Mapbox GL,
 * to display maps that show the exact road network that
 * OSRM is routing.  This is very useful for debugging routing
 * errors
 */
namespace osrm
{
namespace engine
{
namespace plugins
{

class TilePlugin final : public BasePlugin
{
  public:
    Status HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                         const api::TileParameters &parameters,
                         osrm::engine::api::ResultT &pbf_buffer) const;
};
}
}
}

#endif /* TILEPLUGIN_HPP */
