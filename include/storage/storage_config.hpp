#ifndef STORAGE_CONFIG_HPP
#define STORAGE_CONFIG_HPP

#include <boost/filesystem/path.hpp>

#include <string>

namespace osrm
{
namespace storage
{
struct StorageConfig
{
    StorageConfig() = default;
    StorageConfig(const boost::filesystem::path &base);
    bool IsValid() const;

    std::string ram_index_path;
    std::string file_index_path;
    std::string hsgr_data_path;
    std::string nodes_data_path;
    std::string edges_data_path;
    std::string core_data_path;
    std::string geometries_path;
    std::string timestamp_path;
    std::string datasource_names_path;
    std::string datasource_indexes_path;
    std::string names_data_path;
    std::string properties_path;
};
}
}

#endif
