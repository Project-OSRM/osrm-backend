/*

Copyright (c) 2016, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

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
        turn_weight_penalties_path = osrm_input_path.string() + ".turn_weight_penalties";
        turn_duration_penalties_path = osrm_input_path.string() + ".turn_duration_penalties";
        turn_penalties_index_path = osrm_input_path.string() + ".turn_penalties_index";
        node_based_graph_path = osrm_input_path.string() + ".nodes";
        geometry_path = osrm_input_path.string() + ".geometry";
        rtree_leaf_path = osrm_input_path.string() + ".fileIndex";
        datasource_names_path = osrm_input_path.string() + ".datasource_names";
        datasource_indexes_path = osrm_input_path.string() + ".datasource_indexes";
    }

    boost::filesystem::path config_file_path;
    boost::filesystem::path osrm_input_path;

    std::string level_output_path;
    std::string core_output_path;
    std::string graph_output_path;
    std::string edge_based_graph_path;

    std::string edge_segment_lookup_path;
    std::string turn_weight_penalties_path;
    std::string turn_duration_penalties_path;
    std::string turn_penalties_index_path;
    std::string node_based_graph_path;
    std::string geometry_path;
    std::string rtree_leaf_path;
    bool use_cached_priority;

    unsigned requested_num_threads;
    double log_edge_updates_factor;

    // A percentage of vertices that will be contracted for the hierarchy.
    // Offers a trade-off between preprocessing and query time.
    // The remaining vertices form the core of the hierarchy
    //(e.g. 0.8 contracts 80 percent of the hierarchy, leaving a core of 20%)
    double core_factor;

    std::vector<std::string> segment_speed_lookup_paths;
    std::vector<std::string> turn_penalty_lookup_paths;
    std::string datasource_indexes_path;
    std::string datasource_names_path;
};
}
}

#endif // EXTRACTOR_OPTIONS_HPP
