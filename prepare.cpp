/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
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

#include "Algorithms/IteratorBasedCRC32.h"
#include "Contractor/Contractor.h"
#include "Contractor/EdgeBasedGraphFactory.h"
#include "DataStructures/BinaryHeap.h"
#include "DataStructures/DeallocatingVector.h"
#include "DataStructures/QueryEdge.h"
#include "DataStructures/StaticGraph.h"
#include "DataStructures/StaticRTree.h"
#include "DataStructures/RestrictionMap.h"
#include "Util/GitDescription.h"
#include "Util/GraphLoader.h"
#include "Util/LuaUtil.h"
#include "Util/OSRMException.h"

#include "Util/SimpleLogger.h"
#include "Util/StringUtil.h"
#include "Util/TimingUtil.h"
#include "typedefs.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <luabind/luabind.hpp>

#include <thread>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <tbb/task_scheduler_init.h>
#include <tbb/parallel_sort.h>

typedef QueryEdge::EdgeData EdgeData;
typedef DynamicGraph<EdgeData>::InputEdge InputEdge;
typedef StaticGraph<EdgeData>::InputEdge StaticEdge;

std::vector<NodeInfo> internal_to_external_node_map;
std::vector<TurnRestriction> restriction_list;
std::vector<NodeID> barrier_node_list;
std::vector<NodeID> traffic_light_list;
std::vector<ImportEdge> edge_list;

