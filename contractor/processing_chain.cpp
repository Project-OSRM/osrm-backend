/*

Copyright (c) 2015, Project OSRM, Dennis Luxen, others
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

#include "processing_chain.hpp"

#include "contractor.hpp"

#include "../algorithms/crc32_processor.hpp"
#include "../data_structures/deallocating_vector.hpp"
#include "../data_structures/static_rtree.hpp"
#include "../data_structures/restriction_map.hpp"

#include "../util/git_sha.hpp"
#include "../util/graph_loader.hpp"
#include "../util/integer_range.hpp"
#include "../util/lua_util.hpp"
#include "../util/make_unique.hpp"
#include "../util/osrm_exception.hpp"
#include "../util/simple_logger.hpp"
#include "../util/string_util.hpp"
#include "../util/timing_util.hpp"
#include "../typedefs.h"

#include <boost/filesystem/fstream.hpp>
#include <boost/program_options.hpp>

#include <tbb/parallel_sort.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

Prepare::~Prepare() {}

int Prepare::Run()
{
#ifdef WIN32
#pragma message("Memory consumption on Windows can be higher due to different bit packing")
#else
    static_assert(sizeof(NodeBasedEdge) == 20,
                  "changing NodeBasedEdge type has influence on memory consumption!");
    static_assert(sizeof(EdgeBasedEdge) == 16,
                  "changing EdgeBasedEdge type has influence on memory consumption!");
#endif

    TIMER_START(preparing);

    // Create a new lua state

    SimpleLogger().Write() << "Generating edge-expanded graph representation";

    TIMER_START(expansion);

    auto node_based_edge_list = osrm::make_unique<std::vector<EdgeBasedNode>>();;
    DeallocatingVector<EdgeBasedEdge> edge_based_edge_list;
    auto internal_to_external_node_map = osrm::make_unique<std::vector<QueryNode>>();
    auto graph_size =
        BuildEdgeExpandedGraph(*internal_to_external_node_map,
                               *node_based_edge_list, edge_based_edge_list);

    auto number_of_node_based_nodes = graph_size.first;
    auto number_of_edge_based_nodes = graph_size.second;

    TIMER_STOP(expansion);

    SimpleLogger().Write() << "building r-tree ...";
    TIMER_START(rtree);

    BuildRTree(*node_based_edge_list, *internal_to_external_node_map);

    TIMER_STOP(rtree);

    SimpleLogger().Write() << "writing node map ...";
    WriteNodeMapping(std::move(internal_to_external_node_map));

    // Contracting the edge-expanded graph

    TIMER_START(contraction);
    auto contracted_edge_list = osrm::make_unique<DeallocatingVector<QueryEdge>>();
    ContractGraph(number_of_edge_based_nodes, edge_based_edge_list, *contracted_edge_list);
    TIMER_STOP(contraction);

    SimpleLogger().Write() << "Contraction took " << TIMER_SEC(contraction) << " sec";

    std::size_t number_of_used_edges = WriteContractedGraph(number_of_edge_based_nodes,
                                                            std::move(node_based_edge_list),
                                                            std::move(contracted_edge_list));

    TIMER_STOP(preparing);

    SimpleLogger().Write() << "Preprocessing : " << TIMER_SEC(preparing) << " seconds";
    SimpleLogger().Write() << "Expansion  : " << (number_of_node_based_nodes / TIMER_SEC(expansion))
                           << " nodes/sec and "
                           << (number_of_edge_based_nodes / TIMER_SEC(expansion)) << " edges/sec";

    SimpleLogger().Write() << "Contraction: "
                           << (number_of_edge_based_nodes / TIMER_SEC(contraction))
                           << " nodes/sec and " << number_of_used_edges / TIMER_SEC(contraction)
                           << " edges/sec";

    SimpleLogger().Write() << "finished preprocessing";

    return 0;
}

std::size_t Prepare::WriteContractedGraph(unsigned number_of_edge_based_nodes,
                                          std::unique_ptr<std::vector<EdgeBasedNode>> node_based_edge_list,
                                          std::unique_ptr<DeallocatingVector<QueryEdge>> contracted_edge_list)
{
    const unsigned crc32_value = CalculateEdgeChecksum(std::move(node_based_edge_list));

    // Sorting contracted edges in a way that the static query graph can read some in in-place.
    tbb::parallel_sort(contracted_edge_list->begin(), contracted_edge_list->end());
    const unsigned contracted_edge_count = contracted_edge_list->size();
    SimpleLogger().Write() << "Serializing compacted graph of " << contracted_edge_count
                           << " edges";

    const FingerPrint fingerprint = FingerPrint::GetValid();
    boost::filesystem::ofstream hsgr_output_stream(config.graph_output_path, std::ios::binary);
    hsgr_output_stream.write((char *)&fingerprint, sizeof(FingerPrint));
    const unsigned max_used_node_id = 1 + [&contracted_edge_list]
    {
        unsigned tmp_max = 0;
        for (const QueryEdge &edge : *contracted_edge_list)
        {
            BOOST_ASSERT(SPECIAL_NODEID != edge.source);
            BOOST_ASSERT(SPECIAL_NODEID != edge.target);
            tmp_max = std::max(tmp_max, edge.source);
            tmp_max = std::max(tmp_max, edge.target);
        }
        return tmp_max;
    }();

    SimpleLogger().Write(logDEBUG) << "input graph has " << number_of_edge_based_nodes << " nodes";
    SimpleLogger().Write(logDEBUG) << "contracted graph has " << max_used_node_id << " nodes";

    std::vector<StaticGraph<EdgeData>::NodeArrayEntry> node_array;
    node_array.resize(number_of_edge_based_nodes + 1);

    SimpleLogger().Write() << "Building node array";
    StaticGraph<EdgeData>::EdgeIterator edge = 0;
    StaticGraph<EdgeData>::EdgeIterator position = 0;
    StaticGraph<EdgeData>::EdgeIterator last_edge = edge;

    // initializing 'first_edge'-field of nodes:
    for (const auto node : osrm::irange(0u, max_used_node_id))
    {
        last_edge = edge;
        while ((edge < contracted_edge_count) && ((*contracted_edge_list)[edge].source == node))
        {
            ++edge;
        }
        node_array[node].first_edge = position; //=edge
        position += edge - last_edge;           // remove
    }

    for (const auto sentinel_counter : osrm::irange<unsigned>(max_used_node_id, node_array.size()))
    {
        // sentinel element, guarded against underflow
        node_array[sentinel_counter].first_edge = contracted_edge_count;
    }

    SimpleLogger().Write() << "Serializing node array";

    const unsigned node_array_size = node_array.size();
    // serialize crc32, aka checksum
    hsgr_output_stream.write((char *)&crc32_value, sizeof(unsigned));
    // serialize number of nodes
    hsgr_output_stream.write((char *)&node_array_size, sizeof(unsigned));
    // serialize number of edges
    hsgr_output_stream.write((char *)&contracted_edge_count, sizeof(unsigned));
    // serialize all nodes
    if (node_array_size > 0)
    {
        hsgr_output_stream.write((char *)&node_array[0],
                                 sizeof(StaticGraph<EdgeData>::NodeArrayEntry) * node_array_size);
    }

    // serialize all edges
    SimpleLogger().Write() << "Building edge array";
    edge = 0;
    int number_of_used_edges = 0;

    StaticGraph<EdgeData>::EdgeArrayEntry current_edge;
    for (const auto edge : osrm::irange<std::size_t>(0, contracted_edge_list->size()))
    {
        // no eigen loops
        BOOST_ASSERT((*contracted_edge_list)[edge].source != (*contracted_edge_list)[edge].target);
        current_edge.target = (*contracted_edge_list)[edge].target;
        current_edge.data = (*contracted_edge_list)[edge].data;

        // every target needs to be valid
        BOOST_ASSERT(current_edge.target < max_used_node_id);
#ifndef NDEBUG
        if (current_edge.data.distance <= 0)
        {
            SimpleLogger().Write(logWARNING) << "Edge: " << edge
                                             << ",source: " << (*contracted_edge_list)[edge].source
                                             << ", target: " << (*contracted_edge_list)[edge].target
                                             << ", dist: " << current_edge.data.distance;

            SimpleLogger().Write(logWARNING) << "Failed at adjacency list of node "
                                             << (*contracted_edge_list)[edge].source << "/"
                                             << node_array.size() - 1;
            return 1;
        }
#endif
        hsgr_output_stream.write((char *)&current_edge,
                                 sizeof(StaticGraph<EdgeData>::EdgeArrayEntry));

        ++number_of_used_edges;
    }

    return number_of_used_edges;
}

unsigned Prepare::CalculateEdgeChecksum(std::unique_ptr<std::vector<EdgeBasedNode>> node_based_edge_list)
{
    RangebasedCRC32 crc32;
    if (crc32.using_hardware())
    {
        SimpleLogger().Write() << "using hardware based CRC32 computation";
    }
    else
    {
        SimpleLogger().Write() << "using software based CRC32 computation";
    }

    const unsigned crc32_value = crc32(*node_based_edge_list);
    SimpleLogger().Write() << "CRC32: " << crc32_value;

    return crc32_value;
}

/**
    \brief Setups scripting environment (lua-scripting)
    Also initializes speed profile.
*/
void Prepare::SetupScriptingEnvironment(
    lua_State *lua_state, EdgeBasedGraphFactory::SpeedProfileProperties &speed_profile)
{
    // open utility libraries string library;
    luaL_openlibs(lua_state);

    // adjust lua load path
    luaAddScriptFolderToLoadPath(lua_state, config.profile_path.string().c_str());

    // Now call our function in a lua script
    if (0 != luaL_dofile(lua_state, config.profile_path.string().c_str()))
    {
        std::stringstream msg;
        msg << lua_tostring(lua_state, -1) << " occured in scripting block";
        throw osrm::exception(msg.str());
    }

    if (0 != luaL_dostring(lua_state, "return traffic_signal_penalty\n"))
    {
        std::stringstream msg;
        msg << lua_tostring(lua_state, -1) << " occured in scripting block";
        throw osrm::exception(msg.str());
    }
    speed_profile.traffic_signal_penalty = 10 * lua_tointeger(lua_state, -1);
    SimpleLogger().Write(logDEBUG)
        << "traffic_signal_penalty: " << speed_profile.traffic_signal_penalty;

    if (0 != luaL_dostring(lua_state, "return u_turn_penalty\n"))
    {
        std::stringstream msg;
        msg << lua_tostring(lua_state, -1) << " occured in scripting block";
        throw osrm::exception(msg.str());
    }

    speed_profile.u_turn_penalty = 10 * lua_tointeger(lua_state, -1);
    speed_profile.has_turn_penalty_function = lua_function_exists(lua_state, "turn_function");
}

