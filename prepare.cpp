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
#include "Util/GitDescription.h"
#include "Util/GraphLoader.h"
#include "Util/InputFileUtil.h"
#include "Util/LuaUtil.h"
#include "Util/OpenMPWrapper.h"
#include "Util/OSRMException.h"
#include "Util/TimingUtil.h"
#include "Util/SimpleLogger.h"
#include "Util/StringUtil.h"
#include "typedefs.h"

#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <luabind/luabind.hpp>

#include <string>
#include <vector>

typedef QueryEdge::EdgeData EdgeData;
typedef DynamicGraph<EdgeData>::InputEdge InputEdge;
typedef StaticGraph<EdgeData>::InputEdge StaticEdge;

std::vector<NodeInfo> internalToExternalNodeMapping;
std::vector<TurnRestriction> inputRestrictions;
std::vector<NodeID> bollardNodes;
std::vector<NodeID> trafficLightNodes;
std::vector<ImportEdge> edgeList;

int main (int argc, char *argv[]) {
    try {
        LogPolicy::GetInstance().Unmute();
        double startupTime = get_timestamp();
        boost::filesystem::path config_file_path, input_path, restrictions_path, profile_path;
        int requested_num_threads;

        // declare a group of options that will be allowed only on command line
        boost::program_options::options_description generic_options("Options");
        generic_options.add_options()
            ("version,v", "Show version")
            ("help,h", "Show this help message")
            ("config,c", boost::program_options::value<boost::filesystem::path>(&config_file_path)->default_value("contractor.ini"),
                  "Path to a configuration file.");

        // declare a group of options that will be allowed both on command line and in config file
        boost::program_options::options_description config_options("Configuration");
        config_options.add_options()
            ("restrictions,r", boost::program_options::value<boost::filesystem::path>(&restrictions_path),
                "Restrictions file in .osrm.restrictions format")
            ("profile,p", boost::program_options::value<boost::filesystem::path>(&profile_path)->default_value("profile.lua"),
                "Path to LUA routing profile")
            ("threads,t", boost::program_options::value<int>(&requested_num_threads)->default_value(8),
                "Number of threads to use");

        // hidden options, will be allowed both on command line and in config file, but will not be shown to the user
        boost::program_options::options_description hidden_options("Hidden options");
        hidden_options.add_options()
            ("input,i", boost::program_options::value<boost::filesystem::path>(&input_path),
                "Input file in .osm, .osm.bz2 or .osm.pbf format");

        // positional option
        boost::program_options::positional_options_description positional_options;
        positional_options.add("input", 1);

        // combine above options for parsing
        boost::program_options::options_description cmdline_options;
        cmdline_options.add(generic_options).add(config_options).add(hidden_options);

        boost::program_options::options_description config_file_options;
        config_file_options.add(config_options).add(hidden_options);

        boost::program_options::options_description visible_options("Usage: " + boost::filesystem::basename(argv[0]) + " <input.osrm> [options]");
        visible_options.add(generic_options).add(config_options);

        // parse command line options
        boost::program_options::variables_map option_variables;
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
            options(cmdline_options).positional(positional_options).run(), option_variables);

        if(option_variables.count("version")) {
            SimpleLogger().Write() << g_GIT_DESCRIPTION;
            return 0;
        }

        if(option_variables.count("help")) {
            SimpleLogger().Write() << "\n" << visible_options;
            return 0;
        }

        boost::program_options::notify(option_variables);

        if(!option_variables.count("restrictions")) {
            restrictions_path = std::string(input_path.string() + ".restrictions");
        }

        if(!option_variables.count("input")) {
            SimpleLogger().Write() << "\n" << visible_options;
            return 0;
        }

        if(!boost::filesystem::is_regular_file(input_path)) {
            SimpleLogger().Write(logWARNING) << "Input file " << input_path.string() << " not found!";
            return 1;
        }

        if(!boost::filesystem::is_regular_file(profile_path)) {
            SimpleLogger().Write(logWARNING) << "Profile " << profile_path.string() << " not found!";
            return 1;
        }

        if(1 > requested_num_threads) {
            SimpleLogger().Write(logWARNING) << "Number of threads must be 1 or larger";
            return 1;
        }

        int real_num_threads = std::min(omp_get_num_procs(), requested_num_threads);

        SimpleLogger().Write() << "Input file: " << input_path.filename().string();
        SimpleLogger().Write() << "Restrictions file: " << restrictions_path.filename().string();
        SimpleLogger().Write() << "Profile: " << profile_path.filename().string();
        SimpleLogger().Write() << "Threads: " << real_num_threads << " (requested " << requested_num_threads << ")";

        omp_set_num_threads(real_num_threads);
        LogPolicy::GetInstance().Unmute();
        boost::filesystem::ifstream restrictionsInstream(restrictions_path, std::ios::binary);
        TurnRestriction restriction;
        UUID uuid_loaded, uuid_orig;
        unsigned usableRestrictionsCounter(0);
        restrictionsInstream.read((char*)&uuid_loaded, sizeof(UUID));
        if( !uuid_loaded.TestPrepare(uuid_orig) ) {
            SimpleLogger().Write(logWARNING) <<
                ".restrictions was prepared with different build.\n"
                "Reprocess to get rid of this warning.";
        }

        restrictionsInstream.read(
            (char*)&usableRestrictionsCounter,
            sizeof(unsigned)
        );
        inputRestrictions.resize(usableRestrictionsCounter);
        restrictionsInstream.read(
            (char *)&(inputRestrictions[0]),
            usableRestrictionsCounter*sizeof(TurnRestriction)
        );
        restrictionsInstream.close();

        boost::filesystem::ifstream in;
        in.open(input_path, std::ios::in|std::ios::binary);

        const std::string nodeOut           = input_path.string() + ".nodes";
        const std::string edgeOut           = input_path.string() + ".edges";
        const std::string geometry_filename = input_path.string() + ".geometry";
        const std::string graphOut          = input_path.string() + ".hsgr";
        const std::string rtree_nodes_path  = input_path.string() + ".ramIndex";
        const std::string rtree_leafs_path  = input_path.string() + ".fileIndex";

        /*** Setup Scripting Environment ***/

        // Create a new lua state
        lua_State *myLuaState = luaL_newstate();

        // Connect LuaBind to this lua state
        luabind::open(myLuaState);

        //open utility libraries string library;
        luaL_openlibs(myLuaState);

        //adjust lua load path
        luaAddScriptFolderToLoadPath( myLuaState, profile_path.c_str() );

        // Now call our function in a lua script
        if(0 != luaL_dofile(myLuaState, profile_path.c_str() )) {
            std::cerr <<
                lua_tostring(myLuaState,-1)   <<
                " occured in scripting block" <<
                std::endl;
        }

        EdgeBasedGraphFactory::SpeedProfileProperties speedProfile;

        if(0 != luaL_dostring( myLuaState, "return traffic_signal_penalty\n")) {
            std::cerr << lua_tostring(myLuaState,-1) << " occured in scripting block" << std::endl;
            return 1;
        }
        speedProfile.trafficSignalPenalty = 10*lua_tointeger(myLuaState, -1);
        SimpleLogger().Write(logDEBUG) << "traffic_signal_penalty: " << speedProfile.trafficSignalPenalty;

        if(0 != luaL_dostring( myLuaState, "return u_turn_penalty\n")) {
            std::cerr <<
                lua_tostring(myLuaState,-1)   <<
                " occured in scripting block" <<
                std::endl;
            return 1;
        }
        speedProfile.uTurnPenalty = 10*lua_tointeger(myLuaState, -1);

        speedProfile.has_turn_penalty_function = lua_function_exists( myLuaState, "turn_function" );

        std::vector<ImportEdge> edgeList;
        NodeID nodeBasedNodeNumber = readBinaryOSRMGraphFromStream(in, edgeList, bollardNodes, trafficLightNodes, &internalToExternalNodeMapping, inputRestrictions);
        in.close();

        if( edgeList.empty() ) {
            SimpleLogger().Write(logWARNING) << "The input data is empty, exiting.";
            return 1;
        }

        SimpleLogger().Write() <<
            inputRestrictions.size() << " restrictions, " <<
            bollardNodes.size() << " bollard nodes, " <<
            trafficLightNodes.size() << " traffic lights";

        /***
         * Building an edge-expanded graph from node-based input and turn restrictions
         */

        SimpleLogger().Write() << "Generating edge-expanded graph representation";
        EdgeBasedGraphFactory * edgeBasedGraphFactory = new EdgeBasedGraphFactory (nodeBasedNodeNumber, edgeList, bollardNodes, trafficLightNodes, inputRestrictions, internalToExternalNodeMapping, speedProfile);
        std::vector<ImportEdge>().swap(edgeList);
        edgeBasedGraphFactory->Run(edgeOut,geometry_filename, myLuaState);
        std::vector<TurnRestriction>().swap(inputRestrictions);
        std::vector<NodeID>().swap(bollardNodes);
        std::vector<NodeID>().swap(trafficLightNodes);
        unsigned edgeBasedNodeNumber = edgeBasedGraphFactory->GetNumberOfEdgeBasedNodes();
        BOOST_ASSERT(
            edgeBasedNodeNumber != std::numeric_limits<unsigned>::max()
        );
        DeallocatingVector<EdgeBasedEdge> edgeBasedEdgeList;
        edgeBasedGraphFactory->GetEdgeBasedEdges(edgeBasedEdgeList);
        std::vector<EdgeBasedNode> nodeBasedEdgeList;
        edgeBasedGraphFactory->GetEdgeBasedNodes(nodeBasedEdgeList);
        delete edgeBasedGraphFactory;

        double expansionHasFinishedTime = get_timestamp() - startupTime;


        // Building grid-like nearest-neighbor data structure
        SimpleLogger().Write() << "building r-tree ...";
        StaticRTree<EdgeBasedNode> * rtree =
                new StaticRTree<EdgeBasedNode>(
                    nodeBasedEdgeList,
                    rtree_nodes_path.c_str(),
                    rtree_leafs_path.c_str(),
                    internalToExternalNodeMapping
                );
        delete rtree;
        IteratorbasedCRC32<std::vector<EdgeBasedNode> > crc32;
        unsigned crc32OfNodeBasedEdgeList = crc32(nodeBasedEdgeList.begin(), nodeBasedEdgeList.end() );
        nodeBasedEdgeList.clear();
        std::vector<EdgeBasedNode>(nodeBasedEdgeList).swap(nodeBasedEdgeList);
        SimpleLogger().Write() << "CRC32: " << crc32OfNodeBasedEdgeList;

        /***
         * Writing info on original (node-based) nodes
         */

        SimpleLogger().Write() << "writing node map ...";
        boost::filesystem::ofstream mapOutFile(nodeOut, std::ios::binary);
        const unsigned size_of_mapping = internalToExternalNodeMapping.size();
        mapOutFile.write((char *)&size_of_mapping, sizeof(unsigned));
        mapOutFile.write(
            (char *)&(internalToExternalNodeMapping[0]),
            size_of_mapping*sizeof(NodeInfo)
        );
        mapOutFile.close();
        std::vector<NodeInfo>().swap(internalToExternalNodeMapping);

        /***
         * Contracting the edge-expanded graph
         */

        SimpleLogger().Write() << "initializing contractor";
        Contractor* contractor = new Contractor( edgeBasedNodeNumber, edgeBasedEdgeList );
        double contractionStartedTimestamp(get_timestamp());
        contractor->Run();
        const double contraction_duration = (get_timestamp() - contractionStartedTimestamp);
        SimpleLogger().Write() <<
            "Contraction took " <<
            contraction_duration <<
            " sec";

        DeallocatingVector< QueryEdge > contractedEdgeList;
        contractor->GetEdges( contractedEdgeList );
        delete contractor;

        /***
         * Sorting contracted edges in a way that the static query graph can read some in in-place.
         */

        std::sort(contractedEdgeList.begin(), contractedEdgeList.end());
        unsigned max_used_node_id = 0;
        unsigned contracted_edge_count = contractedEdgeList.size();
        SimpleLogger().Write() <<
            "Serializing compacted graph of " <<
            contracted_edge_count <<
            " edges";

        boost::filesystem::ofstream hsgr_output_stream(graphOut, std::ios::binary);
        hsgr_output_stream.write((char*)&uuid_orig, sizeof(UUID) );
        BOOST_FOREACH(const QueryEdge & edge, contractedEdgeList)
        {
            BOOST_ASSERT( UINT_MAX != edge.source );
            BOOST_ASSERT( UINT_MAX != edge.target );

            max_used_node_id = std::max(max_used_node_id, edge.source);
            max_used_node_id = std::max(max_used_node_id, edge.target);
        }
        SimpleLogger().Write(logDEBUG) << "input graph has " << edgeBasedNodeNumber << " nodes";
        SimpleLogger().Write(logDEBUG) << "contracted graph has " << max_used_node_id << " nodes";
        max_used_node_id+=1;

        std::vector< StaticGraph<EdgeData>::NodeArrayEntry > node_array;
        node_array.resize( edgeBasedNodeNumber + 1);

        SimpleLogger().Write() << "Building node array";
        StaticGraph<EdgeData>::EdgeIterator edge = 0;
        StaticGraph<EdgeData>::EdgeIterator position = 0;
        StaticGraph<EdgeData>::EdgeIterator lastEdge = edge;

        for ( StaticGraph<EdgeData>::NodeIterator node = 0; node < max_used_node_id; ++node)
        {
            lastEdge = edge;
            while ((edge < contracted_edge_count) && (contractedEdgeList[edge].source == node))
            {
                ++edge;
            }
            node_array[node].firstEdge = position; //=edge
            position += edge - lastEdge; //remove
        }

        for (unsigned sentinel_counter = max_used_node_id;
             sentinel_counter != node_array.size();
             ++sentinel_counter
            )
        {
            //sentinel element, guarded against underflow
            node_array[sentinel_counter].firstEdge = contracted_edge_count;
        }

        unsigned node_array_size = node_array.size();
        //serialize crc32, aka checksum
        hsgr_output_stream.write((char*) &crc32OfNodeBasedEdgeList, sizeof(unsigned));
        //serialize number of nodes
        hsgr_output_stream.write((char*) &node_array_size, sizeof(unsigned));
        //serialize number of edges
        hsgr_output_stream.write((char*) &contracted_edge_count, sizeof(unsigned));
        //serialize all nodes
        hsgr_output_stream.write((char*) &node_array[0], sizeof(StaticGraph<EdgeData>::NodeArrayEntry)*node_array_size);
        //serialize all edges

        SimpleLogger().Write() << "Building edge array";
        edge = 0;
        int usedEdgeCounter = 0;

        StaticGraph<EdgeData>::EdgeArrayEntry currentEdge;
        for (unsigned edge = 0; edge < contractedEdgeList.size(); ++edge)
        {
            // no eigen loops
            BOOST_ASSERT(contractedEdgeList[edge].source != contractedEdgeList[edge].target);
            currentEdge.target = contractedEdgeList[edge].target;
            currentEdge.data = contractedEdgeList[edge].data;

            // every target needs to be valid
            BOOST_ASSERT(currentEdge.target < max_used_node_id);
            if(currentEdge.data.distance <= 0) {
                SimpleLogger().Write(logWARNING) <<
                    "Edge: "     << edge <<
                    ",source: "  << contractedEdgeList[edge].source <<
                    ", target: " << contractedEdgeList[edge].target <<
                    ", dist: "   << currentEdge.data.distance;

                SimpleLogger().Write(logWARNING) <<
                    "Failed at adjacency list of node " << contractedEdgeList[edge].source << "/" << node_array.size()-1;
                return 1;
            }
            hsgr_output_stream.write((char*) &currentEdge, sizeof(StaticGraph<EdgeData>::EdgeArrayEntry));
            ++usedEdgeCounter;
        }
        hsgr_output_stream.close();

        SimpleLogger().Write() << "Preprocessing : " <<
            (get_timestamp() - startupTime) << " seconds";
        SimpleLogger().Write() << "Expansion  : " <<
            (nodeBasedNodeNumber/expansionHasFinishedTime) << " nodes/sec and " <<
            (edgeBasedNodeNumber/expansionHasFinishedTime) << " edges/sec";

        SimpleLogger().Write() << "Contraction: " <<
            (edgeBasedNodeNumber/contraction_duration) << " nodes/sec and " <<
            usedEdgeCounter/contraction_duration << " edges/sec";

        node_array.clear();
        SimpleLogger().Write() << "finished preprocessing";
    } catch(boost::program_options::too_many_positional_options_error&) {
        SimpleLogger().Write(logWARNING) << "Only one file can be specified";
        return 1;
    } catch(boost::program_options::error& e) {
        SimpleLogger().Write(logWARNING) << e.what();
        return 1;
    } catch ( const std::exception &e ) {
        SimpleLogger().Write(logWARNING) << "Exception occured: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
