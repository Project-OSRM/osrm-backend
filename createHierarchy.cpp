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

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <luabind/luabind.hpp>

#include <boost/foreach.hpp>

#include <fstream>
#include <istream>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

#include "Algorithms/IteratorBasedCRC32.h"
#include "Util/OpenMPWrapper.h"
#include "typedefs.h"
#include "Contractor/Contractor.h"
#include "Contractor/EdgeBasedGraphFactory.h"
#include "DataStructures/BinaryHeap.h"
#include "DataStructures/DeallocatingVector.h"
#include "DataStructures/NNGrid.h"
#include "DataStructures/QueryEdge.h"
#include "Util/BaseConfiguration.h"
#include "Util/GraphLoader.h"
#include "Util/InputFileUtil.h"
#include "Util/LuaUtil.h"
#include "Util/StringUtil.h"

typedef QueryEdge::EdgeData EdgeData;
typedef DynamicGraph<EdgeData>::InputEdge InputEdge;
typedef StaticGraph<EdgeData>::InputEdge StaticEdge;
typedef BaseConfiguration ContractorConfiguration;

std::vector<NodeInfo> internalToExternalNodeMapping;
std::vector<_Restriction> inputRestrictions;
std::vector<NodeID> bollardNodes;
std::vector<NodeID> trafficLightNodes;
std::vector<ImportEdge> edgeList;

