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
    explicit NearestPlugin(datafacade::BaseDataFacade &facade,
                           const int max_results_nearest,
                           const double max_radius_when_bearings);

    Status HandleRequest(const api::NearestParameters &params, util::json::Object &result);

  private:
    const int max_results;
};
}
}
}

#endif /* NEAREST_HPP */