/**
  \brief Build load restrictions from .restriction file
  */
std::shared_ptr<RestrictionMap> Prepare::LoadRestrictionMap()
{
    boost::filesystem::ifstream input_stream(config.restrictions_path, std::ios::in | std::ios::binary);

    std::vector<TurnRestriction> restriction_list;
    loadRestrictionsFromFile(input_stream, restriction_list);

    SimpleLogger().Write() << " - " << restriction_list.size() << " restrictions.";

    return std::make_shared<RestrictionMap>(restriction_list);
}

/**
  \brief Load node based graph from .osrm file
  */
std::shared_ptr<NodeBasedDynamicGraph>
Prepare::LoadNodeBasedGraph(std::vector<NodeID> &barrier_node_list,
                            std::vector<NodeID> &traffic_light_list,
                            std::vector<QueryNode>& internal_to_external_node_map)
{
    std::vector<NodeBasedEdge> edge_list;

    boost::filesystem::ifstream input_stream(config.osrm_input_path, std::ios::in | std::ios::binary);

    NodeID number_of_node_based_nodes = loadNodesFromFile(input_stream,
                                            barrier_node_list, traffic_light_list,
                                            internal_to_external_node_map);

    SimpleLogger().Write() << " - " << barrier_node_list.size() << " bollard nodes, "
                           << traffic_light_list.size() << " traffic lights";

    loadEdgesFromFile(input_stream, edge_list);

    if (edge_list.empty())
    {
        SimpleLogger().Write(logWARNING) << "The input data is empty, exiting.";
        return std::shared_ptr<NodeBasedDynamicGraph>();
    }

    return NodeBasedDynamicGraphFromImportEdges(number_of_node_based_nodes, edge_list);
}


