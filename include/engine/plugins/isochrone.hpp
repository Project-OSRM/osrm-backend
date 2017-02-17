#ifndef ISOCHRONEPLUGIN_HPP
#define ISOCHRONEPLUGIN_HPP

#include "engine/api/isochrone_parameters.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "engine/routing_algorithms/shortest_path.hpp"
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
  private:
    mutable SearchEngineData heaps;
    mutable routing_algorithms::ShortestPathRouting shortest_path;

  public:
    explicit IsochronePlugin();

    Status HandleRequest(const std::shared_ptr<const datafacade::BaseDataFacade> facade,
                         const api::IsochroneParameters &parameters,
                         std::string &pbf_buffer) const;
};
}
}
}

#endif /* TILEPLUGIN_HPP */
