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

#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "../Util/OpenMPReplacement.h"
#include "../DataStructures/Percent.h"
#include "EdgeBasedGraphFactory.h"

template<>
EdgeBasedGraphFactory::EdgeBasedGraphFactory(int nodes, std::vector<NodeBasedEdge> & inputEdges, std::vector<NodeID> & bn, std::vector<NodeID> & tl, std::vector<_Restriction> & irs, std::vector<NodeInfo> & nI, boost::property_tree::ptree speedProfile, std::string & srtm) : inputNodeInfoList(nI), numberOfTurnRestrictions(irs.size()), trafficSignalPenalty(0) {
    BOOST_FOREACH(_Restriction & restriction, irs) {
        std::pair<NodeID, NodeID> restrictionSource = std::make_pair(restriction.fromNode, restriction.viaNode);
        unsigned index;
        RestrictionMap::iterator restrIter = _restrictionMap.find(restrictionSource);
        if(restrIter == _restrictionMap.end()) {
            index = _restrictionBucketVector.size();
            _restrictionBucketVector.resize(index+1);
            _restrictionMap[restrictionSource] = index;
        } else {
            index = restrIter->second;
            //Map already contains an is_only_*-restriction
            if(_restrictionBucketVector.at(index).begin()->second)
                continue;
            else if(restriction.flags.isOnly){
                //We are going to insert an is_only_*-restriction. There can be only one.
                _restrictionBucketVector.at(index).clear();
            }
        }

        _restrictionBucketVector.at(index).push_back(std::make_pair(restriction.toNode, restriction.flags.isOnly));
    }

    std::string usedSpeedProfile( speedProfile.get_child("").begin()->first );
    BOOST_FOREACH(boost::property_tree::ptree::value_type &v, speedProfile.get_child(usedSpeedProfile)) {
        if("trafficSignalPenalty" ==  v.first) {
            std::string value = v.second.get<std::string>("");
            try {
                trafficSignalPenalty = 10*boost::lexical_cast<int>(v.second.get<std::string>(""));
            } catch(boost::bad_lexical_cast &) {
                trafficSignalPenalty = 0;
            }
        }
        if("uturnPenalty" ==  v.first) {
            std::string value = v.second.get<std::string>("");
            try {
                uturnPenalty = 10*boost::lexical_cast<int>(v.second.get<std::string>(""));
            } catch(boost::bad_lexical_cast &) {
                uturnPenalty = 0;
            }
        }
        if("takeMinimumOfSpeeds" ==  v.first) {
            std::string value = v.second.get<std::string>("");
            takeMinimumOfSpeeds = (v.second.get<std::string>("") == "yes");
        }
    }

//    INFO("traffic signal penalty: " << trafficSignalPenalty << ", U-Turn penalty: " << uturnPenalty << ", takeMinimumOfSpeeds=" << (takeMinimumOfSpeeds ? "yes" : "no"));

    BOOST_FOREACH(NodeID id, bn) {
        _barrierNodes[id] = true;
    }
    BOOST_FOREACH(NodeID id, tl) {
        _trafficLights[id] = true;
    }

    DeallocatingVector< _NodeBasedEdge > edges;
//    edges.reserve( 2 * inputEdges.size() );
    for ( std::vector< NodeBasedEdge >::const_iterator i = inputEdges.begin(); i != inputEdges.end(); ++i ) {

        _NodeBasedEdge edge;
        if(!i->isForward()) {
            edge.source = i->target();
            edge.target = i->source();
            edge.data.backward = i->isForward();
            edge.data.forward = i->isBackward();
       } else {
            edge.source = i->source();
            edge.target = i->target();
            edge.data.forward = i->isForward();
            edge.data.backward = i->isBackward();
        }
        if(edge.source == edge.target)
            continue;

        edge.data.distance = (std::max)((int)i->weight(), 1 );
        assert( edge.data.distance > 0 );
        edge.data.shortcut = false;
        edge.data.roundabout = i->isRoundabout();
        edge.data.ignoreInGrid = i->ignoreInGrid();
        edge.data.nameID = i->name();
        edge.data.type = i->type();
        edge.data.isAccessRestricted = i->isAccessRestricted();
        edge.data.edgeBasedNodeID = edges.size();
        edges.push_back( edge );
        if( edge.data.backward ) {
            std::swap( edge.source, edge.target );
            edge.data.forward = i->isBackward();
            edge.data.backward = i->isForward();
            edge.data.edgeBasedNodeID = edges.size();
            edges.push_back( edge );
        }
    }
    std::vector<NodeBasedEdge>().swap(inputEdges);
    //std::vector<_NodeBasedEdge>(edges).swap(edges);
    std::sort( edges.begin(), edges.end() );

    _nodeBasedGraph.reset(new _NodeBasedDynamicGraph( nodes, edges ));
}

