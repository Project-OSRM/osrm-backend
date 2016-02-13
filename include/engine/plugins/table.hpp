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
  private:
    SearchEngineData heaps;
    routing_algorithms::ManyToManyRouting<datafacade::BaseDataFacade> distance_table;
    int max_locations_distance_table;

  public:
    explicit TablePlugin(datafacade::BaseDataFacade &facade,
                         const int max_locations_distance_table);

    Status HandleRequest(const api::TableParameters &params, util::json::Object &result);
};
}
}
}

#endif // TABLE_HPP