/**
 \brief Building an edge-expanded graph from node-based input and turn restrictions
*/
std::pair<std::size_t, std::size_t>
Prepare::BuildEdgeExpandedGraph(std::vector<QueryNode> &internal_to_external_node_map,
                                std::vector<EdgeBasedNode> &node_based_edge_list,
                                DeallocatingVector<EdgeBasedEdge> &edge_based_edge_list)
{
    lua_State *lua_state = luaL_newstate();
    luabind::open(lua_state);

    EdgeBasedGraphFactory::SpeedProfileProperties speed_profile;

    SetupScriptingEnvironment(lua_state, speed_profile);

    auto barrier_node_list = osrm::make_unique<std::vector<NodeID>>();
    auto traffic_light_list = osrm::make_unique<std::vector<NodeID>>();

    auto restriction_map = LoadRestrictionMap();
    auto node_based_graph = LoadNodeBasedGraph(*barrier_node_list, *traffic_light_list, internal_to_external_node_map);

    const std::size_t number_of_node_based_nodes = node_based_graph->GetNumberOfNodes();

    EdgeBasedGraphFactory edge_based_graph_factory(node_based_graph,
                                                   restriction_map,
                                                   std::move(barrier_node_list),
                                                   std::move(traffic_light_list),
                                                   internal_to_external_node_map,
                                                   speed_profile);

    edge_based_graph_factory.Run(config.edge_output_path, config.geometry_output_path, lua_state);
    lua_close(lua_state);

    const std::size_t number_of_edge_based_nodes =
        edge_based_graph_factory.GetNumberOfEdgeBasedNodes();

    BOOST_ASSERT(number_of_edge_based_nodes != std::numeric_limits<unsigned>::max());

    edge_based_graph_factory.GetEdgeBasedEdges(edge_based_edge_list);
    edge_based_graph_factory.GetEdgeBasedNodes(node_based_edge_list);

    return std::make_pair(number_of_node_based_nodes, number_of_edge_based_nodes);
}

