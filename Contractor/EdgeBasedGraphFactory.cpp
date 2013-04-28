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

#include "EdgeBasedGraphFactory.h"

template<>
EdgeBasedGraphFactory::EdgeBasedGraphFactory(int nodes, std::vector<NodeBasedEdge> & inputEdges, std::vector<NodeID> & bn, std::vector<NodeID> & tl, std::vector<_Restriction> & irs, std::vector<NodeInfo> & nI, SpeedProfileProperties sp) : speedProfile(sp), inputNodeInfoList(nI), numberOfTurnRestrictions(irs.size()) {
	BOOST_FOREACH(const _Restriction & restriction, irs) {
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

	_barrierNodes.insert(bn.begin(), bn.end());
	_trafficLights.insert(tl.begin(), tl.end());

    DeallocatingVector< _NodeBasedEdge > edges;
    _NodeBasedEdge edge;
    for ( std::vector< NodeBasedEdge >::const_iterator i = inputEdges.begin(); i != inputEdges.end(); ++i ) {
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
        if(edge.source == edge.target) {
        	continue;
        }
        edge.data.distance = (std::max)((int)i->weight(), 1 );
        assert( edge.data.distance > 0 );
        edge.data.shortcut = false;
        edge.data.roundabout = i->isRoundabout();
        edge.data.ignoreInGrid = i->ignoreInGrid();
        edge.data.nameID = i->name();
        edge.data.isAccessRestricted = i->isAccessRestricted();
        edge.data.edgeBasedNodeID = edges.size();
        edge.data.mode = i->mode();
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
    std::sort( edges.begin(), edges.end() );
    _nodeBasedGraph = boost::make_shared<_NodeBasedDynamicGraph>( nodes, edges );
}

void EdgeBasedGraphFactory::GetEdgeBasedEdges(DeallocatingVector< EdgeBasedEdge >& outputEdgeList ) {
    GUARANTEE(0 == outputEdgeList.size(), "Vector passed to EdgeBasedGraphFactory::GetEdgeBasedEdges(..) is not empty");
    edgeBasedEdges.swap(outputEdgeList);
}

void EdgeBasedGraphFactory::GetEdgeBasedNodes( DeallocatingVector< EdgeBasedNode> & nodes) {
#ifndef NDEBUG
    BOOST_FOREACH(EdgeBasedNode & node, edgeBasedNodes){
        assert(node.lat1 != INT_MAX); assert(node.lon1 != INT_MAX);
        assert(node.lat2 != INT_MAX); assert(node.lon2 != INT_MAX);
    }
#endif
    nodes.swap(edgeBasedNodes);
}

NodeID EdgeBasedGraphFactory::CheckForEmanatingIsOnlyTurn(const NodeID u, const NodeID v) const {
    std::pair < NodeID, NodeID > restrictionSource = std::make_pair(u, v);
    RestrictionMap::const_iterator restrIter = _restrictionMap.find(restrictionSource);
    if (restrIter != _restrictionMap.end()) {
        unsigned index = restrIter->second;
        BOOST_FOREACH(const RestrictionSource & restrictionTarget, _restrictionBucketVector.at(index)) {
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
        _NodeBasedDynamicGraph::NodeIterator v,
        bool belongsToTinyComponent) {
    _NodeBasedDynamicGraph::EdgeData & data = _nodeBasedGraph->GetEdgeData(e1);
    EdgeBasedNode currentNode;
    currentNode.nameID = data.nameID;
    currentNode.lat1 = inputNodeInfoList[u].lat;
    currentNode.lon1 = inputNodeInfoList[u].lon;
    currentNode.lat2 = inputNodeInfoList[v].lat;
    currentNode.lon2 = inputNodeInfoList[v].lon;
    currentNode.belongsToTinyComponent = belongsToTinyComponent;
    currentNode.id = data.edgeBasedNodeID;
    currentNode.ignoreInGrid = data.ignoreInGrid;
    currentNode.weight = data.distance;
    currentNode.mode = data.mode;
    edgeBasedNodes.push_back(currentNode);
}

void EdgeBasedGraphFactory::Run(const char * originalEdgeDataFilename, lua_State *myLuaState) {
    Percent p(_nodeBasedGraph->GetNumberOfNodes());
    int numberOfSkippedTurns(0);
    int nodeBasedEdgeCounter(0);
    unsigned numberOfOriginalEdges(0);
    std::ofstream originalEdgeDataOutFile(originalEdgeDataFilename, std::ios::binary);
    originalEdgeDataOutFile.write((char*)&numberOfOriginalEdges, sizeof(unsigned));


    INFO("Identifying small components");
    //Run a BFS on the undirected graph and identify small components
    std::queue<std::pair<NodeID, NodeID> > bfsQueue;
    std::vector<unsigned> componentsIndex(_nodeBasedGraph->GetNumberOfNodes(), UINT_MAX);
    std::vector<NodeID> vectorOfComponentSizes;
    unsigned currentComponent = 0, sizeOfCurrentComponent = 0;
    //put unexplorered node with parent pointer into queue
    for(NodeID node = 0, endNodes = _nodeBasedGraph->GetNumberOfNodes(); node < endNodes; ++node) {
        if(UINT_MAX == componentsIndex[node]) {
            bfsQueue.push(std::make_pair(node, node));
            //mark node as read
            componentsIndex[node] = currentComponent;
            p.printIncrement();
            while(!bfsQueue.empty()) {
                //fetch element from BFS queue
                std::pair<NodeID, NodeID> currentQueueItem = bfsQueue.front();
                bfsQueue.pop();
                //                INFO("sizeof queue: " << bfsQueue.size() <<  ", sizeOfCurrentComponents: " <<  sizeOfCurrentComponent << ", settled nodes: " << settledNodes++ << ", max: " << endNodes);
                const NodeID v = currentQueueItem.first;  //current node
                const NodeID u = currentQueueItem.second; //parent
                //increment size counter of current component
                ++sizeOfCurrentComponent;
                const bool isBollardNode = (_barrierNodes.find(v) != _barrierNodes.end());
                if(!isBollardNode) {
                    const NodeID onlyToNode = CheckForEmanatingIsOnlyTurn(u, v);

                    //relaxieren edge outgoing edge like below where edge-expanded graph
                    for(_NodeBasedDynamicGraph::EdgeIterator e2 = _nodeBasedGraph->BeginEdges(v); e2 < _nodeBasedGraph->EndEdges(v); ++e2) {
                        _NodeBasedDynamicGraph::NodeIterator w = _nodeBasedGraph->GetTarget(e2);

                        if(onlyToNode != UINT_MAX && w != onlyToNode) { //We are at an only_-restriction but not at the right turn.
                            continue;
                        }
                        if( u != w ) { //only add an edge if turn is not a U-turn except it is the end of dead-end street.
                            if (!CheckIfTurnIsRestricted(u, v, w) ) { //only add an edge if turn is not prohibited
                                //insert next (node, parent) only if w has not yet been explored
                                if(UINT_MAX == componentsIndex[w]) {
                                    //mark node as read
                                    componentsIndex[w] = currentComponent;
                                    bfsQueue.push(std::make_pair(w,v));
                                    p.printIncrement();
                                }
                            }
                        }
                    }
                }
            }
            //push size into vector
            vectorOfComponentSizes.push_back(sizeOfCurrentComponent);
            //reset counters;
            sizeOfCurrentComponent = 0;
            ++currentComponent;
        }
    }
    INFO("identified: " << vectorOfComponentSizes.size() << " many components");

    p.reinit(_nodeBasedGraph->GetNumberOfNodes());
    //loop over all edges and generate new set of nodes.
    for(_NodeBasedDynamicGraph::NodeIterator u = 0; u < _nodeBasedGraph->GetNumberOfNodes(); ++u ) {
        for(_NodeBasedDynamicGraph::EdgeIterator e1 = _nodeBasedGraph->BeginEdges(u); e1 < _nodeBasedGraph->EndEdges(u); ++e1) {
            _NodeBasedDynamicGraph::NodeIterator v = _nodeBasedGraph->GetTarget(e1);

            assert(e1 != UINT_MAX);
            assert(u != UINT_MAX);
            assert(v != UINT_MAX);
            //edges that end on bollard nodes may actually be in two distinct components
            InsertEdgeBasedNode(e1, u, v, (std::min(vectorOfComponentSizes[componentsIndex[u]], vectorOfComponentSizes[componentsIndex[v]]) < 1000) );
        }
    }

    std::vector<NodeID>().swap(vectorOfComponentSizes);
    std::vector<NodeID>().swap(componentsIndex);

    std::vector<OriginalEdgeData> original_edge_data_vector;
    original_edge_data_vector.reserve(10000);

    //Loop over all turns and generate new set of edges.
    //Three nested loop look super-linear, but we are dealing with a linear number of turns only.
    for(_NodeBasedDynamicGraph::NodeIterator u = 0; u < _nodeBasedGraph->GetNumberOfNodes(); ++u ) {
        for(_NodeBasedDynamicGraph::EdgeIterator e1 = _nodeBasedGraph->BeginEdges(u); e1 < _nodeBasedGraph->EndEdges(u); ++e1) {
            ++nodeBasedEdgeCounter;
            _NodeBasedDynamicGraph::NodeIterator v = _nodeBasedGraph->GetTarget(e1);
            bool isBollardNode = (_barrierNodes.find(v) != _barrierNodes.end());
            //EdgeWeight heightPenalty = ComputeHeightPenalty(u, v);
            NodeID onlyToNode = CheckForEmanatingIsOnlyTurn(u, v);
            for(_NodeBasedDynamicGraph::EdgeIterator e2 = _nodeBasedGraph->BeginEdges(v); e2 < _nodeBasedGraph->EndEdges(v); ++e2) {
                const _NodeBasedDynamicGraph::NodeIterator w = _nodeBasedGraph->GetTarget(e2);

                if(onlyToNode != UINT_MAX && w != onlyToNode) { //We are at an only_-restriction but not at the right turn.
                    ++numberOfSkippedTurns;
                    continue;
                }

                if(u == w && 1 != _nodeBasedGraph->GetOutDegree(v) ) {
                    continue;
                }

                if( !isBollardNode ) { //only add an edge if turn is not a U-turn except it is the end of dead-end street.
                    if (!CheckIfTurnIsRestricted(u, v, w) || (onlyToNode != UINT_MAX && w == onlyToNode)) { //only add an edge if turn is not prohibited
                        const _NodeBasedDynamicGraph::EdgeData edgeData1 = _nodeBasedGraph->GetEdgeData(e1);
                        const _NodeBasedDynamicGraph::EdgeData edgeData2 = _nodeBasedGraph->GetEdgeData(e2);
                        assert(edgeData1.edgeBasedNodeID < _nodeBasedGraph->GetNumberOfEdges());
                        assert(edgeData2.edgeBasedNodeID < _nodeBasedGraph->GetNumberOfEdges());

                        if(!edgeData1.forward || !edgeData2.forward) {
                            continue;
                        }

                        unsigned distance = edgeData1.distance;
                        if(_trafficLights.find(v) != _trafficLights.end()) {
                            distance += speedProfile.trafficSignalPenalty;
                        }
                        unsigned penalty = 0;
                        
                        TurnInstruction turnInstruction = AnalyzeTurn(u, v, w, penalty, myLuaState);
                        if(turnInstruction == TurnInstructions.UTurn)
                            distance += speedProfile.uTurnPenalty;
//                        if(!edgeData1.isAccessRestricted && edgeData2.isAccessRestricted) {
//                            distance += TurnInstructions.AccessRestrictionPenalty;
//                            turnInstruction |= TurnInstructions.AccessRestrictionFlag;
//                        }
                        distance += penalty;
						
                        //distance += heightPenalty;
                        //distance += ComputeTurnPenalty(u, v, w);
                        assert(edgeData1.edgeBasedNodeID != edgeData2.edgeBasedNodeID);
                        OriginalEdgeData oed(v,edgeData2.nameID, turnInstruction, edgeData2.mode);
                        original_edge_data_vector.push_back(oed);
                        ++numberOfOriginalEdges;

                        if(original_edge_data_vector.size() > 100000) {
                            originalEdgeDataOutFile.write((char*)&(original_edge_data_vector[0]), original_edge_data_vector.size()*sizeof(OriginalEdgeData));
                            original_edge_data_vector.clear();
                        }

                        EdgeBasedEdge newEdge(edgeData1.edgeBasedNodeID, edgeData2.edgeBasedNodeID, edgeBasedEdges.size(), distance, true, false );
                        edgeBasedEdges.push_back(newEdge);
                    } else {
                        ++numberOfSkippedTurns;
                    }
                }
            }
        }
        p.printIncrement();
    }
    originalEdgeDataOutFile.write((char*)&(original_edge_data_vector[0]), original_edge_data_vector.size()*sizeof(OriginalEdgeData));
    originalEdgeDataOutFile.seekp(std::ios::beg);
    originalEdgeDataOutFile.write((char*)&numberOfOriginalEdges, sizeof(unsigned));
    originalEdgeDataOutFile.close();

//    INFO("Sorting edge-based Nodes");
//    std::sort(edgeBasedNodes.begin(), edgeBasedNodes.end());
//    INFO("Removing duplicate nodes (if any)");
//    edgeBasedNodes.erase( std::unique(edgeBasedNodes.begin(), edgeBasedNodes.end()), edgeBasedNodes.end() );
//    INFO("Applying vector self-swap trick to free up memory");
//    INFO("size: " << edgeBasedNodes.size() << ", cap: " << edgeBasedNodes.capacity());
//    std::vector<EdgeBasedNode>(edgeBasedNodes).swap(edgeBasedNodes);
//    INFO("size: " << edgeBasedNodes.size() << ", cap: " << edgeBasedNodes.capacity());
    INFO("Node-based graph contains " << nodeBasedEdgeCounter     << " edges");
    INFO("Edge-based graph contains " << edgeBasedEdges.size()    << " edges");
//    INFO("Edge-based graph contains " << edgeBasedEdges.size()    << " edges, blowup is " << 2*((double)edgeBasedEdges.size()/(double)nodeBasedEdgeCounter));
    INFO("Edge-based graph skipped "  << numberOfSkippedTurns     << " turns, defined by " << numberOfTurnRestrictions << " restrictions.");
    INFO("Generated " << edgeBasedNodes.size() << " edge based nodes");
}

TurnInstruction EdgeBasedGraphFactory::AnalyzeTurn(const NodeID u, const NodeID v, const NodeID w, unsigned& penalty, lua_State *myLuaState) const {
    const double angle = GetAngleBetweenTwoEdges(inputNodeInfoList[u], inputNodeInfoList[v], inputNodeInfoList[w]);
	
    if( speedProfile.has_turn_penalty_function ) {
    	try {
            //call lua profile to compute turn penalty
            penalty = luabind::call_function<int>( myLuaState, "turn_function", 180-angle );
        } catch (const luabind::error &er) {
            std::cerr << er.what() << std::endl;
            //TODO handle lua errors
        }
    } else {
        penalty = 0;
    }
    
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
        }
        return TurnInstructions.StayOnRoundAbout;
    }
    //Does turn start or end on roundabout?
    if(data1.roundabout || data2.roundabout) {
        //We are entering the roundabout
        if( (!data1.roundabout) && data2.roundabout) {
            return TurnInstructions.EnterRoundAbout;
        }
        //We are leaving the roundabout
        if(data1.roundabout && (!data2.roundabout) ) {
            return TurnInstructions.LeaveRoundAbout;
        }
    }

    //If street names and modes stay the same and if we are certain that it is not a roundabout, we skip it.
    if( (data1.nameID == data2.nameID) && (data1.mode == data2.mode) && (0 != data1.nameID)) {
        return TurnInstructions.NoTurn;
    }
    if( (data1.nameID == data2.nameID) && (0 == data1.nameID) && (_nodeBasedGraph->GetOutDegree(v) <= 2) ) {
        return TurnInstructions.NoTurn;
    }

    return TurnInstructions.GetTurnDirectionOfInstruction(angle);
}

unsigned EdgeBasedGraphFactory::GetNumberOfNodes() const {
    return _nodeBasedGraph->GetNumberOfEdges();
}

/* Get angle of line segment (A,C)->(C,B), atan2 magic, formerly cosine theorem*/
template<class CoordinateT>
double EdgeBasedGraphFactory::GetAngleBetweenTwoEdges(const CoordinateT& A, const CoordinateT& C, const CoordinateT& B) const {
    const double v1x = (A.lon - C.lon)/100000.;
    const double v1y = lat2y(A.lat/100000.) - lat2y(C.lat/100000.);
    const double v2x = (B.lon - C.lon)/100000.;
    const double v2y = lat2y(B.lat/100000.) - lat2y(C.lat/100000.);

    double angle = (atan2(v2y,v2x) - atan2(v1y,v1x) )*180/M_PI;
    while(angle < 0)
        angle += 360;
    return angle;
}