void EdgeBasedGraphFactory::GetEdgeBasedEdges(DeallocatingVector< EdgeBasedEdge >& outputEdgeList ) {
    GUARANTEE(0 == outputEdgeList.size(), "Vector passed to EdgeBasedGraphFactory::GetEdgeBasedEdges(..) is not empty");
    GUARANTEE(0 != edgeBasedEdges.size(), "No edges in edge based graph");

    edgeBasedEdges.swap(outputEdgeList);
}

void EdgeBasedGraphFactory::GetEdgeBasedNodes( std::vector< EdgeBasedNode> & nodes) {
    BOOST_FOREACH(EdgeBasedNode & node, edgeBasedNodes){
        assert(node.lat1 != INT_MAX); assert(node.lon1 != INT_MAX);
        assert(node.lat2 != INT_MAX); assert(node.lon2 != INT_MAX);
    }
    nodes.swap(edgeBasedNodes);
}

void EdgeBasedGraphFactory::GetOriginalEdgeData( std::vector< OriginalEdgeData> & oed) {
    oed.swap(originalEdgeData);
}

NodeID EdgeBasedGraphFactory::CheckForEmanatingIsOnlyTurn(const NodeID u, const NodeID v) const {
    std::pair < NodeID, NodeID > restrictionSource = std::make_pair(u, v);
    RestrictionMap::const_iterator restrIter = _restrictionMap.find(restrictionSource);
    if (restrIter != _restrictionMap.end()) {
        unsigned index = restrIter->second;
        BOOST_FOREACH(RestrictionSource restrictionTarget, _restrictionBucketVector.at(index)) {
            if(restrictionTarget.second) {
                return restrictionTarget.first;
            }
        }
    }
    return UINT_MAX;
}

bool EdgeBasedGraphFactory::CheckIfTurnIsRestricted(const NodeID u, const NodeID v, const NodeID w) const {
    //only add an edge if turn is not a U-turn except it is the end of dead-end street.
    std::pair < NodeID, NodeID > restrictionSource = std::make_pair(u, v);
    RestrictionMap::const_iterator restrIter = _restrictionMap.find(restrictionSource);
    if (restrIter != _restrictionMap.end()) {
        unsigned index = restrIter->second;
        BOOST_FOREACH(RestrictionTarget restrictionTarget, _restrictionBucketVector.at(index)) {
            if(w == restrictionTarget.first)
                return true;
        }
    }
    return false;
}

void EdgeBasedGraphFactory::InsertEdgeBasedNode(
        _NodeBasedDynamicGraph::EdgeIterator e1,
        _NodeBasedDynamicGraph::NodeIterator u,
        _NodeBasedDynamicGraph::NodeIterator v) {
    _NodeBasedDynamicGraph::EdgeData & data = _nodeBasedGraph->GetEdgeData(e1);
    EdgeBasedNode currentNode;
    currentNode.nameID = data.nameID;
    currentNode.lat1 = inputNodeInfoList[u].lat;
    currentNode.lon1 = inputNodeInfoList[u].lon;
    currentNode.lat2 = inputNodeInfoList[v].lat;
    currentNode.lon2 = inputNodeInfoList[v].lon;
    currentNode.id = data.edgeBasedNodeID;
    currentNode.ignoreInGrid = data.ignoreInGrid;
    currentNode.weight = data.distance;
    //currentNode.weight += ComputeHeightPenalty(u, v);
    edgeBasedNodes.push_back(currentNode);
}

