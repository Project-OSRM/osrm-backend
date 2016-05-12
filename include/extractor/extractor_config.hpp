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

#ifndef EXTRACTOR_CONFIG_HPP
#define EXTRACTOR_CONFIG_HPP

#include <boost/filesystem/path.hpp>

#include <array>
#include <string>

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
        turn_lane_descriptions_file_name = basepath + ".osrm.tls";
        turn_lane_data_file_name = basepath + ".osrm.tld";
        timestamp_file_name = basepath + ".osrm.timestamp";
        geometry_output_path = basepath + ".osrm.geometry";
        node_output_path = basepath + ".osrm.nodes";
        edge_output_path = basepath + ".osrm.edges";
        edge_graph_output_path = basepath + ".osrm.ebg";
        rtree_nodes_output_path = basepath + ".osrm.ramIndex";
        rtree_leafs_output_path = basepath + ".osrm.fileIndex";
        edge_segment_lookup_path = basepath + ".osrm.edge_segment_lookup";
        turn_duration_penalties_path = basepath + ".osrm.turn_duration_penalties";
        turn_weight_penalties_path = basepath + ".osrm.turn_weight_penalties";
        turn_penalties_index_path = basepath + ".osrm.turn_penalties_index";
        edge_based_node_weights_output_path = basepath + ".osrm.enw";
        profile_properties_output_path = basepath + ".osrm.properties";
        intersection_class_data_output_path = basepath + ".osrm.icd";
    }

    boost::filesystem::path input_path;
    boost::filesystem::path profile_path;

    std::string output_file_name;
    std::string restriction_file_name;
    std::string names_file_name;
    std::string turn_lane_data_file_name;
    std::string turn_lane_descriptions_file_name;
    std::string timestamp_file_name;
    std::string geometry_output_path;
    std::string edge_output_path;
    std::string edge_graph_output_path;
    std::string edge_based_node_weights_output_path;
    std::string node_output_path;
    std::string rtree_nodes_output_path;
    std::string rtree_leafs_output_path;
    std::string profile_properties_output_path;
    std::string intersection_class_data_output_path;
    std::string turn_weight_penalties_path;
    std::string turn_duration_penalties_path;

    unsigned requested_num_threads;
    unsigned small_component_size;

    bool generate_edge_lookup;
    std::string turn_penalties_index_path;
    std::string edge_segment_lookup_path;
};
}
}

#endif // EXTRACTOR_CONFIG_HPP
