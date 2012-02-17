/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

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

#ifndef GRAPHLOADER_H
#define GRAPHLOADER_H

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>

#include <boost/unordered_map.hpp>

#ifdef _GLIBCXX_PARALLEL
#include <parallel/algorithm>
#else
#include <algorithm>
#endif

#include "../DataStructures/ImportEdge.h"
#include "../typedefs.h"

typedef boost::unordered_map<NodeID, NodeID> ExternalNodeMap;

template<typename EdgeT>
NodeID readOSRMGraphFromStream(istream &in, vector<EdgeT>& edgeList, vector<NodeInfo> * int2ExtNodeMap) {
    NodeID n, source, target, id;
    EdgeID m;
    int dir, xcoord, ycoord;// direction (0 = open, 1 = forward, 2+ = open)
    ExternalNodeMap ext2IntNodeMap;
    in >> n;
    DEBUG("Importing n = " << n << " nodes ");
    for (NodeID i=0; i < n; ++i) {
        in >> id >> ycoord >> xcoord;
        int2ExtNodeMap->push_back(NodeInfo(xcoord, ycoord, id));
        ext2IntNodeMap.insert(make_pair(id, i));
    }
    in >> m;
    DEBUG(" and " << m << " edges ...");

    edgeList.reserve(m);
    for (EdgeID i=0; i<m; ++i) {
        EdgeWeight weight;
        short type;
        NodeID nameID;
        int length;
        in >> source >> target >> length >> dir >> weight >> type >> nameID;
        assert(length > 0);
        assert(weight > 0);
        assert(0<=dir && dir<=2);

        bool forward = true;
        bool backward = true;
        if (1 == dir) backward = false;
        if (2 == dir) forward = false;

        if(length == 0) { ERR("loaded null length edge"); }

        //         translate the external NodeIDs to internal IDs
        ExternalNodeMap::iterator intNodeID = ext2IntNodeMap.find(source);
        if( ext2IntNodeMap.find(source) == ext2IntNodeMap.end()) {
            ERR("after " << edgeList.size() << " edges" << "\n->" << source << "," << target << "," << length << "," << dir << "," << weight << "\n->unresolved source NodeID: " << source );
        }
        source = intNodeID->second;
        intNodeID = ext2IntNodeMap.find(target);
        if(ext2IntNodeMap.find(target) == ext2IntNodeMap.end()) { ERR("unresolved target NodeID : " << target); }
        target = intNodeID->second;

        if(source == UINT_MAX || target == UINT_MAX) { ERR( "nonexisting source or target" ); }

        EdgeT inputEdge(source, target, nameID, weight, forward, backward, type );
        edgeList.push_back(inputEdge);
    }
    ext2IntNodeMap.clear();
    vector<ImportEdge>(edgeList.begin(), edgeList.end()).swap(edgeList); //remove excess candidates.
    cout << "ok" << endl;
    return n;
}
template<typename EdgeT>
NodeID readBinaryOSRMGraphFromStream(std::istream &in, std::vector<EdgeT>& edgeList, std::vector<NodeID> &bollardNodes, std::vector<NodeID> &trafficLightNodes, std::vector<NodeInfo> * int2ExtNodeMap, std::vector<_Restriction> & inputRestrictions) {
    NodeID n, source, target;
    EdgeID m;
    short dir;// direction (0 = open, 1 = forward, 2+ = open)
    ExternalNodeMap ext2IntNodeMap;
    in.read((char*)&n, sizeof(NodeID));
    DEBUG("Importing n = " << n << " nodes ");
    _Node node;
    for (NodeID i=0; i<n; ++i) {
        in.read((char*)&node, sizeof(_Node));
        int2ExtNodeMap->push_back(NodeInfo(node.lat, node.lon, node.id));
        ext2IntNodeMap.insert(make_pair(node.id, i));
        if(node.bollard)
        	bollardNodes.push_back(i);
        if(node.trafficLight)
        	trafficLightNodes.push_back(i);
    }

    in.read((char*)&m, sizeof(unsigned));
    DEBUG(" and " << m << " edges ");
    for(unsigned i = 0; i < inputRestrictions.size(); ++i) {
        ExternalNodeMap::iterator intNodeID = ext2IntNodeMap.find(inputRestrictions[i].fromNode);
        if( intNodeID == ext2IntNodeMap.end()) {
            DEBUG("Unmapped from Node of restriction");
            continue;

        }
        inputRestrictions[i].fromNode = intNodeID->second;

        intNodeID = ext2IntNodeMap.find(inputRestrictions[i].viaNode);
        if( intNodeID == ext2IntNodeMap.end()) {
            DEBUG("Unmapped via node of restriction");
            continue;
        }
        inputRestrictions[i].viaNode = intNodeID->second;

        intNodeID = ext2IntNodeMap.find(inputRestrictions[i].toNode);
        if( intNodeID == ext2IntNodeMap.end()) {
            DEBUG("Unmapped to node of restriction");
            continue;
        }
        inputRestrictions[i].toNode = intNodeID->second;
    }

    edgeList.reserve(m);
    EdgeWeight weight;
    short type;
    NodeID nameID;
    int length;
    bool isRoundabout, ignoreInGrid;

    for (EdgeID i=0; i<m; ++i) {
        in.read((char*)&source,         sizeof(unsigned));
        in.read((char*)&target,         sizeof(unsigned));
        in.read((char*)&length,         sizeof(int));
        in.read((char*)&dir,            sizeof(short));
        in.read((char*)&weight,         sizeof(int));
        in.read((char*)&type,           sizeof(short));
        in.read((char*)&nameID,         sizeof(unsigned));
        in.read((char*)&isRoundabout,   sizeof(bool));
        in.read((char*)&ignoreInGrid,   sizeof(bool));

        GUARANTEE(length > 0, "loaded null length edge" );
        GUARANTEE(weight > 0, "loaded null weight");
        GUARANTEE(0<=dir && dir<=2, "loaded bogus direction");

        bool forward = true;
        bool backward = true;
        if (1 == dir) { backward = false; }
        if (2 == dir) { forward = false; }

        assert(type >= 0);

        //         translate the external NodeIDs to internal IDs
        ExternalNodeMap::iterator intNodeID = ext2IntNodeMap.find(source);
        if( ext2IntNodeMap.find(source) == ext2IntNodeMap.end()) {
#ifndef NDEBUG
            WARN(" unresolved source NodeID: " << source );
#endif
            continue;
        }
        source = intNodeID->second;
        intNodeID = ext2IntNodeMap.find(target);
        if(ext2IntNodeMap.find(target) == ext2IntNodeMap.end()) {
#ifndef NDEBUG
            WARN("unresolved target NodeID : " << target );
#endif
            continue;
        }
        target = intNodeID->second;
        GUARANTEE(source != UINT_MAX && target != UINT_MAX, "nonexisting source or target");

        EdgeT inputEdge(source, target, nameID, weight, forward, backward, type, isRoundabout, ignoreInGrid );
        edgeList.push_back(inputEdge);
    }
    ext2IntNodeMap.clear();
    vector<ImportEdge>(edgeList.begin(), edgeList.end()).swap(edgeList); //remove excess candidates.
    INFO("Graph loaded ok");
    return n;
}
template<typename EdgeT>
NodeID readDTMPGraphFromStream(istream &in, vector<EdgeT>& edgeList, vector<NodeInfo> * int2ExtNodeMap) {
    NodeID n, source, target, id;
    EdgeID m;
    int dir, xcoord, ycoord;// direction (0 = open, 1 = forward, 2+ = open)
    ExternalNodeMap ext2IntNodeMap;
    in >> n;
    DEBUG("Importing n = " << n << " nodes ");
    for (NodeID i=0; i<n;++i) {
        in >> id >> ycoord >> xcoord;
        int2ExtNodeMap->push_back(NodeInfo(xcoord, ycoord, id));
        ext2IntNodeMap.insert(make_pair(id, i));
    }
    in >> m;
    DEBUG(" and " << m << " edges");

    edgeList.reserve(m);
    for (EdgeID i=0; i<m; ++i) {
        EdgeWeight weight;
        unsigned speedType(0);
        short type(0);
        // NodeID nameID;
        int length;
        in >> source >> target >> length >> dir >> speedType;

        if(dir == 3)
            dir = 0;

        switch(speedType) {
        case 1:
            weight = 130;
            break;
        case 2:
            weight = 120;
            break;
        case 3:
            weight = 110;
            break;
        case 4:
            weight = 100;
            break;
        case 5:
            weight = 90;
            break;
        case 6:
            weight = 80;
            break;
        case 7:
            weight = 70;
            break;
        case 8:
            weight = 60;
            break;
        case 9:
            weight = 50;
            break;
        case 10:
            weight = 40;
            break;
        case 11:
            weight = 30;
            break;
        case 12:
            weight = 20;
            break;
        case 13:
            weight = length;
            break;
        case 15:
            weight = 10;
            break;
        default:
            weight = 0;
            break;
        }

        weight = length*weight/3.6;
        if(speedType == 13)
            weight = length;
        assert(length > 0);
        assert(weight > 0);
        if(dir <0 || dir > 2)
            WARN("direction bogus: " << dir);
        assert(0<=dir && dir<=2);

        bool forward = true;
        bool backward = true;
        if (dir == 1) backward = false;
        if (dir == 2) forward = false;

        if(length == 0) { ERR("loaded null length edge"); }

        //         translate the external NodeIDs to internal IDs
        ExternalNodeMap::iterator intNodeID = ext2IntNodeMap.find(source);
        if( ext2IntNodeMap.find(source) == ext2IntNodeMap.end()) {
            ERR("after " << edgeList.size() << " edges" << "\n->" << source << "," << target << "," << length << "," << dir << "," << weight << "\n->unresolved source NodeID: " << source);
        }
        source = intNodeID->second;
        intNodeID = ext2IntNodeMap.find(target);
        if(ext2IntNodeMap.find(target) == ext2IntNodeMap.end()) { ERR("unresolved target NodeID : " << target); }
        target = intNodeID->second;

        if(source == UINT_MAX || target == UINT_MAX) { ERR("nonexisting source or target" ); }

        EdgeT inputEdge(source, target, 0, weight, forward, backward, type );
        edgeList.push_back(inputEdge);
    }
    ext2IntNodeMap.clear();
    vector<ImportEdge>(edgeList.begin(), edgeList.end()).swap(edgeList); //remove excess candidates.
    cout << "ok" << endl;
    return n;
}