int main(int argc, char *argv[])
{
    try
    {
        LogPolicy::GetInstance().Unmute();
        TIMER_START(preparing);
        TIMER_START(expansion);

        boost::filesystem::path config_file_path, input_path, restrictions_path, profile_path;
        unsigned int requested_num_threads;
        bool use_elevation;

        // declare a group of options that will be allowed only on command line
        boost::program_options::options_description generic_options("Options");
        generic_options.add_options()("version,v", "Show version")("help,h",
                                                                   "Show this help message")(
            "config,c",
            boost::program_options::value<boost::filesystem::path>(&config_file_path)
                ->default_value("contractor.ini"),
            "Path to a configuration file.");

        // declare a group of options that will be allowed both on command line and in config file
        boost::program_options::options_description config_options("Configuration");
        config_options.add_options()(
            "restrictions,r",
            boost::program_options::value<boost::filesystem::path>(&restrictions_path),
            "Restrictions file in .osrm.restrictions format")(
            "profile,p",
            boost::program_options::value<boost::filesystem::path>(&profile_path)
                ->default_value("profile.lua"),"Path to LUA routing profile")(
            "elevation,e", boost::program_options::value<bool>(&use_elevation)->default_value(true),
                "Process node elevations")(
            "threads,t",
            boost::program_options::value<unsigned int>(&requested_num_threads)->default_value(tbb::task_scheduler_init::default_num_threads()),
            "Number of threads to use");

        // hidden options, will be allowed both on command line and in config file, but will not be
        // shown to the user
        boost::program_options::options_description hidden_options("Hidden options");
        hidden_options.add_options()(
            "input,i",
            boost::program_options::value<boost::filesystem::path>(&input_path),
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

        if (option_variables.count("version"))
        {
            SimpleLogger().Write() << g_GIT_DESCRIPTION;
            return 0;
        }

        if (option_variables.count("help"))
        {
            SimpleLogger().Write() << "\n" << visible_options;
            return 0;
        }

        boost::program_options::notify(option_variables);

        if (!option_variables.count("restrictions"))
        {
            restrictions_path = std::string(input_path.string() + ".restrictions");
        }

        if (!option_variables.count("input"))
        {
            SimpleLogger().Write() << "\n" << visible_options;
            return 0;
        }

        if (!boost::filesystem::is_regular_file(input_path))
        {
            SimpleLogger().Write(logWARNING) << "Input file " << input_path.string()
                                             << " not found!";
            return 1;
        }

        if (!boost::filesystem::is_regular_file(profile_path))
        {
            SimpleLogger().Write(logWARNING) << "Profile " << profile_path.string()
                                             << " not found!";
            return 1;
        }

        if (1 > requested_num_threads)
        {
            SimpleLogger().Write(logWARNING) << "Number of threads must be 1 or larger";
            return 1;
        }

        const unsigned recommended_num_threads = tbb::task_scheduler_init::default_num_threads();

        SimpleLogger().Write() << "Input file: " << input_path.filename().string();
        SimpleLogger().Write() << "Restrictions file: " << restrictions_path.filename().string();
        SimpleLogger().Write() << "Profile: " << profile_path.filename().string();
        SimpleLogger().Write() << "Using elevation: " << use_elevation;
        SimpleLogger().Write() << "Threads: " << requested_num_threads;
        if (recommended_num_threads != requested_num_threads)
        {
            SimpleLogger().Write(logWARNING) << "The recommended number of threads is "
                                             << recommended_num_threads
                                             << "! This setting may have performance side-effects.";
        }

        tbb::task_scheduler_init init(requested_num_threads);

        LogPolicy::GetInstance().Unmute();
        boost::filesystem::ifstream restriction_stream(restrictions_path, std::ios::binary);
        TurnRestriction restriction;
        FingerPrint fingerprint_loaded, fingerprint_orig;
        unsigned number_of_usable_restrictions = 0;
        restriction_stream.read((char *)&fingerprint_loaded, sizeof(FingerPrint));
        if (!fingerprint_loaded.TestPrepare(fingerprint_orig))
        {
            SimpleLogger().Write(logWARNING) << ".restrictions was prepared with different build.\n"
                                                "Reprocess to get rid of this warning.";
        }

        restriction_stream.read((char *)&number_of_usable_restrictions, sizeof(unsigned));
        restriction_list.resize(number_of_usable_restrictions);
        if (number_of_usable_restrictions > 0)
        {
            restriction_stream.read((char *)&(restriction_list[0]),
                                number_of_usable_restrictions * sizeof(TurnRestriction));
        }
        restriction_stream.close();

        boost::filesystem::ifstream in;
        in.open(input_path, std::ios::in | std::ios::binary);

        const std::string node_filename = input_path.string() + ".nodes";
        const std::string edge_out = input_path.string() + ".edges";
        const std::string geometry_filename = input_path.string() + ".geometry";
        const std::string graphOut = input_path.string() + ".hsgr";
        const std::string rtree_nodes_path = input_path.string() + ".ramIndex";
        const std::string rtree_leafs_path = input_path.string() + ".fileIndex";

        /*** Setup Scripting Environment ***/

        // Create a new lua state
        lua_State *lua_state = luaL_newstate();

        // Connect LuaBind to this lua state
        luabind::open(lua_state);

        // open utility libraries string library;
        luaL_openlibs(lua_state);

        // adjust lua load path
        luaAddScriptFolderToLoadPath(lua_state, profile_path.string().c_str());

        // Now call our function in a lua script
        if (0 != luaL_dofile(lua_state, profile_path.string().c_str()))
        {
            std::cerr << lua_tostring(lua_state, -1) << " occured in scripting block" << std::endl;
            return 1;
        }

        EdgeBasedGraphFactory::SpeedProfileProperties speed_profile;

        if (0 != luaL_dostring(lua_state, "return traffic_signal_penalty\n"))
        {
            std::cerr << lua_tostring(lua_state, -1) << " occured in scripting block" << std::endl;
            return 1;
        }
        speed_profile.trafficSignalPenalty = 10 * lua_tointeger(lua_state, -1);
        SimpleLogger().Write(logDEBUG)
            << "traffic_signal_penalty: " << speed_profile.trafficSignalPenalty;

        if (0 != luaL_dostring(lua_state, "return u_turn_penalty\n"))
        {
            std::cerr << lua_tostring(lua_state, -1) << " occured in scripting block" << std::endl;
            return 1;
        }
        speed_profile.uTurnPenalty = 10 * lua_tointeger(lua_state, -1);

        speed_profile.has_turn_penalty_function = lua_function_exists(lua_state, "turn_function");

        #ifdef WIN32
        #pragma message ("Memory consumption on Windows can be higher due to memory alignment")
        #else
        static_assert(sizeof(ImportEdge) == 20,
                      "changing ImportEdge type has influence on memory consumption!");
        #endif
        std::vector<ImportEdge> edge_list;
        NodeID number_of_node_based_nodes =
            readBinaryOSRMGraphFromStream(in,
                                          edge_list,
                                          barrier_node_list,
                                          traffic_light_list,
                                          &internal_to_external_node_map,
                                          restriction_list,
                                          use_elevation);
        in.close();

        if (edge_list.empty())
        {
            SimpleLogger().Write(logWARNING) << "The input data is empty, exiting.";
            return 1;
        }

        SimpleLogger().Write() << restriction_list.size() << " restrictions, "
                               << barrier_node_list.size() << " bollard nodes, "
                               << traffic_light_list.size() << " traffic lights";

        /***
         * Building an edge-expanded graph from node-based input and turn restrictions
         */

        SimpleLogger().Write() << "Generating edge-expanded graph representation";
        std::shared_ptr<NodeBasedDynamicGraph> node_based_graph =
            NodeBasedDynamicGraphFromImportEdges(number_of_node_based_nodes, edge_list);
        std::unique_ptr<RestrictionMap> restriction_map =
            std::unique_ptr<RestrictionMap>(new RestrictionMap(node_based_graph, restriction_list));
        EdgeBasedGraphFactory *edge_based_graph_factor =
            new EdgeBasedGraphFactory(node_based_graph,
                                      std::move(restriction_map),
                                      barrier_node_list,
                                      traffic_light_list,
                                      internal_to_external_node_map,
                                      speed_profile);
        edge_list.clear();
        edge_list.shrink_to_fit();

        edge_based_graph_factor->Run(edge_out, geometry_filename, lua_state);

        restriction_list.clear();
        restriction_list.shrink_to_fit();
        barrier_node_list.clear();
        barrier_node_list.shrink_to_fit();
        traffic_light_list.clear();
        traffic_light_list.shrink_to_fit();

        unsigned number_of_edge_based_nodes = edge_based_graph_factor->GetNumberOfEdgeBasedNodes();
        BOOST_ASSERT(number_of_edge_based_nodes != std::numeric_limits<unsigned>::max());
        DeallocatingVector<EdgeBasedEdge> edgeBasedEdgeList;
        #ifndef WIN32
        static_assert(sizeof(EdgeBasedEdge) == 16,
                      "changing ImportEdge type has influence on memory consumption!");
        #endif

        edge_based_graph_factor->GetEdgeBasedEdges(edgeBasedEdgeList);
        std::vector<EdgeBasedNode> node_based_edge_list;
        edge_based_graph_factor->GetEdgeBasedNodes(node_based_edge_list);
        delete edge_based_graph_factor;

        // TODO actually use scoping: Split this up in subfunctions
        node_based_graph.reset();

        TIMER_STOP(expansion);

        // Building grid-like nearest-neighbor data structure
        SimpleLogger().Write() << "building r-tree ...";
        StaticRTree<EdgeBasedNode> *rtree =
            new StaticRTree<EdgeBasedNode>(node_based_edge_list,
                                           rtree_nodes_path.c_str(),
                                           rtree_leafs_path.c_str(),
                                           internal_to_external_node_map);
        delete rtree;
        IteratorbasedCRC32<std::vector<EdgeBasedNode>> crc32;
        unsigned node_based_edge_list_CRC32 =
            crc32(node_based_edge_list.begin(), node_based_edge_list.end());
        node_based_edge_list.clear();
        node_based_edge_list.shrink_to_fit();
        SimpleLogger().Write() << "CRC32: " << node_based_edge_list_CRC32;

        /***
         * Writing info on original (node-based) nodes
         */

        SimpleLogger().Write() << "writing node map ...";
        boost::filesystem::ofstream node_stream(node_filename, std::ios::binary);
        const unsigned size_of_mapping = internal_to_external_node_map.size();
        node_stream.write((char *)&size_of_mapping, sizeof(unsigned));
        if (size_of_mapping > 0)
        {
            node_stream.write((char *)&(internal_to_external_node_map[0]),
                          size_of_mapping * sizeof(NodeInfo));
        }
        node_stream.close();
        internal_to_external_node_map.clear();
        internal_to_external_node_map.shrink_to_fit();

        /***
         * Contracting the edge-expanded graph
         */

        SimpleLogger().Write() << "initializing contractor";
        Contractor *contractor = new Contractor(number_of_edge_based_nodes, edgeBasedEdgeList);

        TIMER_START(contraction);
        contractor->Run();
        TIMER_STOP(contraction);

        SimpleLogger().Write() << "Contraction took " << TIMER_SEC(contraction) << " sec";

        DeallocatingVector<QueryEdge> contracted_edge_list;
        contractor->GetEdges(contracted_edge_list);
        delete contractor;

        /***
         * Sorting contracted edges in a way that the static query graph can read some in in-place.
         */

        std::sort(contracted_edge_list.begin(), contracted_edge_list.end());
        unsigned max_used_node_id = 0;
        unsigned contracted_edge_count = contracted_edge_list.size();
        SimpleLogger().Write() << "Serializing compacted graph of " << contracted_edge_count
                               << " edges";

        boost::filesystem::ofstream hsgr_output_stream(graphOut, std::ios::binary);
        hsgr_output_stream.write((char *)&fingerprint_orig, sizeof(FingerPrint));
        for (const QueryEdge &edge : contracted_edge_list)
        {
            BOOST_ASSERT(UINT_MAX != edge.source);
            BOOST_ASSERT(UINT_MAX != edge.target);

            max_used_node_id = std::max(max_used_node_id, edge.source);
            max_used_node_id = std::max(max_used_node_id, edge.target);
        }
        SimpleLogger().Write(logDEBUG) << "input graph has " << number_of_edge_based_nodes
                                       << " nodes";
        SimpleLogger().Write(logDEBUG) << "contracted graph has " << max_used_node_id << " nodes";
        max_used_node_id += 1;

        std::vector<StaticGraph<EdgeData>::NodeArrayEntry> node_array;
        node_array.resize(number_of_edge_based_nodes + 1);

        SimpleLogger().Write() << "Building node array";
        StaticGraph<EdgeData>::EdgeIterator edge = 0;
        StaticGraph<EdgeData>::EdgeIterator position = 0;
        StaticGraph<EdgeData>::EdgeIterator last_edge = edge;

        for (StaticGraph<EdgeData>::NodeIterator node = 0; node < max_used_node_id; ++node)
        {
            last_edge = edge;
            while ((edge < contracted_edge_count) && (contracted_edge_list[edge].source == node))
            {
                ++edge;
            }
            node_array[node].first_edge = position; //=edge
            position += edge - last_edge;           // remove
        }

        for (unsigned sentinel_counter = max_used_node_id; sentinel_counter != node_array.size();
             ++sentinel_counter)
        {
            // sentinel element, guarded against underflow
            node_array[sentinel_counter].first_edge = contracted_edge_count;
        }

        unsigned node_array_size = node_array.size();
        // serialize crc32, aka checksum
        hsgr_output_stream.write((char *)&node_based_edge_list_CRC32, sizeof(unsigned));
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
        for (unsigned edge = 0; edge < contracted_edge_list.size(); ++edge)
        {
            // no eigen loops
            BOOST_ASSERT(contracted_edge_list[edge].source != contracted_edge_list[edge].target);
            current_edge.target = contracted_edge_list[edge].target;
            current_edge.data = contracted_edge_list[edge].data;

            // every target needs to be valid
            BOOST_ASSERT(current_edge.target < max_used_node_id);
#ifndef NDEBUG
            if (current_edge.data.distance <= 0)
            {
                SimpleLogger().Write(logWARNING)
                    << "Edge: " << edge << ",source: " << contracted_edge_list[edge].source
                    << ", target: " << contracted_edge_list[edge].target
                    << ", dist: " << current_edge.data.distance;

                SimpleLogger().Write(logWARNING) << "Failed at adjacency list of node "
                                                 << contracted_edge_list[edge].source << "/"
                                                 << node_array.size() - 1;
                return 1;
            }
#endif
            hsgr_output_stream.write((char *)&current_edge,
                                     sizeof(StaticGraph<EdgeData>::EdgeArrayEntry));
            ++number_of_used_edges;
        }
        hsgr_output_stream.close();

        TIMER_STOP(preparing);

        SimpleLogger().Write() << "Preprocessing : " << TIMER_SEC(preparing) << " seconds";
        SimpleLogger().Write() << "Expansion  : "
                               << (number_of_node_based_nodes / TIMER_SEC(expansion))
                               << " nodes/sec and "
                               << (number_of_edge_based_nodes / TIMER_SEC(expansion))
                               << " edges/sec";

        SimpleLogger().Write() << "Contraction: "
                               << (number_of_edge_based_nodes / TIMER_SEC(contraction))
                               << " nodes/sec and "
                               << number_of_used_edges / TIMER_SEC(contraction)
                               << " edges/sec";

        node_array.clear();
        SimpleLogger().Write() << "finished preprocessing";
    }
    catch (boost::program_options::too_many_positional_options_error &)
    {
        SimpleLogger().Write(logWARNING) << "Only one file can be specified";
        return 1;
    }
    catch (boost::program_options::error &e)
    {
        SimpleLogger().Write(logWARNING) << e.what();
        return 1;
    }
    catch (const std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << "Exception occured: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
