#ifndef ISOCHRONEPLUGIN_HPP
#define ISOCHRONEPLUGIN_HPP

#include "engine/api/isochrone_parameters.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/routing_algorithms.hpp"
#include "engine/search_engine_data.hpp"

#include <string>

/**
 * TODO
 */
namespace osrm
{
namespace engine
{
namespace plugins
{

class IsochronePlugin final : public BasePlugin
{
  public:
    explicit IsochronePlugin();

    Status HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                         const api::IsochroneParameters &parameters,
                         osrm::engine::api::ResultT &result) const;
};
}
}
}

#endif /* TILEPLUGIN_HPP */