template<typename EdgeT>
NodeID readDDSGGraphFromStream(istream &in, vector<EdgeT>& edgeList, vector<NodeID> & int2ExtNodeMap) {
    ExternalNodeMap nodeMap;
    NodeID n, source, target;
    unsigned numberOfNodes = 0;
    char d;
    EdgeID m;
    int dir;// direction (0 = open, 1 = forward, 2+ = open)
    in >> d;
    in >> n;
    in >> m;
#ifndef DEBUG
    std::cout << "expecting " << n << " nodes and " << m << " edges ..." << flush;
#endif
    edgeList.reserve(m);
    for (EdgeID i=0; i<m; i++) {
        EdgeWeight weight;
        in >> source >> target >> weight >> dir;

        assert(weight > 0);
        if(dir <0 || dir > 3)
            ERR( "[error] direction bogus: " << dir );
        assert(0<=dir && dir<=3);

        bool forward = true;
        bool backward = true;
        if (dir == 1) backward = false;
        if (dir == 2) forward = false;
        if (dir == 3) {backward = true; forward = true;}

        if(weight == 0) { ERR("loaded null length edge"); }

        if( nodeMap.find(source) == nodeMap.end()) {
            nodeMap.insert(std::make_pair(source, numberOfNodes ));
            int2ExtNodeMap.push_back(source);
            numberOfNodes++;
        }
        if( nodeMap.find(target) == nodeMap.end()) {
            nodeMap.insert(std::make_pair(target, numberOfNodes));
            int2ExtNodeMap.push_back(target);
            numberOfNodes++;
        }
        EdgeT inputEdge(source, target, 0, weight, forward, backward, 1 );
        edgeList.push_back(inputEdge);
    }
    vector<EdgeT>(edgeList.begin(), edgeList.end()).swap(edgeList); //remove excess candidates.

    nodeMap.clear();
    return numberOfNodes;
}

