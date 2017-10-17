#ifndef TABLE_HPP
#define TABLE_HPP

#include "engine/plugins/plugin_base.hpp"

#include "engine/api/table_parameters.hpp"
#include "engine/api/table_result.hpp"
#include "engine/error.hpp"
#include "engine/routing_algorithms.hpp"

#include "util/json_container.hpp"

namespace osrm
{
namespace engine
{
namespace plugins
{

class TablePlugin final : public BasePlugin
{
  public:
    explicit TablePlugin(const int max_locations_distance_table);

    // to be removed in v6.0
    Status HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                         const api::TableParameters &params,
                         util::json::Object &result) const;

    MaybeResult<api::TableResult> HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                                                const api::TableParameters &params) const;

  private:
    const int max_locations_distance_table;
};
}
}
}

#endif // TABLE_HPP
