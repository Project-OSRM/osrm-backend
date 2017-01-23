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
    PartitionConfig() noexcept : requested_num_threads(0) {}

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
        partition_path = basepath + ".osrm.partition";
    }

    // might be changed to the node based graph at some point
    boost::filesystem::path base_path;
    boost::filesystem::path edge_based_graph_path;
    boost::filesystem::path partition_path;

    unsigned requested_num_threads;
};
}
}

#endif // PARTITIONER_CONFIG_HPP
