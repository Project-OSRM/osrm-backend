#include "engine/engine_config.hpp"

namespace osrm::engine
{

bool EngineConfig::IsValid() const
{
    // check whether a base_path has been defined by verifying an empty extension
    // leads to an empty path
    const bool all_path_are_empty = storage_config.GetPath("").empty();

    const auto unlimited_or_more_than = [](const auto v, const auto limit)
    { return v == -1 || v > limit; };

    const auto positive = [](const auto v) { return v > 0; };

    const bool limits_valid =
        unlimited_or_more_than(max_locations_distance_table, 2) &&
        unlimited_or_more_than(max_locations_map_matching, 2) &&
        unlimited_or_more_than(max_radius_map_matching, 0) &&
        unlimited_or_more_than(max_locations_trip, 2) &&
        unlimited_or_more_than(max_locations_viaroute, 2) &&
        unlimited_or_more_than(max_results_nearest, 0) &&
        unlimited_or_more_than(max_locations_nearest, 0) &&
        unlimited_or_more_than(default_radius, 0) && max_alternatives >= 0 &&
        positive(max_isochrone_search_records) && positive(max_isochrone_materialized_points) &&
        positive(max_isochrone_rasterization_steps) && positive(max_isochrone_output_points) &&
        positive(max_isochrone_grid_cells) && positive(max_isochrone_contours);

    return ((use_shared_memory && all_path_are_empty) || (use_mmap && storage_config.IsValid()) ||
            storage_config.IsValid()) &&
           limits_valid;
}
} // namespace osrm::engine
