#ifndef TABLE_HPP
#define TABLE_HPP

#include "engine/plugins/plugin_base.hpp"

#include "engine/api/table_parameters.hpp"
#include "engine/routing_algorithms.hpp"
#include "engine/routing_algorithms/many_to_many.hpp"
#include "engine/search_engine_data.hpp"
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

    Status HandleRequest(const datafacade::ContiguousInternalMemoryDataFacadeBase &facade,
                         const RoutingAlgorithmsInterface &algorithms,
                         const api::TableParameters &params,
                         util::json::Object &result) const;

  private:
    const int max_locations_distance_table;
};
}
}
}

#endif // TABLE_HPP
