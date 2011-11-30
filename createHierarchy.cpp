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

int main (int argc, char *argv[]) {
    if(argc < 3) {
        ERR("usage: " << std::endl << argv[0] << " <osrm-data> <osrm-restrictions>");
    }
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
    INFO("Loaded " << inputRestrictions.size() << " restrictions from file");

    unsigned numberOfThreads = omp_get_num_procs();
    if(testDataFile("contractor.ini")) {
        ContractorConfiguration contractorConfig("contractor.ini");
        if(atoi(contractorConfig.GetParameter("Threads").c_str()) != 0 && (unsigned)atoi(contractorConfig.GetParameter("Threads").c_str()) <= numberOfThreads)
            numberOfThreads = (unsigned)atoi( contractorConfig.GetParameter("Threads").c_str() );
    }
    omp_set_num_threads(numberOfThreads);

    INFO("preprocessing data from input file " << argv[1] << " using STL"
#ifdef _GLIBCXX_PARALLEL
    "parallel (GCC)"
#else
    "serial"
#endif
    " mode");
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
    NodeID n = readBinaryOSRMGraphFromStream(in, edgeList, &internalToExternaleNodeMapping, inputRestrictions);
    in.close();

    EdgeBasedGraphFactory * edgeBasedGraphFactory = new EdgeBasedGraphFactory (n, edgeList, inputRestrictions, internalToExternaleNodeMapping);
    edgeList.clear();
    std::vector<ImportEdge>().swap(edgeList);

    edgeBasedGraphFactory->Run();
    n = edgeBasedGraphFactory->GetNumberOfNodes();
    std::vector<EdgeBasedEdge> edgeBasedEdgeList;
    edgeBasedGraphFactory->GetEdgeBasedEdges(edgeBasedEdgeList);

    std::vector<EdgeBasedGraphFactory::EdgeBasedNode> nodeBasedEdgeList;
    edgeBasedGraphFactory->GetEdgeBasedNodes(nodeBasedEdgeList);
    DELETE(edgeBasedGraphFactory);

    WritableGrid * writeableGrid = new WritableGrid();
    INFO("building grid ...");
    writeableGrid->ConstructGrid(nodeBasedEdgeList, &internalToExternaleNodeMapping, ramIndexOut, fileIndexOut);
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
    Contractor* contractor = new Contractor( n, edgeBasedEdgeList );
    double contractionStartedTimestamp(get_timestamp());
    contractor->Run();
    INFO("Contraction took " << get_timestamp() - contractionStartedTimestamp << " sec");

    std::vector< ContractionCleanup::Edge > contractedEdges;
    contractor->GetEdges( contractedEdges );

    ContractionCleanup * cleanup = new ContractionCleanup(n, contractedEdges);
    contractedEdges.clear();
    std::vector<ContractionCleanup::Edge>().swap(contractedEdges);
    cleanup->Run();

    std::vector< InputEdge> cleanedEdgeList;
    cleanup->GetData(cleanedEdgeList);
    DELETE( cleanup );

    INFO("Serializing edges ");
    ofstream edgeOutFile(edgeOut, ios::binary);
    Percent p(cleanedEdgeList.size());
    BOOST_FOREACH(InputEdge & edge, cleanedEdgeList) {
        p.printIncrement();
        edgeOutFile.write((char *)&(edge.data), sizeof(EdgeData));
        edgeOutFile.write((char *)&(edge.source), sizeof(NodeID));
        edgeOutFile.write((char *)&(edge.target), sizeof(NodeID));
    }
    edgeOutFile.close();
    cleanedEdgeList.clear();

    INFO("finished preprocessing");
    return 0;
}