/**
 \brief Build contracted graph.
 */
void Prepare::ContractGraph(const std::size_t number_of_edge_based_nodes,
                            DeallocatingVector<EdgeBasedEdge>& edge_based_edge_list,
                            DeallocatingVector<QueryEdge>& contracted_edge_list)
{
    Contractor contractor(number_of_edge_based_nodes, edge_based_edge_list);
    contractor.Run();
    contractor.GetEdges(contracted_edge_list);
}

/**
  \brief Writing info on original (node-based) nodes
 */
void Prepare::WriteNodeMapping(std::unique_ptr<std::vector<QueryNode>> internal_to_external_node_map)
{
    boost::filesystem::ofstream node_stream(config.node_output_path, std::ios::binary);
    const unsigned size_of_mapping = internal_to_external_node_map->size();
    node_stream.write((char *)&size_of_mapping, sizeof(unsigned));
    if (size_of_mapping > 0)
    {
        node_stream.write((char *) internal_to_external_node_map->data(),
                          size_of_mapping * sizeof(QueryNode));
    }
    node_stream.close();
}

/**
    \brief Building rtree-based nearest-neighbor data structure

    Saves tree into '.ramIndex' and leaves into '.fileIndex'.
 */
void Prepare::BuildRTree(const std::vector<EdgeBasedNode> &node_based_edge_list, const std::vector<QueryNode>& internal_to_external_node_map)
{
    StaticRTree<EdgeBasedNode>(node_based_edge_list, config.rtree_nodes_output_path.c_str(),
                               config.rtree_leafs_output_path.c_str(), internal_to_external_node_map);
}
