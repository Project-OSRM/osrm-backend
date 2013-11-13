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

#include <cmath>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>

typedef boost::unordered_map<NodeID, NodeID> ExternalNodeMap;

template<class EdgeT>
struct NodesWithoutSourceRemover {
    inline bool operator()( const EdgeT & edge ) const {
        return edge.source() == UINT_MAX;
    }
};

template<typename EdgeT>
NodeID readBinaryOSRMGraphFromStream(
    std::istream                 & input_stream,
    std::vector<EdgeT>           & edge_list,
    std::vector<NodeID>          & barrier_node_list,
    std::vector<NodeID>          & traffic_light_node_list,
    std::vector<NodeInfo>        * int_to_ext_node_id_map,
    std::vector<TurnRestriction> & restriction_list
) {
    const UUID uuid_orig;
    UUID uuid_loaded;
    input_stream.read((char *) &uuid_loaded, sizeof(UUID));

    if( !uuid_loaded.TestGraphUtil(uuid_orig) ) {
        SimpleLogger().Write(logWARNING) <<
            ".osrm was prepared with different build."
            "Reprocess to get rid of this warning.";
    }

    NodeID n, source, target;
    EdgeID m;
    short dir;// direction (0 = open, 1 = forward, 2+ = open)
    ExternalNodeMap ext_to_int_id_map;
    input_stream.read((char*)&n, sizeof(NodeID));
    SimpleLogger().Write() << "Importing n = " << n << " nodes ";
    ExternalMemoryNode node;
    for( NodeID i=0; i<n; ++i ) {
        input_stream.read((char*)&node, sizeof(ExternalMemoryNode));
        int_to_ext_node_id_map->push_back(
            NodeInfo(node.lat, node.lon, node.id)
        );
        ext_to_int_id_map.emplace(node.id, i);
        if(node.bollard) {
        	barrier_node_list.push_back(i);
        }
        if(node.trafficLight) {
        	traffic_light_node_list.push_back(i);
        }
    }

    //tighten vector sizes
    std::vector<NodeID>(barrier_node_list).swap(barrier_node_list);
    std::vector<NodeID>(traffic_light_node_list).swap(traffic_light_node_list);

    input_stream.read((char*)&m, sizeof(unsigned));
    SimpleLogger().Write() << " and " << m << " edges ";
    // for(unsigned i = 0; i < restriction_list.size(); ++i) {
    BOOST_FOREACH(TurnRestriction & current_restriction, restriction_list) {
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

    edge_list.reserve(m);
    EdgeWeight weight;
    short type;
    NodeID nameID;
    int length;
    bool isRoundabout, ignoreInGrid, isAccessRestricted, isContraFlow;

    for (EdgeID i=0; i<m; ++i) {
        input_stream.read((char*)&source,             sizeof(unsigned));
        input_stream.read((char*)&target,             sizeof(unsigned));
        input_stream.read((char*)&length,             sizeof(int));
        input_stream.read((char*)&dir,                sizeof(short));
        input_stream.read((char*)&weight,             sizeof(int));
        input_stream.read((char*)&type,               sizeof(short));
        input_stream.read((char*)&nameID,             sizeof(unsigned));
        input_stream.read((char*)&isRoundabout,       sizeof(bool));
        input_stream.read((char*)&ignoreInGrid,       sizeof(bool));
        input_stream.read((char*)&isAccessRestricted, sizeof(bool));
        input_stream.read((char*)&isContraFlow,       sizeof(bool));

        BOOST_ASSERT_MSG(length > 0, "loaded null length edge" );
        BOOST_ASSERT_MSG(weight > 0, "loaded null weight");
        BOOST_ASSERT_MSG(0<=dir && dir<=2, "loaded bogus direction");

        bool forward = true;
        bool backward = true;
        if (1 == dir) { backward = false; }
        if (2 == dir) { forward = false; }

        BOOST_ASSERT(type >= 0);

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
        edge_list.push_back(inputEdge);
    }
    std::sort(edge_list.begin(), edge_list.end());
    for(unsigned i = 1; i < edge_list.size(); ++i) {
        if( (edge_list[i-1].target() == edge_list[i].target()) && (edge_list[i-1].source() == edge_list[i].source()) ) {
            bool edgeFlagsAreEquivalent = (edge_list[i-1].isForward() == edge_list[i].isForward()) && (edge_list[i-1].isBackward() == edge_list[i].isBackward());
            bool edgeFlagsAreSuperSet1 = (edge_list[i-1].isForward() && edge_list[i-1].isBackward()) && (edge_list[i].isBackward() != edge_list[i].isBackward() );
            bool edgeFlagsAreSuperSet2 = (edge_list[i].isForward() && edge_list[i].isBackward()) && (edge_list[i-1].isBackward() != edge_list[i-1].isBackward() );

            if( edgeFlagsAreEquivalent ) {
                edge_list[i]._weight = std::min(edge_list[i-1].weight(), edge_list[i].weight());
                edge_list[i-1]._source = UINT_MAX;
            } else if (edgeFlagsAreSuperSet1) {
                if(edge_list[i-1].weight() <= edge_list[i].weight()) {
                    //edge i-1 is smaller and goes in both directions. Throw away the other edge
                    edge_list[i]._source = UINT_MAX;
                } else {
                    //edge i-1 is open in both directions, but edge i is smaller in one direction. Close edge i-1 in this direction
                    edge_list[i-1].forward = !edge_list[i].isForward();
                    edge_list[i-1].backward = !edge_list[i].isBackward();
                }
            } else if (edgeFlagsAreSuperSet2) {
                if(edge_list[i-1].weight() <= edge_list[i].weight()) {
                     //edge i-1 is smaller for one direction. edge i is open in both. close edge i in the other direction
                     edge_list[i].forward = !edge_list[i-1].isForward();
                     edge_list[i].backward = !edge_list[i-1].isBackward();
                 } else {
                     //edge i is smaller and goes in both direction. Throw away edge i-1
                     edge_list[i-1]._source = UINT_MAX;
                 }
            }
        }
    }
    typename std::vector<EdgeT>::iterator newEnd = std::remove_if(edge_list.begin(), edge_list.end(), NodesWithoutSourceRemover<EdgeT>());
    ext_to_int_id_map.clear();
    std::vector<EdgeT>(edge_list.begin(), newEnd).swap(edge_list); //remove excess candidates.
    SimpleLogger().Write() << "Graph loaded ok and has " << edge_list.size() << " edges";
    return n;
}

template<typename EdgeT>
NodeID readDTMPGraphFromStream(
    std::istream &in,
    std::vector<EdgeT>& edge_list,
    std::vector<NodeInfo> * int_to_ext_node_id_map
) {
    NodeID n, source, target, id;
    EdgeID m;
    int dir, xcoord, ycoord;// direction (0 = open, 1 = forward, 2+ = open)
    ExternalNodeMap ext_to_int_id_map;
    in >> n;
    SimpleLogger().Write(logDEBUG) << "Importing n = " << n << " nodes ";
    for (NodeID i=0; i<n; ++i) {
        in >> id >> ycoord >> xcoord;
        int_to_ext_node_id_map->push_back(NodeInfo(xcoord, ycoord, id));
        ext_to_int_id_map.insert(std::make_pair(id, i));
    }
    in >> m;
    SimpleLogger().Write(logDEBUG) << " and " << m << " edges";

    edge_list.reserve(m);
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
        BOOST_ASSERT(length > 0);
        BOOST_ASSERT(weight > 0);
        if(dir <0 || dir > 2) {
            SimpleLogger().Write(logWARNING) << "direction bogus: " << dir;
        }
        BOOST_ASSERT(0<=dir && dir<=2);

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
        edge_list.push_back(inputEdge);
    }
    ext_to_int_id_map.clear();
    std::vector<EdgeT>(edge_list.begin(), edge_list.end()).swap(edge_list); //remove excess candidates.
    std::cout << "ok" << std::endl;
    return n;
}

