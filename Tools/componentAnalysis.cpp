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

#include <fstream>
#include <istream>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

#include "../typedefs.h"
#include "../Algorithms/StronglyConnectedComponents.h"
#include "../DataStructures/BinaryHeap.h"
#include "../DataStructures/DeallocatingVector.h"
#include "../DataStructures/DynamicGraph.h"
#include "../DataStructures/QueryEdge.h"
#include "../DataStructures/TurnInstructions.h"
#include "../Util/BaseConfiguration.h"
#include "../Util/InputFileUtil.h"
#include "../Util/GraphLoader.h"

using namespace std;

typedef QueryEdge::EdgeData EdgeData;
typedef DynamicGraph<EdgeData>::InputEdge InputEdge;
typedef BaseConfiguration ContractorConfiguration;

std::vector<NodeInfo> internalToExternalNodeMapping;
std::vector<_Restriction> inputRestrictions;
std::vector<NodeID> bollardNodes;
std::vector<NodeID> trafficLightNodes;

int main (int argc, char *argv[]) {
    if(argc < 3) {
        ERR("usage: " << std::endl << argv[0] << " <osrm-data> <osrm-restrictions>");
    }
    std::string SRTM_ROOT;

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

    std::vector<ImportEdge> edgeList;
    NodeID nodeBasedNodeNumber = readBinaryOSRMGraphFromStream(in, edgeList, bollardNodes, trafficLightNodes, &internalToExternalNodeMapping, inputRestrictions);
    in.close();
    INFO(inputRestrictions.size() << " restrictions, " << bollardNodes.size() << " bollard nodes, " << trafficLightNodes.size() << " traffic lights");

    /***
     * Building an edge-expanded graph from node-based input an turn restrictions
     */

    INFO("Starting SCC graph traversal");
    TarjanSCC * tarjan = new TarjanSCC (nodeBasedNodeNumber, edgeList, bollardNodes, trafficLightNodes, inputRestrictions, internalToExternalNodeMapping);
    std::vector<ImportEdge>().swap(edgeList);
    tarjan->Run();
    std::vector<_Restriction>().swap(inputRestrictions);
    std::vector<NodeID>().swap(bollardNodes);
    std::vector<NodeID>().swap(trafficLightNodes);
    INFO("finished component analysis");
    return 0;
}