void EdgeBasedGraphFactory::Run(const char * originalEdgeDataFilename) {
    Percent p(_nodeBasedGraph->GetNumberOfNodes());
    int numberOfSkippedTurns(0);
    int nodeBasedEdgeCounter(0);
    unsigned numberOfOriginalEdges(0);
    std::ofstream originalEdgeDataOutFile(originalEdgeDataFilename, std::ios::binary);
    originalEdgeDataOutFile.write((char*)&numberOfOriginalEdges, sizeof(unsigned));


    //loop over all edges and generate new set of nodes.
    for(_NodeBasedDynamicGraph::NodeIterator u = 0; u < _nodeBasedGraph->GetNumberOfNodes(); ++u ) {
        for(_NodeBasedDynamicGraph::EdgeIterator e1 = _nodeBasedGraph->BeginEdges(u); e1 < _nodeBasedGraph->EndEdges(u); ++e1) {
            _NodeBasedDynamicGraph::NodeIterator v = _nodeBasedGraph->GetTarget(e1);

            if(_nodeBasedGraph->GetEdgeData(e1).type != SHRT_MAX) {
                assert(e1 != UINT_MAX);
                assert(u != UINT_MAX);
                assert(v != UINT_MAX);
                InsertEdgeBasedNode(e1, u, v);
            }
        }
    }

    //Loop over all turns and generate new set of edges.
    //Three nested loop look super-linear, but we are dealing with a linear number of turns only.
    for(_NodeBasedDynamicGraph::NodeIterator u = 0; u < _nodeBasedGraph->GetNumberOfNodes(); ++u ) {
        for(_NodeBasedDynamicGraph::EdgeIterator e1 = _nodeBasedGraph->BeginEdges(u); e1 < _nodeBasedGraph->EndEdges(u); ++e1) {
            ++nodeBasedEdgeCounter;
            _NodeBasedDynamicGraph::NodeIterator v = _nodeBasedGraph->GetTarget(e1);
            //EdgeWeight heightPenalty = ComputeHeightPenalty(u, v);
            NodeID onlyToNode = CheckForEmanatingIsOnlyTurn(u, v);
            for(_NodeBasedDynamicGraph::EdgeIterator e2 = _nodeBasedGraph->BeginEdges(v); e2 < _nodeBasedGraph->EndEdges(v); ++e2) {
                _NodeBasedDynamicGraph::NodeIterator w = _nodeBasedGraph->GetTarget(e2);

                if(onlyToNode != UINT_MAX && w != onlyToNode) { //We are at an only_-restriction but not at the right turn.
                    ++numberOfSkippedTurns;
                    continue;
                }
                bool isBollardNode = (_barrierNodes.find(v) != _barrierNodes.end());
                if( (!isBollardNode && (u != w || 1 == _nodeBasedGraph->GetOutDegree(v))) || ((u == w) && isBollardNode)) { //only add an edge if turn is not a U-turn except it is the end of dead-end street.
                    if (!CheckIfTurnIsRestricted(u, v, w) || (onlyToNode != UINT_MAX && w == onlyToNode)) { //only add an edge if turn is not prohibited
                        const _NodeBasedDynamicGraph::EdgeData edgeData1 = _nodeBasedGraph->GetEdgeData(e1);
                        const _NodeBasedDynamicGraph::EdgeData edgeData2 = _nodeBasedGraph->GetEdgeData(e2);
                        assert(edgeData1.edgeBasedNodeID < _nodeBasedGraph->GetNumberOfEdges());
                        assert(edgeData2.edgeBasedNodeID < _nodeBasedGraph->GetNumberOfEdges());

                        unsigned distance = edgeData1.distance;
                        if(_trafficLights.find(v) != _trafficLights.end()) {
                            distance += trafficSignalPenalty;
                        }
                        short turnInstruction = AnalyzeTurn(u, v, w);
                        if(turnInstruction == TurnInstructions.UTurn)
                            distance += uturnPenalty;
                        if(!edgeData1.isAccessRestricted && edgeData2.isAccessRestricted) {
                            distance += TurnInstructions.AccessRestrictionPenalty;
                            turnInstruction |= TurnInstructions.AccessRestrictionFlag;
                        }

                        //distance += heightPenalty;
                        //distance += ComputeTurnPenalty(u, v, w);
                        assert(edgeData1.edgeBasedNodeID != edgeData2.edgeBasedNodeID);
                        if(originalEdgeData.size() == originalEdgeData.capacity()-3) {
                            originalEdgeData.reserve(originalEdgeData.size()*1.2);
                        }
                        OriginalEdgeData oed(v,edgeData2.nameID, turnInstruction);
                        EdgeBasedEdge newEdge(edgeData1.edgeBasedNodeID, edgeData2.edgeBasedNodeID, edgeBasedEdges.size(), distance, true, false );
                        originalEdgeData.push_back(oed);
                        if(originalEdgeData.size() > 100000) {
                            originalEdgeDataOutFile.write((char*)&(originalEdgeData[0]), originalEdgeData.size()*sizeof(OriginalEdgeData));
                            originalEdgeData.clear();
                        }
                        ++numberOfOriginalEdges;
                        ++nodeBasedEdgeCounter;
                        edgeBasedEdges.push_back(newEdge);
                    } else {
                        ++numberOfSkippedTurns;
                    }
                }
            }
        }
        p.printIncrement();
    }
    numberOfOriginalEdges += originalEdgeData.size();
    originalEdgeDataOutFile.write((char*)&(originalEdgeData[0]), originalEdgeData.size()*sizeof(OriginalEdgeData));
    originalEdgeDataOutFile.seekp(std::ios::beg);
    originalEdgeDataOutFile.write((char*)&numberOfOriginalEdges, sizeof(unsigned));
    originalEdgeDataOutFile.close();

    INFO("Sorting edge-based Nodes");
    std::sort(edgeBasedNodes.begin(), edgeBasedNodes.end());
    INFO("Removing duplicate nodes (if any)");
    edgeBasedNodes.erase( std::unique(edgeBasedNodes.begin(), edgeBasedNodes.end()), edgeBasedNodes.end() );
    INFO("Applying vector self-swap trick to free up memory");
    INFO("size: " << edgeBasedNodes.size() << ", cap: " << edgeBasedNodes.capacity());
    std::vector<EdgeBasedNode>(edgeBasedNodes).swap(edgeBasedNodes);
    INFO("size: " << edgeBasedNodes.size() << ", cap: " << edgeBasedNodes.capacity());
    INFO("Node-based graph contains " << nodeBasedEdgeCounter     << " edges");
    INFO("Edge-based graph contains " << edgeBasedEdges.size()    << " edges, blowup is " << (double)edgeBasedEdges.size()/(double)nodeBasedEdgeCounter);
    INFO("Edge-based graph skipped "  << numberOfSkippedTurns     << " turns, defined by " << numberOfTurnRestrictions << " restrictions.");
    INFO("Generated " << edgeBasedNodes.size() << " edge based nodes");
}

