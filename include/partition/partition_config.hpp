#ifndef PARTITIONER_CONFIG_HPP
#define PARTITIONER_CONFIG_HPP

#include <boost/filesystem/path.hpp>

#include <array>
#include <string>

namespace osrm
{
namespace partition
{

struct PartitionConfig
{
    PartitionConfig()
        : requested_num_threads(0)
    {
    }

    void UseDefaults()
    {
        std::string basepath = base_path.string();

        const std::string ext = ".osrm";
        const auto pos = basepath.find(ext);
        if (pos != std::string::npos)
        {
            basepath.replace(pos, ext.size(), "");
        }
        else
        {
            // unknown extension
        }

        edge_based_graph_path = basepath + ".osrm.ebg";
        compressed_node_based_graph_path = basepath + ".osrm.cnbg";
        cnbg_ebg_mapping_path = basepath + ".osrm.cnbg_to_ebg";
        partition_path = basepath + ".osrm.partition";
        mld_partition_path = basepath + ".osrm.mld_partition";
        mld_storage_path = basepath + ".osrm.mld_storage";
    }

    // might be changed to the node based graph at some point
    boost::filesystem::path base_path;
    boost::filesystem::path edge_based_graph_path;
    boost::filesystem::path compressed_node_based_graph_path;
    boost::filesystem::path cnbg_ebg_mapping_path;
    boost::filesystem::path partition_path;
    boost::filesystem::path mld_partition_path;
    boost::filesystem::path mld_storage_path;

    unsigned requested_num_threads;

    std::size_t maximum_cell_size;
    double balance;
    double boundary_factor;
    std::size_t num_optimizing_cuts;
    std::size_t small_component_size;
};
}
}

#endif // PARTITIONER_CONFIG_HPP
