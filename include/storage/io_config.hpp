#ifndef OSRM_IO_CONFIG_HPP
#define OSRM_IO_CONFIG_HPP

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/path.hpp>

namespace osrm
{
namespace storage
{
struct IOConfig
{
    IOConfig(std::vector<boost::filesystem::path> _required_input_files,
      std::vector<boost::filesystem::path> _optional_input_files,
      std::vector<boost::filesystem::path> _output_files)
      : required_input_files(_required_input_files),
      optional_input_files(_optional_input_files),
      output_files(_output_files)
    {}

    // Infer the base path from the path of the .osrm file
    void UseDefaultOutputNames(const boost::filesystem::path &base)
    {
        // potentially strip off the .osrm extension for
        // determining the base path
        const std::string ext = ".osrm";
        const auto pos = base.find(ext);
        if (pos != std::string::npos)
        {
            base.replace(pos, ext.size(), "");
        }
        else
        {
            // unknown extension or no extension
        }

        base_path = base;
    }

    bool IsValid()
    {
        bool success = true;
        for (auto &path : required_input_files)
        {
            if (!boost::filesystem::is_regular_file(path))
            {
                util::Log(logWARNING) << "Missing/Broken File: " << path.string();
                success = false;
            }
        }
        return success;
    }

    boost::filesystem::path GetPath(const std::string &fileName)
    {
        if(!IsConfigured(fileName, required_input_files)
          && !IsConfigured(fileName, optional_input_files)
          && !IsConfigured(fileName, output_files)) {
            util::Log(logERROR) << "Missing File: " << fileName;
            return 0; // TODO throw
        }

        return { base_path.string() + fileName };
    }

    private:
      bool IsConfigured(const std::string &fileName, const std::vector<boost::filesystem::path> &paths) {
        for (auto &path : paths) {
            if(boost::algorithm::ends_with(path.string(), fileName)) {
                return true;
            }
        }
      }

      std::vector<boost::filesystem::path> required_input_files;
      std::vector<boost::filesystem::path> optional_input_files;
      std::vector<boost::filesystem::path> output_files;

      /*
      boost::filesystem::path base_path;

      ram_index_path = {osrm_path.string() + ".ramIndex"};
      file_index_path = {osrm_path.string() + ".fileIndex"};
      hsgr_data_path = {osrm_path.string() + ".hsgr"};
      nodes_data_path = {osrm_path.string() + ".nodes"};
      edges_data_path = {osrm_path.string() + ".edges"};
      core_data_path = {osrm_path.string() + ".core"};
      geometries_path = {osrm_path.string() + ".geometry"};
      timestamp_path = {osrm_path.string() + ".timestamp"};
      turn_weight_penalties_path = {osrm_path.string() + ".turn_weight_penalties"};
      turn_duration_penalties_path = {osrm_path.string() + ".turn_duration_penalties"};
      turn_penalties_index_path = {osrm_path.string() + ".turn_penalties_index"};
      datasource_names_path = {osrm_path.string() + ".datasource_names"};
      names_data_path = {osrm_path.string() + ".names"};
      properties_path = {osrm_path.string() + ".properties"};
      intersection_class_path = {osrm_path.string() + ".icd"};
      turn_lane_data_path = {osrm_path.string() + ".tld"};
      turn_lane_description_path = {osrm_path.string() + ".tls"};
      mld_partition_path = {osrm_path.string() + ".partition"};
      mld_storage_path = {osrm_path.string() + ".cells"};
      mld_graph_path = {osrm_path.string() + ".mldgr"};
      level_path = {osrm_path.string() + ".level"};
      node_path = {osrm_path.string() + ".enw"};
      edge_based_nodes_data_path = {osrm_path.string() + ".ebg_nodes"};
      node_based_nodes_data_path = {osrm_path.string() + ".nbg_nodes"};
      edge_based_graph_path = {osrm_path.string() + ".ebg"};
      compressed_node_based_graph_path = {osrm_path.string() + ".cnbg"};
      cnbg_ebg_mapping_path = {osrm_path.string() + ".cnbg_to_ebg"};
      turn_restrictions_path = {osrm_path.string() + ".restrictions"};
      intersection_class_data_path = {osrm_path.string() + ".icd"};
      */

      boost::filesystem::path osrm_path;
      boost::filesystem::path ram_index_path;
      boost::filesystem::path file_index_path;
      boost::filesystem::path hsgr_data_path;
      boost::filesystem::path nodes_data_path;
      boost::filesystem::path edges_data_path;
      boost::filesystem::path core_data_path;
      boost::filesystem::path geometries_path;
      boost::filesystem::path timestamp_path;
      boost::filesystem::path turn_weight_penalties_path;
      boost::filesystem::path turn_duration_penalties_path;
      boost::filesystem::path turn_penalties_index_path;
      boost::filesystem::path datasource_names_path;
      boost::filesystem::path datasource_indexes_path;
      boost::filesystem::path names_data_path;
      boost::filesystem::path properties_path;
      boost::filesystem::path intersection_class_path;
      boost::filesystem::path turn_lane_data_path;
      boost::filesystem::path turn_lane_description_path;
      boost::filesystem::path mld_partition_path;
      boost::filesystem::path mld_storage_path;
      boost::filesystem::path mld_graph_path;
      boost::filesystem::path level_path;
      boost::filesystem::path node_path;
      boost::filesystem::path edge_based_nodes_data_path;
      boost::filesystem::path node_based_nodes_data_path;
      boost::filesystem::path edge_based_graph_path;
      boost::filesystem::path compressed_node_based_graph_path;
      boost::filesystem::path cnbg_ebg_mapping_path;
      boost::filesystem::path turn_restrictions_path;
      boost::filesystem::path intersection_class_data_path;
};
}
}

#endif
