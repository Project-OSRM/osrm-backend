#ifndef TABLE_HPP
#define TABLE_HPP

#include "engine/plugins/plugin_base.hpp"

#include "engine/api/table_parameters.hpp"
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
    explicit TablePlugin(datafacade::BaseDataFacade &facade,
                         const int max_locations_distance_table,
                         const double max_radius_when_bearings);

    Status HandleRequest(const api::TableParameters &params, util::json::Object &result);

  private:
    SearchEngineData heaps;
    routing_algorithms::ManyToManyRouting<datafacade::BaseDataFacade> distance_table;
    int max_locations_distance_table;
};
}
}
}

#endif // TABLE_HPP
