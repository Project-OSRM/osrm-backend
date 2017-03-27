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
        : requested_num_threads(0), balance(1.2), boundary_factor(0.25), num_optimizing_cuts(10),
          small_component_size(1000),
          max_cell_sizes{128, 128 * 32, 128 * 32 * 16, 128 * 32 * 16 * 32}
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
        mld_partition_path = basepath + ".osrm.partition";
        mld_storage_path = basepath + ".osrm.cells";
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

    double balance;
    double boundary_factor;
    std::size_t num_optimizing_cuts;
    std::size_t small_component_size;
    std::vector<std::size_t> max_cell_sizes;
};
}
}

#endif // PARTITIONER_CONFIG_HPP