short EdgeBasedGraphFactory::AnalyzeTurn(const NodeID u, const NodeID v, const NodeID w) const {
    if(u == w) {
        return TurnInstructions.UTurn;
    }

    _NodeBasedDynamicGraph::EdgeIterator edge1 = _nodeBasedGraph->FindEdge(u, v);
    _NodeBasedDynamicGraph::EdgeIterator edge2 = _nodeBasedGraph->FindEdge(v, w);

    _NodeBasedDynamicGraph::EdgeData & data1 = _nodeBasedGraph->GetEdgeData(edge1);
    _NodeBasedDynamicGraph::EdgeData & data2 = _nodeBasedGraph->GetEdgeData(edge2);

    //roundabouts need to be handled explicitely
    if(data1.roundabout && data2.roundabout) {
        //Is a turn possible? If yes, we stay on the roundabout!
        if( 1 == (_nodeBasedGraph->EndEdges(v) - _nodeBasedGraph->BeginEdges(v)) ) {
            //No turn possible.
            return TurnInstructions.NoTurn;
        } else {
            return TurnInstructions.StayOnRoundAbout;
        }
    }
    //Does turn start or end on roundabout?
    if(data1.roundabout || data2.roundabout) {
        //We are entering the roundabout
        if( (!data1.roundabout) && data2.roundabout)
            return TurnInstructions.EnterRoundAbout;
        //We are leaving the roundabout
        else if(data1.roundabout && (!data2.roundabout) )
            return TurnInstructions.LeaveRoundAbout;
    }

    //If street names stay the same and if we are certain that it is not a roundabout, we skip it.
    if( (data1.nameID == data2.nameID) && (0 != data1.nameID))
        return TurnInstructions.NoTurn;
    if( (data1.nameID == data2.nameID) && (0 == data1.nameID) && (_nodeBasedGraph->GetOutDegree(v) <= 2) )
        return TurnInstructions.NoTurn;

    double angle = GetAngleBetweenTwoEdges(inputNodeInfoList[u], inputNodeInfoList[v], inputNodeInfoList[w]);
    return TurnInstructions.GetTurnDirectionOfInstruction(angle);
}

unsigned EdgeBasedGraphFactory::GetNumberOfNodes() const {
    return _nodeBasedGraph->GetNumberOfEdges();
}

/* Get angle of line segment (A,C)->(C,B), atan2 magic, formerly cosine theorem*/
template<class CoordinateT>
double EdgeBasedGraphFactory::GetAngleBetweenTwoEdges(const CoordinateT& A, const CoordinateT& C, const CoordinateT& B) const {
    const int v1x = A.lon - C.lon;
    const int v1y = A.lat - C.lat;
    const int v2x = B.lon - C.lon;
    const int v2y = B.lat - C.lat;

    double angle = (atan2((double)v2y,v2x) - atan2((double)v1y,v1x) )*180/M_PI;
    while(angle < 0)
        angle += 360;
    return angle;
}