int main (int argc, char *argv[]) {
    if(argc < 3) {
        ERR("usage: " << std::endl << argv[0] << " <osrm-data> <osrm-restrictions> [<profile>]");
    }

    double startupTime = get_timestamp();
    unsigned numberOfThreads = omp_get_num_procs();
    if(testDataFile("contractor.ini")) {
        ContractorConfiguration contractorConfig("contractor.ini");
        unsigned rawNumber = stringToInt(contractorConfig.GetParameter("Threads"));
        if(rawNumber != 0 && rawNumber <= numberOfThreads)
            numberOfThreads = rawNumber;
    }
    omp_set_num_threads(numberOfThreads);

    INFO("Using restrictions from file: " << argv[2]);
    std::ifstream restrictionsInstream(argv[2], std::ios::binary);
    if(!restrictionsInstream.good()) {
        ERR("Could not access <osrm-restrictions> files");
    }
    _Restriction restriction;
    unsigned usableRestrictionsCounter(0);
    restrictionsInstream.read((char*)&usableRestrictionsCounter, sizeof(unsigned));
    inputRestrictions.resize(usableRestrictionsCounter);
    restrictionsInstream.read((char *)&(inputRestrictions[0]), usableRestrictionsCounter*sizeof(_Restriction));
    restrictionsInstream.close();

    std::ifstream in;
    in.open (argv[1], std::ifstream::in | std::ifstream::binary);
    if (!in.is_open()) {
        ERR("Cannot open " << argv[1]);
    }

    std::string nodeOut(argv[1]);		nodeOut += ".nodes";
    std::string edgeOut(argv[1]);		edgeOut += ".edges";
    std::string graphOut(argv[1]);		graphOut += ".hsgr";
    std::string ramIndexOut(argv[1]);	ramIndexOut += ".ramIndex";
    std::string fileIndexOut(argv[1]);	fileIndexOut += ".fileIndex";

    /*** Setup Scripting Environment ***/
    if(!testDataFile( (argc > 3 ? argv[3] : "profile.lua") )) {
        ERR("Need profile.lua to apply traffic signal penalty");
    }

    // Create a new lua state
    lua_State *myLuaState = luaL_newstate();

    // Connect LuaBind to this lua state
    luabind::open(myLuaState);

    //open utility libraries string library;
    luaL_openlibs(myLuaState);

    //adjust lua load path
    luaAddScriptFolderToLoadPath( myLuaState, (argc > 3 ? argv[3] : "profile.lua") );

    // Now call our function in a lua script
    INFO("Parsing speedprofile from " << (argc > 3 ? argv[3] : "profile.lua") );
    if(0 != luaL_dofile(myLuaState, (argc > 3 ? argv[3] : "profile.lua") )) {
        ERR(lua_tostring(myLuaState,-1)<< " occured in scripting block");
    }

    EdgeBasedGraphFactory::SpeedProfileProperties speedProfile;

    if(0 != luaL_dostring( myLuaState, "return traffic_signal_penalty\n")) {
        ERR(lua_tostring(myLuaState,-1)<< " occured in scripting block");
    }
    speedProfile.trafficSignalPenalty = 10*lua_tointeger(myLuaState, -1);

    if(0 != luaL_dostring( myLuaState, "return u_turn_penalty\n")) {
        ERR(lua_tostring(myLuaState,-1)<< " occured in scripting block");
    }
    speedProfile.uTurnPenalty = 10*lua_tointeger(myLuaState, -1);
    
    speedProfile.has_turn_penalty_function = lua_function_exists( myLuaState, "turn_function" );
    
    std::vector<ImportEdge> edgeList;
    NodeID nodeBasedNodeNumber = readBinaryOSRMGraphFromStream(in, edgeList, bollardNodes, trafficLightNodes, &internalToExternalNodeMapping, inputRestrictions);
    in.close();
    INFO(inputRestrictions.size() << " restrictions, " << bollardNodes.size() << " bollard nodes, " << trafficLightNodes.size() << " traffic lights");
    if(0 == edgeList.size())
        ERR("The input data is broken. It is impossible to do any turns in this graph");


    /***
     * Building an edge-expanded graph from node-based input an turn restrictions
     */

    INFO("Generating edge-expanded graph representation");
    EdgeBasedGraphFactory * edgeBasedGraphFactory = new EdgeBasedGraphFactory (nodeBasedNodeNumber, edgeList, bollardNodes, trafficLightNodes, inputRestrictions, internalToExternalNodeMapping, speedProfile);
    std::vector<ImportEdge>().swap(edgeList);
    edgeBasedGraphFactory->Run(edgeOut.c_str(), myLuaState);
    std::vector<_Restriction>().swap(inputRestrictions);
    std::vector<NodeID>().swap(bollardNodes);
    std::vector<NodeID>().swap(trafficLightNodes);
    NodeID edgeBasedNodeNumber = edgeBasedGraphFactory->GetNumberOfNodes();
    DeallocatingVector<EdgeBasedEdge> edgeBasedEdgeList;
    edgeBasedGraphFactory->GetEdgeBasedEdges(edgeBasedEdgeList);
    DeallocatingVector<EdgeBasedGraphFactory::EdgeBasedNode> nodeBasedEdgeList;
    edgeBasedGraphFactory->GetEdgeBasedNodes(nodeBasedEdgeList);
    delete edgeBasedGraphFactory;

    /***
     * Writing info on original (node-based) nodes
     */

    INFO("writing node map ...");
    std::ofstream mapOutFile(nodeOut.c_str(), std::ios::binary);
    mapOutFile.write((char *)&(internalToExternalNodeMapping[0]), internalToExternalNodeMapping.size()*sizeof(NodeInfo));
    mapOutFile.close();
    std::vector<NodeInfo>().swap(internalToExternalNodeMapping);

    double expansionHasFinishedTime = get_timestamp() - startupTime;

    /***
     * Building grid-like nearest-neighbor data structure
     */

    INFO("building grid ...");
    WritableGrid * writeableGrid = new WritableGrid();
    writeableGrid->ConstructGrid(nodeBasedEdgeList, ramIndexOut.c_str(), fileIndexOut.c_str());
    delete writeableGrid;
    IteratorbasedCRC32<DeallocatingVector<EdgeBasedGraphFactory::EdgeBasedNode> > crc32;
    unsigned crc32OfNodeBasedEdgeList = crc32(nodeBasedEdgeList.begin(), nodeBasedEdgeList.end() );
    nodeBasedEdgeList.clear();
    INFO("CRC32 based checksum is " << crc32OfNodeBasedEdgeList);

    /***
     * Contracting the edge-expanded graph
     */

    INFO("initializing contractor");
    Contractor* contractor = new Contractor( edgeBasedNodeNumber, edgeBasedEdgeList );
    double contractionStartedTimestamp(get_timestamp());
    contractor->Run();
    INFO("Contraction took " << get_timestamp() - contractionStartedTimestamp << " sec");

    DeallocatingVector< QueryEdge > contractedEdgeList;
    contractor->GetEdges( contractedEdgeList );
    delete contractor;

    /***
     * Sorting contracted edges in a way that the static query graph can read some in in-place.
     */

    INFO("Building Node Array");
    std::sort(contractedEdgeList.begin(), contractedEdgeList.end());
    unsigned numberOfNodes = 0;
    unsigned numberOfEdges = contractedEdgeList.size();
    INFO("Serializing compacted graph");
    std::ofstream edgeOutFile(graphOut.c_str(), std::ios::binary);

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
    edgeOutFile.write((char*) &crc32OfNodeBasedEdgeList, sizeof(unsigned));
    edgeOutFile.write((char*) &numberOfNodes, sizeof(unsigned));
    edgeOutFile.write((char*) &_nodes[0], sizeof(StaticGraph<EdgeData>::_StrNode)*(numberOfNodes));
    //Serialize number of Edges
    edgeOutFile.write((char*) &position, sizeof(unsigned));
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
                INFO("Edge: " << i << ",source: " << contractedEdgeList[edge].source << ", target: " << contractedEdgeList[edge].target << ", dist: " << currentEdge.data.distance);
                ERR("Failed at edges of node " << node << " of " << numberOfNodes);
            }
            //Serialize edges
            //INFO( "createHierachy write, i: " << usedEdgeCounter << ", mode: " << (long)currentEdge.data.mode );
            edgeOutFile.write((char*) &currentEdge, sizeof(StaticGraph<EdgeData>::_StrEdge));
            ++edge;
            ++usedEdgeCounter;
        }
    }
    double endTime = (get_timestamp() - startupTime);
    INFO("Expansion  : " << (nodeBasedNodeNumber/expansionHasFinishedTime) << " nodes/sec and "<< (edgeBasedNodeNumber/expansionHasFinishedTime) << " edges/sec");
    INFO("Contraction: " << (edgeBasedNodeNumber/expansionHasFinishedTime) << " nodes/sec and "<< usedEdgeCounter/endTime << " edges/sec");

    edgeOutFile.close();
    //cleanedEdgeList.clear();
    _nodes.clear();
    INFO("finished preprocessing");
    return 0;
}
