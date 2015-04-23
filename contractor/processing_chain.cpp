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

#include <tbb/task_scheduler_init.h>
#include <tbb/parallel_sort.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

Prepare::Prepare() : requested_num_threads(1) {}

Prepare::~Prepare() {}

int Prepare::Process(int argc, char *argv[])
{
#ifdef WIN32
#pragma message("Memory consumption on Windows can be higher due to different bit packing")
#else
    static_assert(sizeof(ImportEdge) == 20,
                  "changing ImportEdge type has influence on memory consumption!");
    static_assert(sizeof(EdgeBasedEdge) == 16,
                  "changing EdgeBasedEdge type has influence on memory consumption!");
#endif

    LogPolicy::GetInstance().Unmute();
    TIMER_START(preparing);

    if (!ParseArguments(argc, argv))
    {
        return 0;
    }
    if (!boost::filesystem::is_regular_file(osrm_input_path))
    {
        SimpleLogger().Write(logWARNING) << "Input file " << osrm_input_path.string() << " not found!";
        return 1;
    }

    if (!boost::filesystem::is_regular_file(profile_path))
    {
        SimpleLogger().Write(logWARNING) << "Profile " << profile_path.string() << " not found!";
        return 1;
    }

    if (1 > requested_num_threads)
    {
        SimpleLogger().Write(logWARNING) << "Number of threads must be 1 or larger";
        return 1;
    }

    const unsigned recommended_num_threads = tbb::task_scheduler_init::default_num_threads();

    SimpleLogger().Write() << "Input file: " << osrm_input_path.filename().string();
    SimpleLogger().Write() << "Restrictions file: " << restrictions_path.filename().string();
    SimpleLogger().Write() << "Profile: " << profile_path.filename().string();
    SimpleLogger().Write() << "Threads: " << requested_num_threads;
    if (recommended_num_threads != requested_num_threads)
    {
        SimpleLogger().Write(logWARNING) << "The recommended number of threads is "
                                         << recommended_num_threads
                                         << "! This setting may have performance side-effects.";
    }

    tbb::task_scheduler_init init(requested_num_threads);

    LogPolicy::GetInstance().Unmute();

    node_output_path = osrm_input_path.string() + ".nodes";
    edge_output_path = osrm_input_path.string() + ".edges";
    geometry_output_path = osrm_input_path.string() + ".geometry";
    graph_output_path = osrm_input_path.string() + ".hsgr";
    rtree_nodes_output_path = osrm_input_path.string() + ".ramIndex";
    rtree_leafs_output_path = osrm_input_path.string() + ".fileIndex";

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

    FingerPrint fingerprint_orig;
    boost::filesystem::ofstream hsgr_output_stream(graph_output_path, std::ios::binary);
    hsgr_output_stream.write((char *)&fingerprint_orig, sizeof(FingerPrint));
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
        BOOST_ASSERT(contracted_edge_list->(edge).source != contracted_edge_list->(edge).target);
        current_edge.target = (*contracted_edge_list)[edge].target;
        current_edge.data = (*contracted_edge_list)[edge].data;

        // every target needs to be valid
        BOOST_ASSERT(current_edge.target < max_used_node_id);
#ifndef NDEBUG
        if (current_edge.data.distance <= 0)
        {
            SimpleLogger().Write(logWARNING) << "Edge: " << edge
                                             << ",source: " << contracted_edge_list->at(edge).source
                                             << ", target: " << contracted_edge_list->at(edge).target
                                             << ", dist: " << current_edge.data.distance;

            SimpleLogger().Write(logWARNING) << "Failed at adjacency list of node "
                                             << contracted_edge_list->at(edge).source << "/"
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
 \brief Parses command line arguments
 \param argc count of arguments
 \param argv array of arguments
 \param result [out] value for exit return value
 \return true if everything is ok, false if need to terminate execution
*/
bool Prepare::ParseArguments(int argc, char *argv[])
{
    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()("version,v", "Show version")("help,h", "Show this help message")(
        "config,c", boost::program_options::value<boost::filesystem::path>(&config_file_path)
                        ->default_value("contractor.ini"),
        "Path to a configuration file.");

    // declare a group of options that will be allowed both on command line and in config file
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options()(
        "restrictions,r",
        boost::program_options::value<boost::filesystem::path>(&restrictions_path),
        "Restrictions file in .osrm.restrictions format")(
        "profile,p", boost::program_options::value<boost::filesystem::path>(&profile_path)
                         ->default_value("profile.lua"),
        "Path to LUA routing profile")(
        "threads,t", boost::program_options::value<unsigned int>(&requested_num_threads)
                         ->default_value(tbb::task_scheduler_init::default_num_threads()),
        "Number of threads to use");

    // hidden options, will be allowed both on command line and in config file, but will not be
    // shown to the user
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()(
        "input,i", boost::program_options::value<boost::filesystem::path>(&osrm_input_path),
        "Input file in .osm, .osm.bz2 or .osm.pbf format");

    // positional option
    boost::program_options::positional_options_description positional_options;
    positional_options.add("input", 1);

    // combine above options for parsing
    boost::program_options::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_options).add(hidden_options);

    boost::program_options::options_description config_file_options;
    config_file_options.add(config_options).add(hidden_options);

    boost::program_options::options_description visible_options(
        "Usage: " + boost::filesystem::basename(argv[0]) + " <input.osrm> [options]");
    visible_options.add(generic_options).add(config_options);

    // parse command line options
    boost::program_options::variables_map option_variables;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
                                      .options(cmdline_options)
                                      .positional(positional_options)
                                      .run(),
                                  option_variables);

    const auto &temp_config_path = option_variables["config"].as<boost::filesystem::path>();
    if (boost::filesystem::is_regular_file(temp_config_path))
    {
        boost::program_options::store(boost::program_options::parse_config_file<char>(
                                          temp_config_path.string().c_str(), cmdline_options, true),
                                      option_variables);
    }

    if (option_variables.count("version"))
    {
        SimpleLogger().Write() << g_GIT_DESCRIPTION;
        return false;
    }

    if (option_variables.count("help"))
    {
        SimpleLogger().Write() << "\n" << visible_options;
        return false;
    }

    boost::program_options::notify(option_variables);

    if (!option_variables.count("restrictions"))
    {
        restrictions_path = std::string(osrm_input_path.string() + ".restrictions");
    }

    if (!option_variables.count("input"))
    {
        SimpleLogger().Write() << "\n" << visible_options;
        return false;
    }

    return true;
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
    luaAddScriptFolderToLoadPath(lua_state, profile_path.string().c_str());

    // Now call our function in a lua script
    if (0 != luaL_dofile(lua_state, profile_path.string().c_str()))
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
void Prepare::LoadRestrictionMap(const std::unordered_map<NodeID, NodeID> &external_to_internal_node_map,
                                 RestrictionMap &restriction_map)
{
    boost::filesystem::ifstream input_stream(restrictions_path, std::ios::in | std::ios::binary);

    std::vector<TurnRestriction> restriction_list;
    loadRestrictionsFromFile(input_stream, external_to_internal_node_map, restriction_list);

    SimpleLogger().Write() << " - " << restriction_list.size() << " restrictions.";

    restriction_map = RestrictionMap(restriction_list);
}

/**
  \brief Build load node based graph from .osrm file and restrictions from .restrictions file
  */
std::shared_ptr<NodeBasedDynamicGraph>
Prepare::LoadNodeBasedGraph(std::vector<NodeID> &barrier_node_list,
                            std::vector<NodeID> &traffic_light_list,
                            RestrictionMap &restriction_map,
                            std::vector<QueryNode>& internal_to_external_node_map)
{
    std::vector<ImportEdge> edge_list;
    std::unordered_map<NodeID, NodeID> external_to_internal_node_map;

    boost::filesystem::ifstream input_stream(osrm_input_path, std::ios::in | std::ios::binary);

    NodeID number_of_node_based_nodes = loadNodesFromFile(input_stream,
                                            barrier_node_list, traffic_light_list,
                                            internal_to_external_node_map,
                                            external_to_internal_node_map);

    SimpleLogger().Write() << " - " << barrier_node_list.size() << " bollard nodes, "
                           << traffic_light_list.size() << " traffic lights";

    loadEdgesFromFile(input_stream, external_to_internal_node_map, edge_list);

    LoadRestrictionMap(external_to_internal_node_map, restriction_map);

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
    auto restriction_map = std::make_shared<RestrictionMap>();

    auto node_based_graph = LoadNodeBasedGraph(*barrier_node_list, *traffic_light_list, *restriction_map, internal_to_external_node_map);

    const std::size_t number_of_node_based_nodes = node_based_graph->GetNumberOfNodes();

    EdgeBasedGraphFactory edge_based_graph_factory(node_based_graph,
                                                   restriction_map,
                                                   std::move(barrier_node_list),
                                                   std::move(traffic_light_list),
                                                   internal_to_external_node_map,
                                                   speed_profile);

    edge_based_graph_factory.Run(edge_output_path, geometry_output_path, lua_state);
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
    boost::filesystem::ofstream node_stream(node_output_path, std::ios::binary);
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
    StaticRTree<EdgeBasedNode>(node_based_edge_list, rtree_nodes_output_path.c_str(),
                               rtree_leafs_output_path.c_str(), internal_to_external_node_map);
}
