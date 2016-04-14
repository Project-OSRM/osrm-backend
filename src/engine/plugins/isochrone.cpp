//
// Created by robin on 4/13/16.
//

#include "engine/plugins/isochrone.hpp"
#include "engine/api/isochrone_parameters.hpp"
#include "engine/api/isochrone_api.hpp"

namespace osrm
{
namespace engine
{
namespace plugins
{
IsochronePlugin::IsochronePlugin(datafacade::BaseDataFacade &facade) : BasePlugin{facade} {}

Status IsochronePlugin::HandleRequest(const api::IsochroneParameters &params,
                                      util::json::Object &json_result)
{

    return Status::Ok;
}
}
}
}
