#ifndef NEAREST_HPP
#define NEAREST_HPP

#include "engine/api/nearest_parameters.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "osrm/json_container.hpp"

namespace osrm
{
namespace engine
{
namespace plugins
{

class NearestPlugin final : public BasePlugin
{
  public:
    explicit NearestPlugin(datafacade::BaseDataFacade &facade);

    Status HandleRequest(const api::NearestParameters &params, util::json::Object &result);
};
}
}
}

#endif /* NEAREST_HPP */
