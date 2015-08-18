/*

Copyright (c) 2015, Project OSRM contributors
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

#ifndef EXTRACTOR_HPP
#define EXTRACTOR_HPP

#include "extractor_options.hpp"
#include "edge_based_graph_factory.hpp"
#include "../algorithms/graph_compressor.hpp"

class extractor
{
public:
  extractor(ExtractorConfig extractor_config) : config(std::move(extractor_config)) {}
    int run();
private:
    ExtractorConfig config;
    void SetupScriptingEnvironment(lua_State *myLuaState,
                                   SpeedProfileProperties &speed_profile);
    unsigned CalculateEdgeChecksum(std::unique_ptr<std::vector<EdgeBasedNode>> node_based_edge_list);
    std::pair<std::size_t, std::size_t>
    BuildEdgeExpandedGraph(std::vector<QueryNode> &internal_to_external_node_map,
                                       std::vector<EdgeBasedNode> &node_based_edge_list,
                                       DeallocatingVector<EdgeBasedEdge> &edge_based_edge_list);
    void WriteNodeMapping(std::unique_ptr<std::vector<QueryNode>> internal_to_external_node_map);
    void FindComponents(unsigned max_edge_id, const DeallocatingVector<EdgeBasedEdge>& edges, std::vector<EdgeBasedNode>& nodes) const;
    void BuildRTree(const std::vector<EdgeBasedNode> &node_based_edge_list,
                    const std::vector<QueryNode> &internal_to_external_node_map);
    std::shared_ptr<RestrictionMap> LoadRestrictionMap();
    std::shared_ptr<NodeBasedDynamicGraph>
    LoadNodeBasedGraph(std::unordered_set<NodeID> &barrier_nodes,
                       std::unordered_set<NodeID> &traffic_lights,
                       std::vector<QueryNode>& internal_to_external_node_map);

    void WriteEdgeBasedGraph(std::string const &output_file_filename, 
                             size_t const& max_edge_id, 
                             const unsigned edges_crc32,
                             DeallocatingVector<EdgeBasedEdge> const & edge_based_edge_list);


 
   
};
#endif /* EXTRACTOR_HPP */
