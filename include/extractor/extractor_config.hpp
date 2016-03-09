#ifndef EXTRACTOR_CONFIG_HPP
#define EXTRACTOR_CONFIG_HPP

#include <boost/filesystem/path.hpp>

#include <string>
#include <array>

namespace osrm
{
namespace extractor
{

struct ExtractorConfig
{
    ExtractorConfig() noexcept : requested_num_threads(0) {}
    void UseDefaultOutputNames()
    {
        std::string basepath = input_path.string();

        auto pos = std::string::npos;
        std::array<std::string, 5> known_extensions{
            {".osm.bz2", ".osm.pbf", ".osm.xml", ".pbf", ".osm"}};
        for (auto ext : known_extensions)
        {
            pos = basepath.find(ext);
            if (pos != std::string::npos)
            {
                basepath.replace(pos, ext.size(), "");
                break;
            }
        }

        output_file_name = basepath + ".osrm";
        restriction_file_name = basepath + ".osrm.restrictions";
        names_file_name = basepath + ".osrm.names";
        timestamp_file_name = basepath + ".osrm.timestamp";
        geometry_output_path = basepath + ".osrm.geometry";
        node_output_path = basepath + ".osrm.nodes";
        edge_output_path = basepath + ".osrm.edges";
        edge_graph_output_path = basepath + ".osrm.ebg";
        rtree_nodes_output_path = basepath + ".osrm.ramIndex";
        rtree_leafs_output_path = basepath + ".osrm.fileIndex";
        edge_segment_lookup_path = basepath + ".osrm.edge_segment_lookup";
        edge_penalty_path = basepath + ".osrm.edge_penalties";
        edge_based_node_weights_output_path = basepath + ".osrm.enw";
    }

    boost::filesystem::path config_file_path;
    boost::filesystem::path input_path;
    boost::filesystem::path profile_path;

    std::string output_file_name;
    std::string restriction_file_name;
    std::string names_file_name;
    std::string timestamp_file_name;
    std::string geometry_output_path;
    std::string edge_output_path;
    std::string edge_graph_output_path;
    std::string edge_based_node_weights_output_path;
    std::string node_output_path;
    std::string rtree_nodes_output_path;
    std::string rtree_leafs_output_path;

    unsigned requested_num_threads;
    unsigned small_component_size;

    bool generate_edge_lookup;
    std::string edge_penalty_path;
    std::string edge_segment_lookup_path;

    bool generate_turn_lookup;
};
}
}

#endif // EXTRACTOR_CONFIG_HPP
