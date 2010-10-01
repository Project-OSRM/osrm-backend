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

//g++ createHierarchy.cpp -fopenmp -Wno-deprecated -o createHierarchy -O3 -march=native -DNDEBUG

#define VERBOSE(x) x
#define VERBOSE2(x)

#ifdef NDEBUG
#undef VERBOSE
#undef VERBOSE2
#endif

#include <climits>
#include <fstream>
#include <istream>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "typedefs.h"
#include "Contractor/GraphLoader.h"
#include "Contractor/BinaryHeap.h"
#include "Contractor/Contractor.h"
#include "Contractor/ContractionCleanup.h"
#include "DataStructures/NNGrid.h"

using namespace std;

typedef ContractionCleanup::Edge::EdgeData EdgeData;
typedef DynamicGraph<EdgeData>::InputEdge GridEdge;
typedef NNGrid::NNGrid<true> WritableGrid;

vector<NodeInfo> * int2ExtNodeMap = new vector<NodeInfo>();

int main (int argc, char *argv[])
{
    if(argc <= 1)
    {
        cerr << "usage: " << endl << argv[0] << " <osmr-data>" << endl;
        exit(-1);
    }
    cout << "preprocessing data from input file " << argv[1] << endl;

    ifstream in;
    in.open (argv[1]);
    if (!in.is_open()) {
        cerr << "Cannot open " << argv[1] << endl; exit(-1);
    }
    vector<ImportEdge> edgeList;
    const NodeID n = readOSRMGraphFromStream(in, edgeList, int2ExtNodeMap);
    in.close();

    char nodeOut[1024];
    char edgeOut[1024];
    char ramIndexOut[1024];
    char fileIndexOut[1024];
    strcpy(nodeOut, argv[1]);
    strcpy(edgeOut, argv[1]);
    strcpy(ramIndexOut, argv[1]);
    strcpy(fileIndexOut, argv[1]);

    strcat(nodeOut, ".nodes");
    strcat(edgeOut, ".hsgr");
    strcat(ramIndexOut, ".ramIndex");
    strcat(fileIndexOut, ".fileIndex");
    ofstream mapOutFile(nodeOut, ios::binary);

    WritableGrid * g = new WritableGrid();
    cout << "building grid ..." << flush;
    Percent p(edgeList.size());
    for(NodeID i = 0; i < edgeList.size(); i++)
    {
        p.printIncrement();
        if(!edgeList[i].isLocatable())
            continue;
        int slat = int2ExtNodeMap->at(edgeList[i].source()).lat;
        int slon = int2ExtNodeMap->at(edgeList[i].source()).lon;
        int tlat = int2ExtNodeMap->at(edgeList[i].target()).lat;
        int tlon = int2ExtNodeMap->at(edgeList[i].target()).lon;
        g->AddEdge(
                _Edge(
                        edgeList[i].source(),
                        edgeList[i].target(),
                        0,
                        ((edgeList[i].isBackward() && edgeList[i].isForward()) ? 0 : 1),
                        edgeList[i].weight()
                ),

                _Coordinate(slat, slon),
                _Coordinate(tlat, tlon)
        );
    }
    g->ConstructGrid(ramIndexOut, fileIndexOut);
    delete g;

    //Serializing the node map.
    for(NodeID i = 0; i < int2ExtNodeMap->size(); i++)
    {
        mapOutFile.write((char *)&(int2ExtNodeMap->at(i)), sizeof(NodeInfo));
    }
    mapOutFile.close();
    int2ExtNodeMap->clear();
    delete int2ExtNodeMap;

    cout << "initializing contractor ..." << flush;
    Contractor* contractor = new Contractor( n, edgeList );
    vector<ImportEdge>(edgeList.begin(), edgeList.end()).swap(edgeList); //remove excess candidates.
    contractor->Run();

    cout << "checking data sanity ..." << flush;
    contractor->CheckForAllOrigEdges(edgeList);
    cout << "ok" << endl;
    std::vector< ContractionCleanup::Edge > contractedEdges;
    contractor->GetEdges( contractedEdges );

    ContractionCleanup * cleanup = new ContractionCleanup(n, contractedEdges);
    cleanup->Run();

    std::vector< GridEdge> cleanedEdgeList;
    cleanup->GetData(cleanedEdgeList);

    cout << "computing turn vector info ..." << flush;
    contractor->ComputeTurnInfoVector(cleanedEdgeList);
    cout << "ok" << endl;

    ofstream edgeOutFile(edgeOut, ios::binary);

    //Serializing the edge list.
    cout << "Serializing edges " << flush;
    p.reinit(cleanedEdgeList.size());
    for(std::vector< GridEdge>::iterator it = cleanedEdgeList.begin(); it != cleanedEdgeList.end(); it++)
    {
        p.printIncrement();
        int distance= it->data.distance;
        assert(distance > 0);
        bool shortcut= it->data.shortcut;
        bool forward= it->data.forward;
        bool backward= it->data.backward;
        NodeID middle;
        if(shortcut)
            middle = it->data.middleName.middle;
        else {
            middle = it->data.middleName.nameID;
            assert (middle < 10000);
        }

        NodeID source = it->source;
        NodeID target = it->target;
        short type = it->data.type;

        bool forwardTurn = it->data.forwardTurn;
        bool backwardTurn = it->data.backwardTurn;

        edgeOutFile.write((char *)&(distance), sizeof(int));
        edgeOutFile.write((char *)&(forwardTurn), sizeof(bool));
        edgeOutFile.write((char *)&(backwardTurn), sizeof(bool));
        edgeOutFile.write((char *)&(shortcut), sizeof(bool));
        edgeOutFile.write((char *)&(forward), sizeof(bool));
        edgeOutFile.write((char *)&(backward), sizeof(bool));
        edgeOutFile.write((char *)&(middle), sizeof(NodeID));
        edgeOutFile.write((char *)&(type), sizeof(short));
        edgeOutFile.write((char *)&(source), sizeof(NodeID));
        edgeOutFile.write((char *)&(target), sizeof(NodeID));
    }
    edgeOutFile.close();
    cleanedEdgeList.clear();

    delete cleanup;
    delete contractor;
    cout << "finished" << endl;
}
