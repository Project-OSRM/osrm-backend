#include "engine/engine_config.hpp"

namespace osrm
{
namespace engine
{

bool EngineConfig::IsValid() const
{
    const bool all_path_are_empty =
        storage_config.ram_index_path.empty() && storage_config.file_index_path.empty() &&
        storage_config.hsgr_data_path.empty() && storage_config.nodes_data_path.empty() &&
        storage_config.edges_data_path.empty() && storage_config.core_data_path.empty() &&
        storage_config.geometries_path.empty() && storage_config.timestamp_path.empty() &&
        storage_config.datasource_names_path.empty() &&
        storage_config.datasource_indexes_path.empty() && storage_config.names_data_path.empty();

    const bool limits_valid =
        (max_locations_distance_table == -1 || max_locations_distance_table > 2) &&
        (max_locations_map_matching == -1 || max_locations_map_matching > 2) &&
        (max_locations_trip == -1 || max_locations_trip > 2) &&
        (max_locations_viaroute == -1 || max_locations_viaroute > 2);

    return ((use_shared_memory && all_path_are_empty) || storage_config.IsValid()) && limits_valid;
}
}
}
