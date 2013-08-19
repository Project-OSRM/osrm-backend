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

#include "OSRMException.h"
#include "../DataStructures/ImportNode.h"
#include "../DataStructures/ImportEdge.h"
#include "../DataStructures/QueryNode.h"
#include "../DataStructures/Restriction.h"
#include "../Util/SimpleLogger.h"
#include "../Util/UUID.h"
#include "../typedefs.h"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/unordered_map.hpp>

#include <cassert>
#include <cmath>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>

typedef boost::unordered_map<NodeID, NodeID> ExternalNodeMap;

template<class EdgeT>
struct _ExcessRemover {
    inline bool operator()( const EdgeT & edge ) const {
        return edge.source() == UINT_MAX;
    }
};

template<typename EdgeT>
NodeID readBinaryOSRMGraphFromStream(
    std::istream &in,
    std::vector<EdgeT>& edgeList,
    std::vector<NodeID> &bollardNodes,
    std::vector<NodeID> &trafficLightNodes,
    std::vector<NodeInfo> * int2ExtNodeMap,
    std::vector<TurnRestriction> & inputRestrictions
) {
    const UUID uuid_orig;
    UUID uuid_loaded;
    in.read((char *) &uuid_loaded, sizeof(UUID));

    if( !uuid_loaded.TestGraphUtil(uuid_orig) ) {
        SimpleLogger().Write(logWARNING) <<
            ".osrm was prepared with different build."
            "Reprocess to get rid of this warning.";
    }

    NodeID n, source, target;
    EdgeID m;
    short dir;// direction (0 = open, 1 = forward, 2+ = open)
    ExternalNodeMap ext_to_int_id_map;
    in.read((char*)&n, sizeof(NodeID));
    SimpleLogger().Write() << "Importing n = " << n << " nodes ";
    _Node node;
    for (NodeID i=0; i<n; ++i) {
        in.read((char*)&node, sizeof(_Node));
        int2ExtNodeMap->push_back(NodeInfo(node.lat, node.lon, node.id));
        ext_to_int_id_map.emplace(node.id, i);
        if(node.bollard) {
        	bollardNodes.push_back(i);
        }
        if(node.trafficLight) {
        	trafficLightNodes.push_back(i);
        }
    }

    //tighten vector sizes
    std::vector<NodeID>(bollardNodes).swap(bollardNodes);
    std::vector<NodeID>(trafficLightNodes).swap(trafficLightNodes);

    in.read((char*)&m, sizeof(unsigned));
    SimpleLogger().Write() << " and " << m << " edges ";
    // for(unsigned i = 0; i < inputRestrictions.size(); ++i) {
    BOOST_FOREACH(TurnRestriction & current_restriction, inputRestrictions) {
        ExternalNodeMap::iterator intNodeID = ext_to_int_id_map.find(current_restriction.fromNode);
        if( intNodeID == ext_to_int_id_map.end()) {
            SimpleLogger().Write(logDEBUG) << "Unmapped from Node of restriction";
            continue;

        }
        current_restriction.fromNode = intNodeID->second;

        intNodeID = ext_to_int_id_map.find(current_restriction.viaNode);
        if( intNodeID == ext_to_int_id_map.end()) {
            SimpleLogger().Write(logDEBUG) << "Unmapped via node of restriction";
            continue;
        }
        current_restriction.viaNode = intNodeID->second;

        intNodeID = ext_to_int_id_map.find(current_restriction.toNode);
        if( intNodeID == ext_to_int_id_map.end()) {
            SimpleLogger().Write(logDEBUG) << "Unmapped to node of restriction";
            continue;
        }
        current_restriction.toNode = intNodeID->second;
    }

    edgeList.reserve(m);
    EdgeWeight weight;
    short type;
    NodeID nameID;
    int length;
    bool isRoundabout, ignoreInGrid, isAccessRestricted, isContraFlow;

    for (EdgeID i=0; i<m; ++i) {
        in.read((char*)&source,             sizeof(unsigned));
        in.read((char*)&target,             sizeof(unsigned));
        in.read((char*)&length,             sizeof(int));
        in.read((char*)&dir,                sizeof(short));
        in.read((char*)&weight,             sizeof(int));
        in.read((char*)&type,               sizeof(short));
        in.read((char*)&nameID,             sizeof(unsigned));
        in.read((char*)&isRoundabout,       sizeof(bool));
        in.read((char*)&ignoreInGrid,       sizeof(bool));
        in.read((char*)&isAccessRestricted, sizeof(bool));
        in.read((char*)&isContraFlow,       sizeof(bool));

        BOOST_ASSERT_MSG(length > 0, "loaded null length edge" );
        BOOST_ASSERT_MSG(weight > 0, "loaded null weight");
        BOOST_ASSERT_MSG(0<=dir && dir<=2, "loaded bogus direction");

        bool forward = true;
        bool backward = true;
        if (1 == dir) { backward = false; }
        if (2 == dir) { forward = false; }

        assert(type >= 0);

        //         translate the external NodeIDs to internal IDs
        ExternalNodeMap::iterator intNodeID = ext_to_int_id_map.find(source);
        if( ext_to_int_id_map.find(source) == ext_to_int_id_map.end()) {
#ifndef NDEBUG
            SimpleLogger().Write(logWARNING) <<
                " unresolved source NodeID: " << source;
#endif
            continue;
        }
        source = intNodeID->second;
        intNodeID = ext_to_int_id_map.find(target);
        if(ext_to_int_id_map.find(target) == ext_to_int_id_map.end()) {
#ifndef NDEBUG
            SimpleLogger().Write(logWARNING) <<
            "unresolved target NodeID : " << target;
#endif
            continue;
        }
        target = intNodeID->second;
        BOOST_ASSERT_MSG(source != UINT_MAX && target != UINT_MAX,
            "nonexisting source or target"
        );

        if(source > target) {
            std::swap(source, target);
            std::swap(forward, backward);
        }

        EdgeT inputEdge(source, target, nameID, weight, forward, backward, type, isRoundabout, ignoreInGrid, isAccessRestricted, isContraFlow );
        edgeList.push_back(inputEdge);
    }
    std::sort(edgeList.begin(), edgeList.end());
    for(unsigned i = 1; i < edgeList.size(); ++i) {
        if( (edgeList[i-1].target() == edgeList[i].target()) && (edgeList[i-1].source() == edgeList[i].source()) ) {
            bool edgeFlagsAreEquivalent = (edgeList[i-1].isForward() == edgeList[i].isForward()) && (edgeList[i-1].isBackward() == edgeList[i].isBackward());
            bool edgeFlagsAreSuperSet1 = (edgeList[i-1].isForward() && edgeList[i-1].isBackward()) && (edgeList[i].isBackward() != edgeList[i].isBackward() );
            bool edgeFlagsAreSuperSet2 = (edgeList[i].isForward() && edgeList[i].isBackward()) && (edgeList[i-1].isBackward() != edgeList[i-1].isBackward() );

            if( edgeFlagsAreEquivalent ) {
                edgeList[i]._weight = std::min(edgeList[i-1].weight(), edgeList[i].weight());
                edgeList[i-1]._source = UINT_MAX;
            } else if (edgeFlagsAreSuperSet1) {
                if(edgeList[i-1].weight() <= edgeList[i].weight()) {
                    //edge i-1 is smaller and goes in both directions. Throw away the other edge
                    edgeList[i]._source = UINT_MAX;
                } else {
                    //edge i-1 is open in both directions, but edge i is smaller in one direction. Close edge i-1 in this direction
                    edgeList[i-1].forward = !edgeList[i].isForward();
                    edgeList[i-1].backward = !edgeList[i].isBackward();
                }
            } else if (edgeFlagsAreSuperSet2) {
                if(edgeList[i-1].weight() <= edgeList[i].weight()) {
                     //edge i-1 is smaller for one direction. edge i is open in both. close edge i in the other direction
                     edgeList[i].forward = !edgeList[i-1].isForward();
                     edgeList[i].backward = !edgeList[i-1].isBackward();
                 } else {
                     //edge i is smaller and goes in both direction. Throw away edge i-1
                     edgeList[i-1]._source = UINT_MAX;
                 }
            }
        }
    }
    typename std::vector<EdgeT>::iterator newEnd = std::remove_if(edgeList.begin(), edgeList.end(), _ExcessRemover<EdgeT>());
    ext_to_int_id_map.clear();
    std::vector<EdgeT>(edgeList.begin(), newEnd).swap(edgeList); //remove excess candidates.
    SimpleLogger().Write() << "Graph loaded ok and has " << edgeList.size() << " edges";
    return n;
}
template<typename EdgeT>
NodeID readDTMPGraphFromStream(std::istream &in, std::vector<EdgeT>& edgeList, std::vector<NodeInfo> * int2ExtNodeMap) {
    NodeID n, source, target, id;
    EdgeID m;
    int dir, xcoord, ycoord;// direction (0 = open, 1 = forward, 2+ = open)
    ExternalNodeMap ext_to_int_id_map;
    in >> n;
    SimpleLogger().Write(logDEBUG) << "Importing n = " << n << " nodes ";
    for (NodeID i=0; i<n; ++i) {
        in >> id >> ycoord >> xcoord;
        int2ExtNodeMap->push_back(NodeInfo(xcoord, ycoord, id));
        ext_to_int_id_map.insert(std::make_pair(id, i));
    }
    in >> m;
    SimpleLogger().Write(logDEBUG) << " and " << m << " edges";

    edgeList.reserve(m);
    for (EdgeID i=0; i<m; ++i) {
        EdgeWeight weight;
        unsigned speedType(0);
        short type(0);
        // NodeID nameID;
        int length;
        in >> source >> target >> length >> dir >> speedType;

        if(dir == 3) {
            dir = 0;
        }
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
        if(speedType == 13) {
            weight = length;
        }
        assert(length > 0);
        assert(weight > 0);
        if(dir <0 || dir > 2) {
            SimpleLogger().Write(logWARNING) << "direction bogus: " << dir;
        }
        assert(0<=dir && dir<=2);

        bool forward = true;
        bool backward = true;
        if (dir == 1) {
            backward = false;
        }
        if (dir == 2) {
            forward = false;
        }

        if(length == 0) {
            throw OSRMException("loaded null length edge");
        }

        //         translate the external NodeIDs to internal IDs
        ExternalNodeMap::iterator intNodeID = ext_to_int_id_map.find(source);
        if( ext_to_int_id_map.find(source) == ext_to_int_id_map.end()) {
            throw OSRMException("unresolvable source Node ID");
        }
        source = intNodeID->second;
        intNodeID = ext_to_int_id_map.find(target);
        if(ext_to_int_id_map.find(target) == ext_to_int_id_map.end()) {
            throw OSRMException("unresolvable target Node ID");
        }
        target = intNodeID->second;

        if(source == UINT_MAX || target == UINT_MAX) {
            throw OSRMException("nonexisting source or target" );
        }

        EdgeT inputEdge(source, target, 0, weight, forward, backward, type );
        edgeList.push_back(inputEdge);
    }
    ext_to_int_id_map.clear();
    std::vector<EdgeT>(edgeList.begin(), edgeList.end()).swap(edgeList); //remove excess candidates.
    std::cout << "ok" << std::endl;
    return n;
}

