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

#ifndef EXTRACTOR_HPP
#define EXTRACTOR_HPP

#include "extractor/edge_based_edge.hpp"
#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/extractor_config.hpp"
#include "extractor/graph_compressor.hpp"

#include "util/guidance/bearing_class.hpp"
#include "util/guidance/entry_class.hpp"

#include "util/typedefs.hpp"

namespace osrm
{
namespace extractor
{

struct ProfileProperties;

class Extractor
{
  public:
    Extractor(ExtractorConfig extractor_config) : config(std::move(extractor_config)) {}
    int run();

  private:
    ExtractorConfig config;

    std::pair<std::size_t, std::size_t>
    BuildEdgeExpandedGraph(lua_State *lua_state,
                           const ProfileProperties &profile_properties,
                           std::vector<QueryNode> &internal_to_external_node_map,
                           std::vector<EdgeBasedNode> &node_based_edge_list,
                           std::vector<bool> &node_is_startpoint,
                           std::vector<EdgeWeight> &edge_based_node_weights,
                           util::DeallocatingVector<EdgeBasedEdge> &edge_based_edge_list,
                           const std::string &intersection_class_output_file);
    void WriteProfileProperties(const std::string &output_path,
                                const ProfileProperties &properties) const;
    void WriteNodeMapping(const std::vector<QueryNode> &internal_to_external_node_map);
    void FindComponents(unsigned max_edge_id,
                        const util::DeallocatingVector<EdgeBasedEdge> &edges,
                        std::vector<EdgeBasedNode> &nodes) const;
    void BuildRTree(std::vector<EdgeBasedNode> node_based_edge_list,
                    std::vector<bool> node_is_startpoint,
                    const std::vector<QueryNode> &internal_to_external_node_map);
    std::shared_ptr<RestrictionMap> LoadRestrictionMap();
    std::shared_ptr<util::NodeBasedDynamicGraph>
    LoadNodeBasedGraph(std::unordered_set<NodeID> &barrier_nodes,
                       std::unordered_set<NodeID> &traffic_lights,
                       std::vector<QueryNode> &internal_to_external_node_map);

    void WriteEdgeBasedGraph(const std::string &output_file_filename,
                             const size_t max_edge_id,
                             util::DeallocatingVector<EdgeBasedEdge> const &edge_based_edge_list);

    void WriteIntersectionClassificationData(
        const std::string &output_file_name,
        const std::vector<std::uint32_t> &node_based_intersection_classes,
        const std::vector<util::guidance::BearingClass> &bearing_classes,
        const std::vector<util::guidance::EntryClass> &entry_classes) const;
};
}
}

#endif /* EXTRACTOR_HPP */
