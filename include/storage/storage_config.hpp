#ifndef STORAGE_CONFIG_HPP
#define STORAGE_CONFIG_HPP

#include <boost/filesystem/path.hpp>

namespace osrm
{
namespace storage
{
struct StorageConfig
{
    StorageConfig() = default;
    StorageConfig(const boost::filesystem::path &base);
    bool IsValid() const;

    boost::filesystem::path ram_index_path;
    boost::filesystem::path file_index_path;
    boost::filesystem::path hsgr_data_path;
    boost::filesystem::path nodes_data_path;
    boost::filesystem::path edges_data_path;
    boost::filesystem::path core_data_path;
    boost::filesystem::path geometries_path;
    boost::filesystem::path timestamp_path;
    boost::filesystem::path datasource_names_path;
    boost::filesystem::path datasource_indexes_path;
    boost::filesystem::path names_data_path;
    boost::filesystem::path properties_path;
};
}
}

#endif
