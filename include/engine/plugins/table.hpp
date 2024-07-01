#ifndef TABLE_HPP
#define TABLE_HPP

#include "engine/plugins/plugin_base.hpp"

#include "engine/api/table_parameters.hpp"
#include "engine/routing_algorithms.hpp"

#include "util/json_container.hpp"

namespace osrm::engine::plugins
{

class TablePlugin final : public BasePlugin
{
  public:
    explicit TablePlugin(const int max_locations_distance_table,
                         const std::optional<double> default_radius);

    Status HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                         const api::TableParameters &params,
                         osrm::engine::api::ResultT &result) const;

  private:
    const int max_locations_distance_table;
};
} // namespace osrm::engine::plugins

#endif // TABLE_HPP
