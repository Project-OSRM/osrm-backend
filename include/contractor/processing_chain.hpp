/*

Copyright (c) 2014, Project OSRM, Dennis Luxen, others
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

#ifndef PROCESSING_CHAIN_HPP
#define PROCESSING_CHAIN_HPP

#include "contractor.hpp"
#include "contractor_options.hpp"
#include "../data_structures/query_edge.hpp"
#include "../data_structures/static_graph.hpp"
#include "../data_structures/deallocating_vector.hpp"
#include "../data_structures/node_based_graph.hpp"

struct SpeedProfileProperties;
struct EdgeBasedNode;
struct lua_State;

#include <boost/filesystem.hpp>

#include <vector>

/**
    \brief class of 'prepare' utility.
 */
class Prepare
{
  public:
    using EdgeData = QueryEdge::EdgeData;

    explicit Prepare(ContractorConfig contractor_config) : config(std::move(contractor_config)) {}
    Prepare(const Prepare &) = delete;
    ~Prepare();

    int Run();

  protected:
    void ContractGraph(const unsigned max_edge_id,
                       DeallocatingVector<EdgeBasedEdge> &edge_based_edge_list,
                       DeallocatingVector<QueryEdge> &contracted_edge_list,
                       std::vector<bool> &is_core_node,
                       std::vector<float> &node_levels) const;
    void WriteCoreNodeMarker(std::vector<bool> &&is_core_node) const;
    void WriteNodeLevels(std::vector<float> &&node_levels) const;
    void ReadNodeLevels(std::vector<float> &contraction_order) const;
    std::size_t WriteContractedGraph(unsigned number_of_edge_based_nodes,
                                     const DeallocatingVector<QueryEdge> &contracted_edge_list);
    void FindComponents(unsigned max_edge_id,
                        const DeallocatingVector<EdgeBasedEdge> &edges,
                        std::vector<EdgeBasedNode> &nodes) const;

  private:
    ContractorConfig config;
    std::size_t LoadEdgeExpandedGraph(const std::string &edge_based_graph_path,
                                      DeallocatingVector<EdgeBasedEdge> &edge_based_edge_list,
                                      const std::string &edge_segment_lookup_path,
                                      const std::string &edge_penalty_path,
                                      const std::string &segment_speed_path);
};

#endif // PROCESSING_CHAIN_HPP
