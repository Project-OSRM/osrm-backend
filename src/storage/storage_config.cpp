#include "storage/storage_config.hpp"
#include "util/simple_logger.hpp"

#include <boost/filesystem/operations.hpp>

namespace osrm
{
namespace storage
{

StorageConfig::StorageConfig(const boost::filesystem::path &base)
    : ram_index_path{base.string() + ".ramIndex"}, file_index_path{base.string() + ".fileIndex"},
      hsgr_data_path{base.string() + ".hsgr"}, nodes_data_path{base.string() + ".nodes"},
      edges_data_path{base.string() + ".edges"}, core_data_path{base.string() + ".core"},
      geometries_path{base.string() + ".geometry"}, timestamp_path{base.string() + ".timestamp"},
      turn_weight_penalties_path{base.string() + ".turn_weight_penalties"},
      turn_duration_penalties_path{base.string() + ".turn_duration_penalties"},
      datasource_names_path{base.string() + ".datasource_names"},
      datasource_indexes_path{base.string() + ".datasource_indexes"},
      names_data_path{base.string() + ".names"}, properties_path{base.string() + ".properties"},
      intersection_class_path{base.string() + ".icd"}, turn_lane_data_path{base.string() + ".tld"},
      turn_lane_description_path{base.string() + ".tls"}
{
}

bool StorageConfig::IsValid() const
{
    const constexpr auto num_files = 15;
    const boost::filesystem::path paths[num_files] = {ram_index_path,
                                                      file_index_path,
                                                      hsgr_data_path,
                                                      nodes_data_path,
                                                      edges_data_path,
                                                      core_data_path,
                                                      geometries_path,
                                                      timestamp_path,
                                                      turn_weight_penalties_path,
                                                      turn_duration_penalties_path,
                                                      datasource_indexes_path,
                                                      datasource_indexes_path,
                                                      names_data_path,
                                                      properties_path,
                                                      intersection_class_path};

    bool success = true;
    for (auto path = paths; path != paths + num_files; ++path)
    {
        if (!boost::filesystem::is_regular_file(*path))
        {
            util::SimpleLogger().Write(logWARNING) << "Missing/Broken File: " << path->string();
            success = false;
        }
    }

    return success;
}
}
}
