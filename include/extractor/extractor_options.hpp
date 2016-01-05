#ifndef EXTRACTOR_OPTIONS_HPP
#define EXTRACTOR_OPTIONS_HPP

#include <boost/filesystem/path.hpp>

#include <string>

namespace osrm
{
namespace extractor
{

enum class return_code : unsigned
{
    ok,
    fail,
    exit
};

struct ExtractorConfig
{
    ExtractorConfig() noexcept : requested_num_threads(0) {}
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
    std::string node_output_path;
    std::string rtree_nodes_output_path;
    std::string rtree_leafs_output_path;

    unsigned requested_num_threads;
    unsigned small_component_size;

    bool generate_edge_lookup;
    std::string edge_penalty_path;
    std::string edge_segment_lookup_path;
#ifdef DEBUG_GEOMETRY
    std::string debug_turns_path;
#endif
};

struct ExtractorOptions
{
    static return_code ParseArguments(int argc, char *argv[], ExtractorConfig &extractor_config);

    static void GenerateOutputFilesNames(ExtractorConfig &extractor_config);
};

}
}

#endif // EXTRACTOR_OPTIONS_HPP
