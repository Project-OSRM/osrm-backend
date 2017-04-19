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
        if (!boost::filesystem::exists(path))
        {
            util::Log(logERROR) << "Missing File: " << path.string();
            success = false;
        }
    }
    return success;
}
}

bool StorageConfig::IsValid() const
{
    // Common files
    if (!CheckFileList({ram_index_path,
                        file_index_path,
                        node_based_nodes_data_path,
                        edge_based_nodes_data_path,
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

    return true;
}
}
}