template<typename EdgeT>
NodeID readDDSGGraphFromStream(std::istream &in, std::vector<EdgeT>& edgeList, std::vector<NodeID> & int2ExtNodeMap) {
    ExternalNodeMap nodeMap;
    NodeID n, source, target;
    unsigned numberOfNodes = 0;
    char d;
    EdgeID m;
    int dir;// direction (0 = open, 1 = forward, 2+ = open)
    in >> d;
    in >> n;
    in >> m;

    SimpleLogger().Write(logDEBUG) <<
        "expecting " << n << " nodes and " << m << " edges ...";

    edgeList.reserve(m);
    for (EdgeID i=0; i<m; i++) {
        EdgeWeight weight;
        in >> source >> target >> weight >> dir;

        assert(weight > 0);
        if(dir <0 || dir > 3) {
            throw OSRMException( "[error] direction bogus");
        }
        assert(0<=dir && dir<=3);

        bool forward = true;
        bool backward = true;
        if (dir == 1) backward = false;
        if (dir == 2) forward = false;
        if (dir == 3) {backward = true; forward = true;}

        if(weight == 0) {
            throw OSRMException("loaded null length edge");
        }

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
    std::vector<EdgeT>(edgeList.begin(), edgeList.end()).swap(edgeList); //remove excess candidates.

    nodeMap.clear();
    return numberOfNodes;
}

template<typename NodeT, typename EdgeT>
unsigned readHSGRFromStream(
    const std::string & hsgr_filename,
    std::vector<NodeT> & node_list,
    std::vector<EdgeT> & edge_list,
    unsigned * check_sum
) {
    boost::filesystem::path hsgr_file(hsgr_filename);
    if ( !boost::filesystem::exists( hsgr_file ) ) {
        throw OSRMException("hsgr file does not exist");
    }
    if ( 0 == boost::filesystem::file_size( hsgr_file ) ) {
        throw OSRMException("hsgr file is empty");
    }

    boost::filesystem::ifstream hsgr_input_stream(hsgr_file, std::ios::binary);

    UUID uuid_loaded, uuid_orig;
    hsgr_input_stream.read((char *)&uuid_loaded, sizeof(UUID));
    if( !uuid_loaded.TestGraphUtil(uuid_orig) ) {
        SimpleLogger().Write(logWARNING) <<
            ".hsgr was prepared with different build. "
            "Reprocess to get rid of this warning.";
    }

    unsigned number_of_nodes = 0;
    hsgr_input_stream.read((char*) check_sum, sizeof(unsigned));
    hsgr_input_stream.read((char*) & number_of_nodes, sizeof(unsigned));
    BOOST_ASSERT_MSG( 0 != number_of_nodes, "number of nodes is zero");
    node_list.resize(number_of_nodes + 1);
    hsgr_input_stream.read(
        (char*) &(node_list[0]),
        number_of_nodes*sizeof(NodeT)
    );
    unsigned number_of_edges = 0;
    hsgr_input_stream.read(
        (char*) &number_of_edges,
        sizeof(unsigned)
    );
    BOOST_ASSERT_MSG( 0 != number_of_edges, "number of edges is zero");

    edge_list.resize(number_of_edges);
    hsgr_input_stream.read(
        (char*) &(edge_list[0]),
        number_of_edges*sizeof(EdgeT)
    );
    hsgr_input_stream.close();
    return number_of_nodes;
}

#endif // GRAPHLOADER_H
