#include "engine/engine_config.hpp"

namespace osrm
{
namespace engine
{

bool EngineConfig::IsValid() const
{
    // check whether a base_bath has been defined by verifying an empty extension
    // leads to an empty path
    const bool all_path_are_empty = storage_config.GetPath("").empty();

    const auto unlimited_or_more_than = [](const auto v, const auto limit) {
        return v == -1 || v > limit;
    };

    const bool limits_valid = unlimited_or_more_than(max_locations_distance_table, 2) &&
                              unlimited_or_more_than(max_locations_map_matching, 2) &&
                              unlimited_or_more_than(max_radius_map_matching, 0) &&
                              unlimited_or_more_than(max_locations_trip, 2) &&
                              unlimited_or_more_than(max_locations_viaroute, 2) &&
                              unlimited_or_more_than(max_results_nearest, 0) &&
                              max_alternatives >= 0;

    return ((use_shared_memory && all_path_are_empty) || (use_mmap && storage_config.IsValid()) ||
            storage_config.IsValid()) &&
           limits_valid;
}
}
}