template<typename EdgeT>
NodeID readDDSGGraphFromStream(std::istream &in, std::vector<EdgeT>& edge_list, std::vector<NodeID> & int_to_ext_node_id_map) {
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

    edge_list.reserve(m);
    for (EdgeID i=0; i<m; i++) {
        EdgeWeight weight;
        in >> source >> target >> weight >> dir;

        BOOST_ASSERT(weight > 0);
        if(dir <0 || dir > 3) {
            throw OSRMException( "[error] direction bogus");
        }
        BOOST_ASSERT(0<=dir && dir<=3);

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
            int_to_ext_node_id_map.push_back(source);
            numberOfNodes++;
        }
        if( nodeMap.find(target) == nodeMap.end()) {
            nodeMap.insert(std::make_pair(target, numberOfNodes));
            int_to_ext_node_id_map.push_back(target);
            numberOfNodes++;
        }
        EdgeT inputEdge(source, target, 0, weight, forward, backward, 1 );
        edge_list.push_back(inputEdge);
    }
    std::vector<EdgeT>(edge_list.begin(), edge_list.end()).swap(edge_list); //remove excess candidates.

    nodeMap.clear();
    return numberOfNodes;
}

template<typename NodeT, typename EdgeT>
unsigned readHSGRFromStream(
    const boost::filesystem::path & hsgr_file,
    std::vector<NodeT> & node_list,
    std::vector<EdgeT> & edge_list,
    unsigned * check_sum
) {
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
    unsigned number_of_edges = 0;
    hsgr_input_stream.read( (char*) check_sum, sizeof(unsigned) );
    hsgr_input_stream.read( (char*) &number_of_nodes, sizeof(unsigned) );
    BOOST_ASSERT_MSG( 0 != number_of_nodes, "number of nodes is zero");
    hsgr_input_stream.read( (char*) &number_of_edges, sizeof(unsigned) );
    BOOST_ASSERT_MSG( 0 != number_of_edges, "number of edges is zero");
    node_list.resize(number_of_nodes + 1);
    hsgr_input_stream.read(
        (char*) &(node_list[0]),
        number_of_nodes*sizeof(NodeT)
    );

    edge_list.resize(number_of_edges);
    hsgr_input_stream.read(
        (char*) &(edge_list[0]),
        number_of_edges*sizeof(EdgeT)
    );
    hsgr_input_stream.close();
    return number_of_nodes;
}

#endif // GRAPHLOADER_H