template<typename NodeT, typename EdgeT>
unsigned readHSGRFromStream(istream &in, vector<NodeT>& nodeList, vector<EdgeT> & edgeList, unsigned * checkSum) {
    unsigned numberOfNodes = 0;
    in.read((char*) checkSum, sizeof(unsigned));
    in.read((char*) & numberOfNodes, sizeof(unsigned));
    nodeList.resize(numberOfNodes + 1);
    NodeT currentNode;
    for(unsigned nodeCounter = 0; nodeCounter <= numberOfNodes; ++nodeCounter ) {
        in.read((char*) &currentNode, sizeof(NodeT));
        nodeList[nodeCounter] = currentNode;
    }

    unsigned numberOfEdges = 0;
    in.read((char*) &numberOfEdges, sizeof(unsigned));
    edgeList.resize(numberOfEdges);
    EdgeT currentEdge;
    for(unsigned edgeCounter = 0; edgeCounter < numberOfEdges; ++edgeCounter) {
        in.read((char*) &currentEdge, sizeof(EdgeT));
        edgeList[edgeCounter] = currentEdge;
    }

    return numberOfNodes;
}

template<typename EdgeT>
unsigned readHSGRFromStream(istream &in, vector<EdgeT> & edgeList) {
    NodeID numberOfNodes = 0;
    do {
        EdgeT g;
        EdgeData e;
        NodeID source;
        NodeID target;

        in.read((char *)&(e), sizeof(EdgeData));
        if(!in.good())
            break;
        assert(e.distance > 0);
        in.read((char *)&(source), sizeof(NodeID));
        in.read((char *)&(target), sizeof(NodeID));
        g.data = e;
        g.source = source; g.target = target;

        if(source > numberOfNodes) {
            numberOfNodes = source;
        }
        if(target > numberOfNodes) {
            numberOfNodes = target;
        }
        edgeList.push_back(g);
    }  while(true);

    return numberOfNodes+1;
}

#endif // GRAPHLOADER_H
