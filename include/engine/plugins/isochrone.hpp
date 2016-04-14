//
// Created by robin on 4/6/16.
//

#ifndef ISOCHRONE_HPP
#define ISOCHRONE_HPP

#include "engine/plugins/plugin_base.hpp"
#include "engine/api/isochrone_parameters.hpp"
#include "osrm/json_container.hpp"

namespace osrm
{
namespace engine
{
namespace plugins
{

class IsochronePlugin final : public BasePlugin
{
  public:
    explicit IsochronePlugin(datafacade::BaseDataFacade &facade);

    Status HandleRequest(const api::IsochroneParameters &params,
                         util::json::Object &json_result);
};
}
}
}

#endif // ISOCHRONE_HPP
