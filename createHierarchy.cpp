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

//g++ createHierarchy.cpp -fopenmp -Wno-deprecated -o createHierarchy -O3 -march=native -DNDEBUG -I/usr/include/libxml2 -lstxxl

#define VERBOSE(x) x
#define VERBOSE2(x)

#ifdef NDEBUG
#undef VERBOSE
#undef VERBOSE2
#endif

#include <boost/foreach.hpp>

#include <fstream>
#include <istream>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

#include "Util/OpenMPReplacement.h"
#include "typedefs.h"
#include "Contractor/Contractor.h"
#include "Contractor/ContractionCleanup.h"
#include "Contractor/EdgeBasedGraphFactory.h"
#include "DataStructures/BinaryHeap.h"
#include "DataStructures/ExtractorStructs.h"
#include "DataStructures/NNGrid.h"
#include "Util/BaseConfiguration.h"
#include "Util/InputFileUtil.h"
#include "Util/GraphLoader.h"
#include "Util/OpenMPReplacement.h"

using namespace std;

typedef DynamicGraph<EdgeData>::InputEdge InputEdge;
typedef StaticGraph<EdgeData>::InputEdge StaticEdge;
typedef BaseConfiguration ContractorConfiguration;

std::vector<NodeInfo> internalToExternaleNodeMapping;
std::vector<_Restriction> inputRestrictions;
std::vector<NodeID> bollardNodes;
std::vector<NodeID> trafficLightNodes;

