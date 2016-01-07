#ifndef CONTRACTOR_OPTIONS_HPP
#define CONTRACTOR_OPTIONS_HPP

#include <boost/filesystem/path.hpp>

#include <string>

namespace osrm
{
namespace contractor
{

struct ContractorConfig
{
    ContractorConfig() : requested_num_threads(0) {}

    // Infer the output names from the path of the .osrm file
    void UseDefaultOutputNames()
    {
        level_output_path = osrm_input_path.string() + ".level";
        core_output_path = osrm_input_path.string() + ".core";
        graph_output_path = osrm_input_path.string() + ".hsgr";
        edge_based_graph_path = osrm_input_path.string() + ".ebg";
        edge_segment_lookup_path = osrm_input_path.string() + ".edge_segment_lookup";
        edge_penalty_path = osrm_input_path.string() + ".edge_penalties";
    }

    boost::filesystem::path config_file_path;
    boost::filesystem::path osrm_input_path;
    boost::filesystem::path profile_path;

    std::string level_output_path;
    std::string core_output_path;
    std::string graph_output_path;
    std::string edge_based_graph_path;

    std::string edge_segment_lookup_path;
    std::string edge_penalty_path;
    bool use_cached_priority;

    unsigned requested_num_threads;

    // A percentage of vertices that will be contracted for the hierarchy.
    // Offers a trade-off between preprocessing and query time.
    // The remaining vertices form the core of the hierarchy
    //(e.g. 0.8 contracts 80 percent of the hierarchy, leaving a core of 20%)
    double core_factor;

    std::string segment_speed_lookup_path;

#ifdef DEBUG_GEOMETRY
    std::string debug_geometry_path;
#endif
};
}
}

#endif // EXTRACTOR_OPTIONS_HPP
