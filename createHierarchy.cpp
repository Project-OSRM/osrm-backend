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

#define VERBOSE(x) x
#define VERBOSE2(x)

#ifdef NDEBUG
#undef VERBOSE
#undef VERBOSE2
#endif

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <fstream>
#include <istream>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

#include "Algorithms/CRC32.h"
#include "Util/OpenMPReplacement.h"
#include "typedefs.h"
#include "Contractor/Contractor.h"
#include "Contractor/EdgeBasedGraphFactory.h"
#include "DataStructures/BinaryHeap.h"
#include "DataStructures/DeallocatingVector.h"
#include "DataStructures/ExtractorStructs.h"
#include "DataStructures/NNGrid.h"
#include "DataStructures/QueryEdge.h"
#include "Util/BaseConfiguration.h"
#include "Util/InputFileUtil.h"
#include "Util/GraphLoader.h"
#include "Util/OpenMPReplacement.h"

using namespace std;

typedef QueryEdge::EdgeData EdgeData;
typedef DynamicGraph<EdgeData>::InputEdge InputEdge;
typedef StaticGraph<EdgeData>::InputEdge StaticEdge;
typedef BaseConfiguration ContractorConfiguration;

std::vector<NodeInfo> internalToExternalNodeMapping;
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

    INFO("Using restrictions from file: " << argv[2]);
    std::ifstream restrictionsInstream(argv[2], ios::binary);
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

    char nodeOut[1024];         strcpy(nodeOut, argv[1]);           strcat(nodeOut, ".nodes");
    char edgeOut[1024];         strcpy(edgeOut, argv[1]);           strcat(edgeOut, ".edges");
    char graphOut[1024];    	strcpy(graphOut, argv[1]);      	strcat(graphOut, ".hsgr");
    char ramIndexOut[1024];    	strcpy(ramIndexOut, argv[1]);    	strcat(ramIndexOut, ".ramIndex");
    char fileIndexOut[1024];    strcpy(fileIndexOut, argv[1]);    	strcat(fileIndexOut, ".fileIndex");
    char levelInfoOut[1024];    strcpy(levelInfoOut, argv[1]);    	strcat(levelInfoOut, ".levels");

    std::vector<ImportEdge> edgeList;
    NodeID nodeBasedNodeNumber = readBinaryOSRMGraphFromStream(in, edgeList, bollardNodes, trafficLightNodes, &internalToExternalNodeMapping, inputRestrictions);
    in.close();
    INFO(inputRestrictions.size() << " restrictions, " << bollardNodes.size() << " bollard nodes, " << trafficLightNodes.size() << " traffic lights");

    if(!testDataFile("speedprofile.ini")) {
        ERR("Need speedprofile.ini to apply traffic signal penalty");
    }
    boost::property_tree::ptree speedProfile;
    boost::property_tree::ini_parser::read_ini("speedprofile.ini", speedProfile);

    /***
     * Building an edge-expanded graph from node-based input an turn restrictions
     */

    INFO("Generating edge-expanded graph representation");
    EdgeBasedGraphFactory * edgeBasedGraphFactory = new EdgeBasedGraphFactory (nodeBasedNodeNumber, edgeList, bollardNodes, trafficLightNodes, inputRestrictions, internalToExternalNodeMapping, speedProfile, SRTM_ROOT);
    std::vector<ImportEdge>().swap(edgeList);
    edgeBasedGraphFactory->Run(edgeOut);
    std::vector<_Restriction>().swap(inputRestrictions);
    std::vector<NodeID>().swap(bollardNodes);
    std::vector<NodeID>().swap(trafficLightNodes);
    NodeID edgeBasedNodeNumber = edgeBasedGraphFactory->GetNumberOfNodes();
    DeallocatingVector<EdgeBasedEdge> edgeBasedEdgeList;
    edgeBasedGraphFactory->GetEdgeBasedEdges(edgeBasedEdgeList);

    /***
     * Writing info on original (node-based) nodes
     */

    INFO("writing node map ...");
    std::ofstream mapOutFile(nodeOut, std::ios::binary);
    mapOutFile.write((char *)&(internalToExternalNodeMapping[0]), internalToExternalNodeMapping.size()*sizeof(NodeInfo));
    mapOutFile.close();
    std::vector<NodeInfo>().swap(internalToExternalNodeMapping);

    /***
     * Writing info on original (node-based) edges
     */
    INFO("writing info on original edges");
    std::vector<OriginalEdgeData> originalEdgeData;
    edgeBasedGraphFactory->GetOriginalEdgeData(originalEdgeData);

//    std::ofstream oedOutFile(edgeOut, std::ios::binary);
//    unsigned numberOfOrigEdges = originalEdgeData.size();
//    oedOutFile.write((char*)&numberOfOrigEdges, sizeof(unsigned));
//    oedOutFile.write((char*)&(originalEdgeData[0]), originalEdgeData.size()*sizeof(OriginalEdgeData));
//    oedOutFile.close();
//    std::vector<OriginalEdgeData>().swap(originalEdgeData);

    std::vector<EdgeBasedGraphFactory::EdgeBasedNode> nodeBasedEdgeList;
    edgeBasedGraphFactory->GetEdgeBasedNodes(nodeBasedEdgeList);
    delete edgeBasedGraphFactory;
    double expansionHasFinishedTime = get_timestamp() - startupTime;

    /***
     * Building grid-like nearest-neighbor data structure
     */

    INFO("building grid ...");
    WritableGrid * writeableGrid = new WritableGrid();
    writeableGrid->ConstructGrid(nodeBasedEdgeList, ramIndexOut, fileIndexOut);
    delete writeableGrid;
    CRC32 crc32;
    unsigned crc32OfNodeBasedEdgeList = crc32((char *)&(nodeBasedEdgeList[0]), nodeBasedEdgeList.size()*sizeof(EdgeBasedGraphFactory::EdgeBasedNode));
    std::vector<EdgeBasedGraphFactory::EdgeBasedNode>().swap(nodeBasedEdgeList);

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
    sort(contractedEdgeList.begin(), contractedEdgeList.end());
    unsigned numberOfNodes = 0;
    unsigned numberOfEdges = contractedEdgeList.size();
    INFO("Serializing compacted graph");
    ofstream edgeOutFile(graphOut, ios::binary);

    BOOST_FOREACH(QueryEdge & edge, contractedEdgeList) {
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
