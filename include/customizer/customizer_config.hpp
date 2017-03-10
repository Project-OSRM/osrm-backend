#ifndef OSRM_CUSTOMIZE_CUSTOMIZER_CONFIG_HPP
#define OSRM_CUSTOMIZE_CUSTOMIZER_CONFIG_HPP

#include "updater/updater_config.hpp"

#include <boost/filesystem/path.hpp>

#include <array>
#include <string>

namespace osrm
{
namespace customizer
{

struct CustomizationConfig
{
    CustomizationConfig() : requested_num_threads(0) {}

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
        mld_partition_path = basepath + ".osrm.partition";
        mld_storage_path = basepath + ".osrm.cells";
        mld_graph_path = basepath + ".osrm.mldgr";

        updater_config.osrm_input_path = basepath + ".osrm";
        updater_config.UseDefaultOutputNames();
    }

    // might be changed to the node based graph at some point
    boost::filesystem::path base_path;
    boost::filesystem::path edge_based_graph_path;
    boost::filesystem::path mld_partition_path;
    boost::filesystem::path mld_storage_path;
    boost::filesystem::path mld_graph_path;

    unsigned requested_num_threads;

    updater::UpdaterConfig updater_config;
};
}
}

#endif // OSRM_CUSTOMIZE_CUSTOMIZER_CONFIG_HPP
