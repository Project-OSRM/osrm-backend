#ifndef ISOCHRONE_HPP
#define ISOCHRONE_HPP

#include "engine/api/isochrone_parameters.hpp"
#include "engine/plugins/plugin_base.hpp"

#include <cstddef>
#include <optional>

namespace osrm::engine::plugins
{

class IsochronePlugin final : public BasePlugin
{
  public:
    IsochronePlugin(const std::optional<double> default_radius,
                    std::size_t max_search_records,
                    std::size_t max_materialized_points,
                    std::size_t max_rasterization_steps,
                    std::size_t max_output_points,
                    std::size_t max_grid_cells,
                    std::size_t max_contours)
        : BasePlugin(default_radius), max_search_records(max_search_records),
          max_materialized_points(max_materialized_points),
          max_rasterization_steps(max_rasterization_steps), max_output_points(max_output_points),
          max_grid_cells(max_grid_cells), max_contours(max_contours)
    {
    }

    Status HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                         const api::IsochroneParameters &parameters,
                         osrm::engine::api::ResultT &result) const;

  private:
    const std::size_t max_search_records;
    const std::size_t max_materialized_points;
    const std::size_t max_rasterization_steps;
    const std::size_t max_output_points;
    const std::size_t max_grid_cells;
    const std::size_t max_contours;
};

} // namespace osrm::engine::plugins

#endif // ISOCHRONE_HPP
