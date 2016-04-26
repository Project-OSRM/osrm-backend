#include "storage/storage_config.hpp"

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
      datasource_names_path{base.string() + ".datasource_names"},
      datasource_indexes_path{base.string() + ".datasource_indexes"},
      names_data_path{base.string() + ".names"}, properties_path{base.string() + ".properties"},
      intersection_class_path{base.string() + ".icd"}
{
}

bool StorageConfig::IsValid() const
{
    return boost::filesystem::is_regular_file(ram_index_path) &&
           boost::filesystem::is_regular_file(file_index_path) &&
           boost::filesystem::is_regular_file(hsgr_data_path) &&
           boost::filesystem::is_regular_file(nodes_data_path) &&
           boost::filesystem::is_regular_file(edges_data_path) &&
           boost::filesystem::is_regular_file(core_data_path) &&
           boost::filesystem::is_regular_file(geometries_path) &&
           boost::filesystem::is_regular_file(timestamp_path) &&
           boost::filesystem::is_regular_file(datasource_names_path) &&
           boost::filesystem::is_regular_file(datasource_indexes_path) &&
           boost::filesystem::is_regular_file(names_data_path) &&
           boost::filesystem::is_regular_file(properties_path) &&
           boost::filesystem::is_regular_file(intersection_class_path);
}
}
}
