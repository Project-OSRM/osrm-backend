#ifndef OSRM_IO_CONFIG_HPP
#define OSRM_IO_CONFIG_HPP

#include <boost/filesystem/path.hpp>

namespace osrm
{
namespace storage
{
struct IOConfig
{
    IOConfig() = default;

    IOConfig(const boost::filesystem::path &base) : osrm_path(base) { UseDefaultOutputNames(); }

    // Infer the output names from the path of the .osrm file
    void UseDefaultOutputNames()
    {
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
        edge_based_nodes_data_path = {osrm_path.string() + ".nodes_data"};
        edge_based_graph_path = {osrm_path.string() + ".ebg"};
        compressed_node_based_graph_path = {osrm_path.string() + ".cnbg"};
        cnbg_ebg_mapping_path = {osrm_path.string() + ".cnbg_to_ebg"};
        restriction_path = {osrm_path.string() + ".restrictions"};
        intersection_class_data_path = {osrm_path.string() + ".icd"};
    }

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
    boost::filesystem::path edge_based_graph_path;
    boost::filesystem::path compressed_node_based_graph_path;
    boost::filesystem::path cnbg_ebg_mapping_path;
    boost::filesystem::path restriction_path;
    boost::filesystem::path intersection_class_data_path;
};
}
}

#endif