int main (int argc, char *argv[]) {
    if(argc < 3) {
        ERR("usage: " << std::endl << argv[0] << " <osrm-data> <osrm-restrictions>");
    }

    double startupTime = get_timestamp();
    unsigned numberOfThreads = omp_get_num_procs();
    std::string SRTM_ROOT;
    if(testDataFile("contractor.ini")) {
        ContractorConfiguration contractorConfig("contractor.ini");
        if(atoi(contractorConfig.GetParameter("Threads").c_str()) != 0 && (unsigned)atoi(contractorConfig.GetParameter("Threads").c_str()) <= numberOfThreads)
            numberOfThreads = (unsigned)atoi( contractorConfig.GetParameter("Threads").c_str() );
        if(0 < contractorConfig.GetParameter("SRTM").size() )
            SRTM_ROOT = contractorConfig.GetParameter("SRTM");
    }
    if(0 != SRTM_ROOT.size())
        INFO("Loading SRTM from/to " << SRTM_ROOT);
    omp_set_num_threads(numberOfThreads);

    INFO("preprocessing data from input file " << argv[2] << " using STL "
#ifdef _GLIBCXX_PARALLEL
            "parallel (GCC)"
#else
            "serial"
#endif
            " mode");

    INFO("Using restrictions from file: " << argv[2]);
    std::ifstream restrictionsInstream(argv[2], ios::binary);
    _Restriction restriction;
    unsigned usableRestrictionsCounter(0);
    restrictionsInstream.read((char*)&usableRestrictionsCounter, sizeof(unsigned));
    for(unsigned i = 0; i < usableRestrictionsCounter; ++i) {
        restrictionsInstream.read((char *)&(restriction), sizeof(_Restriction));
        inputRestrictions.push_back(restriction);
    }
    restrictionsInstream.close();


    ifstream in;
    in.open (argv[1], ifstream::in | ifstream::binary);
    if (!in.is_open()) {
        ERR("Cannot open " << argv[1]);
    }

    char nodeOut[1024];    		strcpy(nodeOut, argv[1]);    		strcat(nodeOut, ".nodes");
    char edgeOut[1024];    		strcpy(edgeOut, argv[1]);    		strcat(edgeOut, ".hsgr");
    char ramIndexOut[1024];    	strcpy(ramIndexOut, argv[1]);    	strcat(ramIndexOut, ".ramIndex");
    char fileIndexOut[1024];    strcpy(fileIndexOut, argv[1]);    	strcat(fileIndexOut, ".fileIndex");
    char levelInfoOut[1024];    strcpy(levelInfoOut, argv[1]);    	strcat(levelInfoOut, ".levels");

    std::vector<ImportEdge> edgeList;
    NodeID nodeBasedNodeNumber = readBinaryOSRMGraphFromStream(in, edgeList, bollardNodes, trafficLightNodes, &internalToExternaleNodeMapping, inputRestrictions);
    in.close();
    INFO("Loaded " << inputRestrictions.size() << " restrictions, " << bollardNodes.size() << " bollard nodes, " << trafficLightNodes.size() << " traffic lights");

    EdgeBasedGraphFactory * edgeBasedGraphFactory = new EdgeBasedGraphFactory (nodeBasedNodeNumber, edgeList, bollardNodes, trafficLightNodes, inputRestrictions, internalToExternaleNodeMapping, SRTM_ROOT);
    edgeList.clear();
    std::vector<ImportEdge>().swap(edgeList);

    edgeBasedGraphFactory->Run();
    NodeID edgeBasedNodeNumber = edgeBasedGraphFactory->GetNumberOfNodes();
    std::vector<EdgeBasedEdge> edgeBasedEdgeList;
    edgeBasedGraphFactory->GetEdgeBasedEdges(edgeBasedEdgeList);

    std::vector<EdgeBasedGraphFactory::EdgeBasedNode> nodeBasedEdgeList;
    edgeBasedGraphFactory->GetEdgeBasedNodes(nodeBasedEdgeList);
    DELETE(edgeBasedGraphFactory);

    double expansionHasFinishedTime = get_timestamp() - startupTime;

    WritableGrid * writeableGrid = new WritableGrid();
    INFO("building grid ...");
    writeableGrid->ConstructGrid(nodeBasedEdgeList, ramIndexOut, fileIndexOut);
    DELETE( writeableGrid );
    nodeBasedEdgeList.clear();
    std::vector<EdgeBasedGraphFactory::EdgeBasedNode>().swap(nodeBasedEdgeList);

    INFO("writing node map ...");
    std::ofstream mapOutFile(nodeOut, ios::binary);
    BOOST_FOREACH(NodeInfo & info, internalToExternaleNodeMapping) {
        mapOutFile.write((char *)&(info), sizeof(NodeInfo));
    }
    mapOutFile.close();
    internalToExternaleNodeMapping.clear();
    std::vector<NodeInfo>().swap(internalToExternaleNodeMapping);
    inputRestrictions.clear();
    std::vector<_Restriction>().swap(inputRestrictions);

    INFO("initializing contractor");
    Contractor* contractor = new Contractor( edgeBasedNodeNumber, edgeBasedEdgeList );
    double contractionStartedTimestamp(get_timestamp());
    contractor->Run();
    INFO("Contraction took " << get_timestamp() - contractionStartedTimestamp << " sec");

    std::vector< ContractionCleanup::Edge > contractedEdges;
    contractor->GetEdges( contractedEdges );
    delete contractor;

    ContractionCleanup * cleanup = new ContractionCleanup(edgeBasedNodeNumber, contractedEdges);
    contractedEdges.clear();
    std::vector<ContractionCleanup::Edge>().swap(contractedEdges);
    cleanup->Run();

    std::vector< InputEdge> cleanedEdgeList;
    cleanup->GetData(cleanedEdgeList);
    DELETE( cleanup );

    INFO("Building Node Array");
    sort(cleanedEdgeList.begin(), cleanedEdgeList.end());
    unsigned numberOfNodes = 0;
    unsigned numberOfEdges = cleanedEdgeList.size();
    INFO("Serializing compacted graph");
    ofstream edgeOutFile(edgeOut, ios::binary);

    BOOST_FOREACH(InputEdge & edge, cleanedEdgeList) {
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
        while ( edge < numberOfEdges && cleanedEdgeList[edge].source == node )
            ++edge;
        _nodes[node].firstEdge = position; //=edge
        position += edge - lastEdge; //remove
    }
    //Serialize numberOfNodes, nodes
    edgeOutFile.write((char*) &numberOfNodes, sizeof(unsigned));
    edgeOutFile.write((char*) &_nodes[0], sizeof(StaticGraph<EdgeData>::_StrNode)*(numberOfNodes+1));

    //Serialize number of Edges
    edgeOutFile.write((char*) &position, sizeof(unsigned));

    edge = 0;
    int usedEdgeCounter = 0;
    StaticGraph<EdgeData>::_StrEdge currentEdge;
    for ( StaticGraph<EdgeData>::NodeIterator node = 0; node < numberOfNodes; ++node ) {
        for ( StaticGraph<EdgeData>::EdgeIterator i = _nodes[node].firstEdge, e = _nodes[node+1].firstEdge; i != e; ++i ) {

            currentEdge.target = cleanedEdgeList[edge].target;
            currentEdge.data = cleanedEdgeList[edge].data;
            if(currentEdge.data.distance <= 0) {
                INFO("Edge: " << i << ",source: " << cleanedEdgeList[edge].source << ", target: " << cleanedEdgeList[edge].target << ", dist: " << currentEdge.data.distance);
                ERR("Failed at edges of node " << node << " of " << numberOfNodes);
            }
            //Serialize edges
            edgeOutFile.write((char*) &currentEdge, sizeof(StaticGraph<EdgeData>::_StrEdge));
            ++edge;
            ++usedEdgeCounter;
        }
    }
    double endTime = (get_timestamp() - startupTime);
    INFO("Expansion  : " << (nodeBasedNodeNumber/expansionHasFinishedTime) << " nodes/sec and "<< (edgeBasedNodeNumber/expansionHasFinishedTime) << " edges/sec");
    INFO("Contraction: " << (edgeBasedNodeNumber/expansionHasFinishedTime) << " nodes/sec and "<< usedEdgeCounter/endTime << " edges/sec");

    edgeOutFile.close();
    cleanedEdgeList.clear();
    _nodes.clear();
    INFO("finished preprocessing");
    return 0;
}
