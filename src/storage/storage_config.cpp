#include "storage/storage_config.hpp"
#include "util/log.hpp"

#include <boost/filesystem/operations.hpp>

namespace osrm
{
namespace storage
{
namespace
{
bool CheckFileList(const std::vector<boost::filesystem::path> &files)
{
    bool success = true;
    for (auto &path : files)
    {
        if (!boost::filesystem::is_regular_file(path))
        {
            util::Log(logWARNING) << "Missing/Broken File: " << path.string();
            success = false;
        }
    }
    return success;
}
}

StorageConfig::StorageConfig(const boost::filesystem::path &base)
    : ram_index_path{base.string() + ".ramIndex"}, file_index_path{base.string() + ".fileIndex"},
      hsgr_data_path{base.string() + ".hsgr"}, nodes_data_path{base.string() + ".nodes"},
      edges_data_path{base.string() + ".edges"}, core_data_path{base.string() + ".core"},
      geometries_path{base.string() + ".geometry"}, timestamp_path{base.string() + ".timestamp"},
      turn_weight_penalties_path{base.string() + ".turn_weight_penalties"},
      turn_duration_penalties_path{base.string() + ".turn_duration_penalties"},
      datasource_names_path{base.string() + ".datasource_names"},
      names_data_path{base.string() + ".names"}, properties_path{base.string() + ".properties"},
      intersection_class_path{base.string() + ".icd"}, turn_lane_data_path{base.string() + ".tld"},
      turn_lane_description_path{base.string() + ".tls"},
      mld_partition_path{base.string() + ".partition"}, mld_storage_path{base.string() + ".cells"},
      mld_graph_path{base.string() + ".mldgr"}
{
}

bool StorageConfig::IsValid() const
{
    // Common files
    if (!CheckFileList({ram_index_path,
                        file_index_path,
                        nodes_data_path,
                        edges_data_path,
                        geometries_path,
                        timestamp_path,
                        turn_weight_penalties_path,
                        turn_duration_penalties_path,
                        names_data_path,
                        properties_path,
                        intersection_class_path,
                        datasource_names_path}))
    {
        return false;
    }

    // TODO: add algorithm checks

    // CH files
    CheckFileList({hsgr_data_path, core_data_path});

    // MLD files
    CheckFileList({mld_partition_path, mld_storage_path, mld_graph_path});

    return true;
}
}
}
