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
#include "util/guidance/turn_lanes.hpp"

#include "util/typedefs.hpp"

namespace osrm
{
namespace extractor
{

class ScriptingEnvironment;
struct ProfileProperties;

class Extractor
{
  public:
    Extractor(ExtractorConfig extractor_config) : config(std::move(extractor_config)) {}
    int run(ScriptingEnvironment &scripting_environment);

  private:
    ExtractorConfig config;

    std::tuple<guidance::LaneDescriptionMap,
               std::vector<TurnRestriction>,
               std::vector<ConditionalTurnRestriction>>
    ParseOSMData(ScriptingEnvironment &scripting_environment, const unsigned number_of_threads);

    std::pair<std::size_t, EdgeID>
    BuildEdgeExpandedGraph(ScriptingEnvironment &scripting_environment,
                           std::vector<util::Coordinate> &coordinates,
                           extractor::PackedOSMIDs &osm_node_ids,
                           EdgeBasedNodeDataContainer &edge_based_nodes_container,
                           std::vector<EdgeBasedNodeSegment> &edge_based_node_segments,
                           std::vector<bool> &node_is_startpoint,
                           std::vector<EdgeWeight> &edge_based_node_weights,
                           util::DeallocatingVector<EdgeBasedEdge> &edge_based_edge_list,
                           const std::string &intersection_class_output_file,
                           std::vector<TurnRestriction> &turn_restrictions,
                           std::vector<ConditionalTurnRestriction> &conditional_turn_restrictions,
                           guidance::LaneDescriptionMap &turn_lane_map);
    void FindComponents(unsigned max_edge_id,
                        const util::DeallocatingVector<EdgeBasedEdge> &input_edge_list,
                        const std::vector<EdgeBasedNodeSegment> &input_node_segments,
                        EdgeBasedNodeDataContainer &nodes_container) const;
    void BuildRTree(std::vector<EdgeBasedNodeSegment> edge_based_node_segments,
                    std::vector<bool> node_is_startpoint,
                    const std::vector<util::Coordinate> &coordinates);
    std::shared_ptr<RestrictionMap> LoadRestrictionMap();
    std::shared_ptr<util::NodeBasedDynamicGraph>
    LoadNodeBasedGraph(std::unordered_set<NodeID> &barrier_nodes,
                       std::unordered_set<NodeID> &traffic_lights,
                       std::vector<util::Coordinate> &coordinates,
                       extractor::PackedOSMIDs &osm_node_ids);

    // Writes compressed node based graph and its embedding into a file for osrm-partition to use.
    static void WriteCompressedNodeBasedGraph(const std::string &path,
                                              const util::NodeBasedDynamicGraph &graph,
                                              const std::vector<util::Coordinate> &coordiantes);

    void WriteConditionalRestrictions(
        const std::string &path,
        std::vector<ConditionalTurnRestriction> &conditional_turn_restrictions);
};
}
}

#endif /* EXTRACTOR_HPP */
