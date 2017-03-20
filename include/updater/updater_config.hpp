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

#ifndef OSRM_UPDATER_UPDATER_CONFIG_HPP
#define OSRM_UPDATER_UPDATER_CONFIG_HPP

#include <boost/filesystem/path.hpp>

#include <string>

namespace osrm
{
namespace updater
{

struct UpdaterConfig final
{
    // Infer the output names from the path of the .osrm file
    void UseDefaultOutputNames()
    {
        edge_based_graph_path = osrm_input_path.string() + ".ebg";
        turn_weight_penalties_path = osrm_input_path.string() + ".turn_weight_penalties";
        turn_duration_penalties_path = osrm_input_path.string() + ".turn_duration_penalties";
        turn_penalties_index_path = osrm_input_path.string() + ".turn_penalties_index";
        node_based_graph_path = osrm_input_path.string() + ".nodes";
        edge_data_path = osrm_input_path.string() + ".edges";
        geometry_path = osrm_input_path.string() + ".geometry";
        rtree_leaf_path = osrm_input_path.string() + ".fileIndex";
        datasource_names_path = osrm_input_path.string() + ".datasource_names";
        profile_properties_path = osrm_input_path.string() + ".properties";
    }

    boost::filesystem::path osrm_input_path;

    std::string edge_based_graph_path;

    std::string turn_weight_penalties_path;
    std::string turn_duration_penalties_path;
    std::string turn_penalties_index_path;
    std::string node_based_graph_path;
    std::string edge_data_path;
    std::string geometry_path;
    std::string rtree_leaf_path;

    double log_edge_updates_factor;

    std::vector<std::string> segment_speed_lookup_paths;
    std::vector<std::string> turn_penalty_lookup_paths;
    std::string datasource_names_path;
    std::string profile_properties_path;
};
}
}

#endif // EXTRACTOR_OPTIONS_HPP
