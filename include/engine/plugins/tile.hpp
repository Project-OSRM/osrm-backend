#ifndef TILEPLUGIN_HPP
#define TILEPLUGIN_HPP

#include "engine/api/tile_parameters.hpp"
#include "engine/plugins/plugin_base.hpp"

#include <string>

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
    TilePlugin(datafacade::BaseDataFacade &facade) : BasePlugin(facade, EngineConfig::UNLIMITED) {}

    Status HandleRequest(const api::TileParameters &parameters, std::string &pbf_buffer);
};
}
}
}

#endif /* TILEPLUGIN_HPP */
