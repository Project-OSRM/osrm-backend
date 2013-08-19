/*
    open source routing machine
    Copyright (C) Dennis Luxen, 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
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
#include "Util/ProgramOptions.h"
#include "Util/SimpleLogger.h"
#include "Util/StringUtil.h"
#include "typedefs.h"

#include <boost/foreach.hpp>

#include <luabind/luabind.hpp>

#include <fstream>
#include <istream>
#include <iostream>
#include <cstring>
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
            ("threads,t", boost::program_options::value<int>(&requested_num_threads)->default_value(10),
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
            SimpleLogger().Write() << std::endl << visible_options;
            return 0;
        }

        boost::program_options::notify(option_variables);

        // parse config file
        if(boost::filesystem::is_regular_file(config_file_path)) {
            std::ifstream ifs(config_file_path.c_str());
            SimpleLogger().Write() << "Reading options from: " << config_file_path.filename().string();
            boost::program_options::store(parse_config_file(ifs, config_file_options), option_variables);
            boost::program_options::notify(option_variables);
        }

        if(!option_variables.count("restrictions")) {
            restrictions_path = std::string( input_path.c_str()) + ".restrictions";
        }

        if(!option_variables.count("input")) {
            SimpleLogger().Write(logWARNING) << "No input file specified";
            return -1;
        }

        if(1 > requested_num_threads) {
            SimpleLogger().Write(logWARNING) << "Number of threads must be 1 or larger";
            return -1;
        }

        SimpleLogger().Write() << "Input file: " << input_path.filename().string();
        SimpleLogger().Write() << "Restrictions file: " << restrictions_path.filename().string();
        SimpleLogger().Write() << "Profile: " << profile_path.filename().string();
        SimpleLogger().Write() << "Threads: " << requested_num_threads;

        omp_set_num_threads( std::min( omp_get_num_procs(), requested_num_threads) );
        LogPolicy::GetInstance().Unmute();
        std::ifstream restrictionsInstream( restrictions_path.c_str(), std::ios::binary);
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

        std::ifstream in;
        in.open (input_path.c_str(), std::ifstream::in | std::ifstream::binary);

        std::string nodeOut(input_path.c_str());		nodeOut += ".nodes";
        std::string edgeOut(input_path.c_str());		edgeOut += ".edges";
        std::string graphOut(input_path.c_str());		graphOut += ".hsgr";
        std::string rtree_nodes_path(input_path.c_str());  rtree_nodes_path += ".ramIndex";
        std::string rtree_leafs_path(input_path.c_str());  rtree_leafs_path += ".fileIndex";

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
            std::cerr <<
                lua_tostring(myLuaState,-1) <<
                " occured in scripting block" <<
                std::endl;
                return -1;
        }
        speedProfile.trafficSignalPenalty = 10*lua_tointeger(myLuaState, -1);

        if(0 != luaL_dostring( myLuaState, "return u_turn_penalty\n")) {
            std::cerr <<
                lua_tostring(myLuaState,-1)   <<
                " occured in scripting block" <<
                std::endl;
            return -1;
        }
        speedProfile.uTurnPenalty = 10*lua_tointeger(myLuaState, -1);

        speedProfile.has_turn_penalty_function = lua_function_exists( myLuaState, "turn_function" );

        std::vector<ImportEdge> edgeList;
        NodeID nodeBasedNodeNumber = readBinaryOSRMGraphFromStream(in, edgeList, bollardNodes, trafficLightNodes, &internalToExternalNodeMapping, inputRestrictions);
        in.close();
        SimpleLogger().Write() <<
            inputRestrictions.size() <<
            " restrictions, " <<
            bollardNodes.size() <<
            " bollard nodes, " <<
            trafficLightNodes.size() <<
            " traffic lights";

        if(0 == edgeList.size()) {
            std::cerr <<
                "The input data is broken. "
                "It is impossible to do any turns in this graph" <<
                std::endl;
            return -1;
        }

        /***
         * Building an edge-expanded graph from node-based input an turn restrictions
         */

        SimpleLogger().Write() << "Generating edge-expanded graph representation";
        EdgeBasedGraphFactory * edgeBasedGraphFactory = new EdgeBasedGraphFactory (nodeBasedNodeNumber, edgeList, bollardNodes, trafficLightNodes, inputRestrictions, internalToExternalNodeMapping, speedProfile);
        std::vector<ImportEdge>().swap(edgeList);
        edgeBasedGraphFactory->Run(edgeOut.c_str(), myLuaState);
        std::vector<TurnRestriction>().swap(inputRestrictions);
        std::vector<NodeID>().swap(bollardNodes);
        std::vector<NodeID>().swap(trafficLightNodes);
        NodeID edgeBasedNodeNumber = edgeBasedGraphFactory->GetNumberOfNodes();
        DeallocatingVector<EdgeBasedEdge> edgeBasedEdgeList;
        edgeBasedGraphFactory->GetEdgeBasedEdges(edgeBasedEdgeList);
        std::vector<EdgeBasedGraphFactory::EdgeBasedNode> nodeBasedEdgeList;
        edgeBasedGraphFactory->GetEdgeBasedNodes(nodeBasedEdgeList);
        delete edgeBasedGraphFactory;

        /***
         * Writing info on original (node-based) nodes
         */

        SimpleLogger().Write() << "writing node map ...";
        std::ofstream mapOutFile(nodeOut.c_str(), std::ios::binary);
        mapOutFile.write((char *)&(internalToExternalNodeMapping[0]), internalToExternalNodeMapping.size()*sizeof(NodeInfo));
        mapOutFile.close();
        std::vector<NodeInfo>().swap(internalToExternalNodeMapping);

        double expansionHasFinishedTime = get_timestamp() - startupTime;

        /***
         * Building grid-like nearest-neighbor data structure
         */

        SimpleLogger().Write() << "building r-tree ...";
        StaticRTree<EdgeBasedGraphFactory::EdgeBasedNode> * rtree =
                new StaticRTree<EdgeBasedGraphFactory::EdgeBasedNode>(
                        nodeBasedEdgeList,
                        rtree_nodes_path.c_str(),
                        rtree_leafs_path.c_str()
                );
        delete rtree;
        IteratorbasedCRC32<std::vector<EdgeBasedGraphFactory::EdgeBasedNode> > crc32;
        unsigned crc32OfNodeBasedEdgeList = crc32(nodeBasedEdgeList.begin(), nodeBasedEdgeList.end() );
        nodeBasedEdgeList.clear();
        SimpleLogger().Write() << "CRC32: " << crc32OfNodeBasedEdgeList;

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

        SimpleLogger().Write() << "Building Node Array";
        std::sort(contractedEdgeList.begin(), contractedEdgeList.end());
        unsigned numberOfNodes = 0;
        unsigned numberOfEdges = contractedEdgeList.size();
        SimpleLogger().Write() <<
            "Serializing compacted graph of " <<
            numberOfEdges <<
            " edges";

        std::ofstream hsgr_output_stream(graphOut.c_str(), std::ios::binary);
        hsgr_output_stream.write((char*)&uuid_orig, sizeof(UUID) );
        BOOST_FOREACH(const QueryEdge & edge, contractedEdgeList) {
            if(edge.source > numberOfNodes) {
                numberOfNodes = edge.source;
            }
            if(edge.target > numberOfNodes) {
                numberOfNodes = edge.target;
            }
        }
        numberOfNodes+=1;

        std::vector< StaticGraph<EdgeData>::_StrNode > _nodes;
        _nodes.resize( numberOfNodes + 1 );

        StaticGraph<EdgeData>::EdgeIterator edge = 0;
        StaticGraph<EdgeData>::EdgeIterator position = 0;
        for ( StaticGraph<EdgeData>::NodeIterator node = 0; node <= numberOfNodes; ++node ) {
            StaticGraph<EdgeData>::EdgeIterator lastEdge = edge;
            while ( edge < numberOfEdges && contractedEdgeList[edge].source == node )
                ++edge;
            _nodes[node].firstEdge = position; //=edge
            position += edge - lastEdge; //remove
        }
        ++numberOfNodes;
        //Serialize numberOfNodes, nodes
        hsgr_output_stream.write((char*) &crc32OfNodeBasedEdgeList, sizeof(unsigned));
        hsgr_output_stream.write((char*) &numberOfNodes, sizeof(unsigned));
        hsgr_output_stream.write((char*) &_nodes[0], sizeof(StaticGraph<EdgeData>::_StrNode)*(numberOfNodes));
        //Serialize number of Edges
        hsgr_output_stream.write((char*) &position, sizeof(unsigned));
        --numberOfNodes;
        edge = 0;
        int usedEdgeCounter = 0;
        StaticGraph<EdgeData>::_StrEdge currentEdge;
        for ( StaticGraph<EdgeData>::NodeIterator node = 0; node < numberOfNodes; ++node ) {
            for ( StaticGraph<EdgeData>::EdgeIterator i = _nodes[node].firstEdge, e = _nodes[node+1].firstEdge; i != e; ++i ) {
                assert(node != contractedEdgeList[edge].target);
                currentEdge.target = contractedEdgeList[edge].target;
                currentEdge.data = contractedEdgeList[edge].data;
                if(currentEdge.data.distance <= 0) {
                    SimpleLogger().Write(logWARNING) <<
                        "Edge: "     << i <<
                        ",source: "  << contractedEdgeList[edge].source <<
                        ", target: " << contractedEdgeList[edge].target <<
                        ", dist: "   << currentEdge.data.distance;

                    SimpleLogger().Write(logWARNING) <<
                        "Failed at edges of node " << node <<
                        " of " << numberOfNodes;
                        return -1;
                }
                //Serialize edges
                hsgr_output_stream.write((char*) &currentEdge, sizeof(StaticGraph<EdgeData>::_StrEdge));
                ++edge;
                ++usedEdgeCounter;
            }
        }
        SimpleLogger().Write() << "Preprocessing : " <<
            (get_timestamp() - startupTime) << " seconds";
        SimpleLogger().Write() << "Expansion  : " <<
            (nodeBasedNodeNumber/expansionHasFinishedTime) << " nodes/sec and " <<
            (edgeBasedNodeNumber/expansionHasFinishedTime) << " edges/sec";

        SimpleLogger().Write() << "Contraction: " <<
            (edgeBasedNodeNumber/contraction_duration) << " nodes/sec and " <<
            usedEdgeCounter/contraction_duration << " edges/sec";

        hsgr_output_stream.close();
        //cleanedEdgeList.clear();
        _nodes.clear();
        SimpleLogger().Write() << "finished preprocessing";
    } catch(boost::program_options::too_many_positional_options_error& e) {
        SimpleLogger().Write(logWARNING) << "Only one file can be specified";
        return -1;
    } catch(boost::program_options::error& e) {
        SimpleLogger().Write(logWARNING) << e.what();
        return -1;
    } catch ( const std::exception &e ) {
        SimpleLogger().Write(logWARNING) <<
            "Exception occured: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
