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

#ifdef _GLIBCXX_PARALLEL
#include <parallel/algorithm>
#else
#include <algorithm>
#endif
#include <boost/foreach.hpp>

#include "../Util/OpenMPReplacement.h"
#include "EdgeBasedGraphFactory.h"

template<>
EdgeBasedGraphFactory::EdgeBasedGraphFactory(int nodes, std::vector<NodeBasedEdge> & inputEdges, std::vector<NodeID> & bn, std::vector<NodeID> & tl, std::vector<_Restriction> & irs, std::vector<NodeInfo> & nI, std::string & srtm)
: inputRestrictions(irs), inputNodeInfoList(nI)/*, srtmLookup(srtm) */{

#ifdef _GLIBCXX_PARALLEL
    __gnu_parallel::sort(inputRestrictions.begin(), inputRestrictions.end(), CmpRestrictionByFrom);
#else
    std::sort(inputRestrictions.begin(), inputRestrictions.end(), CmpRestrictionByFrom);

    BOOST_FOREACH(NodeID id, bn) {
    	_bollardNodes.Add(id, true);
    }

    BOOST_FOREACH(NodeID id, tl) {
    	_trafficLights.Add(id, true);
    }

    INFO("bollards: " << _bollardNodes.Size());
    INFO("signals: " << _trafficLights.Size());

#endif

    std::vector< _NodeBasedEdge > edges;
    edges.reserve( 2 * inputEdges.size() );
    for ( std::vector< NodeBasedEdge >::const_iterator i = inputEdges.begin(), e = inputEdges.end(); i != e; ++i ) {
        _NodeBasedEdge edge;
        edge.source = i->source();
        edge.target = i->target();

        if(edge.source == edge.target)
            continue;

        edge.data.distance = (std::max)((int)i->weight(), 1 );
        assert( edge.data.distance > 0 );
        edge.data.shortcut = false;
        edge.data.roundabout = i->isRoundabout();
        edge.data.ignoreInGrid = i->ignoreInGrid();
        edge.data.nameID = i->name();
        edge.data.type = i->type();
        edge.data.forward = i->isForward();
        edge.data.backward = i->isBackward();
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


#ifdef _GLIBCXX_PARALLEL
    __gnu_parallel::sort( edges.begin(), edges.end() );
#else
    sort( edges.begin(), edges.end() );
#endif

    _nodeBasedGraph.reset(new _NodeBasedDynamicGraph( nodes, edges ));
    INFO("Converted " << inputEdges.size() << " node-based edges into " << _nodeBasedGraph->GetNumberOfEdges() << " edge-based nodes.");
}

template<>
void EdgeBasedGraphFactory::GetEdgeBasedEdges( std::vector< EdgeBasedEdge >& outputEdgeList ) {

    GUARANTEE(0 == outputEdgeList.size(), "Vector passed to EdgeBasedGraphFactory::GetEdgeBasedEdges(..) is not empty");
    GUARANTEE(0 != edgeBasedEdges.size(), "No edges in edge based graph");

    edgeBasedEdges.swap(outputEdgeList);
}

void EdgeBasedGraphFactory::GetEdgeBasedNodes( std::vector< EdgeBasedNode> & nodes) {
    BOOST_FOREACH(EdgeBasedNode & node, edgeBasedNodes){
        assert(node.lat1 != INT_MAX);
        assert(node.lat2 != INT_MAX);
        assert(node.lon1 != INT_MAX);
        assert(node.lon2 != INT_MAX);

        nodes.push_back(node);
    }
}

void EdgeBasedGraphFactory::Run() {
    INFO("Generating Edge based representation of input data");

    std::vector<_Restriction>::iterator restrictionIterator = inputRestrictions.begin();
    Percent p(_nodeBasedGraph->GetNumberOfNodes());
    int numberOfSkippedTurns(0);
    int nodeBasedEdgeCounter(0);
    NodeID onlyToNode(0);

    //Loop over all nodes u. Three nested loop look super-linear, but we are dealing with a number linear in the turns only.
    for(_NodeBasedDynamicGraph::NodeIterator u = 0; u < _nodeBasedGraph->GetNumberOfNodes(); ++u ) {
        //loop over all adjacent edge (u,v)
        while(inputRestrictions.end() != restrictionIterator && restrictionIterator->fromNode < u) {
            ++restrictionIterator;
        }
        for(_NodeBasedDynamicGraph::EdgeIterator e1 = _nodeBasedGraph->BeginEdges(u); e1 < _nodeBasedGraph->EndEdges(u); ++e1) {
            ++nodeBasedEdgeCounter;
            _NodeBasedDynamicGraph::NodeIterator v = _nodeBasedGraph->GetTarget(e1);

            //loop over all reachable edges (v,w)
            bool isOnlyAllowed(false);

            //Check every turn restriction originating from this edge if it is an 'only_*'-turn.
            if(restrictionIterator != inputRestrictions.end() && u == restrictionIterator->fromNode) {
            	//copying iterator, so we can loop over restrictions without forgetting currect position.
                std::vector<_Restriction>::iterator secondRestrictionIterator = restrictionIterator;
                do {
                    if(v == secondRestrictionIterator->viaNode) {
                        if(secondRestrictionIterator->flags.isOnly) {
                            isOnlyAllowed = true;
                            onlyToNode = secondRestrictionIterator->toNode;
                        }
                    }
                    ++secondRestrictionIterator;
                } while(secondRestrictionIterator != inputRestrictions.end() && u == secondRestrictionIterator->fromNode);
            }
            if(_nodeBasedGraph->EndEdges(v) == _nodeBasedGraph->BeginEdges(v) + 1 && _nodeBasedGraph->GetEdgeData(e1).type != SHRT_MAX) {
                EdgeBasedNode currentNode;
                currentNode.nameID = _nodeBasedGraph->GetEdgeData(e1).nameID;
                currentNode.lat1 = inputNodeInfoList[u].lat;
                currentNode.lon1 = inputNodeInfoList[u].lon;
                currentNode.lat2 = inputNodeInfoList[v].lat;
                currentNode.lon2 = inputNodeInfoList[v].lon;
                currentNode.id = _nodeBasedGraph->GetEdgeData(e1).edgeBasedNodeID;
                currentNode.ignoreInGrid = _nodeBasedGraph->GetEdgeData(e1).ignoreInGrid;
//                short startHeight = srtmLookup.height(currentNode.lon1/100000.,currentNode.lat1/100000. );
//                short targetHeight = srtmLookup.height(currentNode.lon2/100000.,currentNode.lat2/100000. );
//                short heightDiff = startHeight - targetHeight;
//                double increase = (heightDiff/ApproximateDistance(currentNode.lat1, currentNode.lon1, currentNode.lat2, currentNode.lon2));
//                if(heightDiff != 0)
//                    INFO("Increase at dead-end street: " << heightDiff << ", edge length: " << ApproximateDistance(currentNode.lat1, currentNode.lon1, currentNode.lat2, currentNode.lon2) << ", percentage: " << increase );
                //incorporate height diff;

                //todo: get some exponential function to converge to one for n->\infty
                currentNode.weight = _nodeBasedGraph->GetEdgeData(e1).distance;
//                if(increase > 0)
//                    currentNode.weight *= (1.+increase);

                edgeBasedNodes.push_back(currentNode);
            }

            if(_bollardNodes.Find(v) == true) {
                numberOfSkippedTurns += _nodeBasedGraph->EndEdges(v) - _nodeBasedGraph->BeginEdges(v);
                continue;
            }

            for(_NodeBasedDynamicGraph::EdgeIterator e2 = _nodeBasedGraph->BeginEdges(v); e2 < _nodeBasedGraph->EndEdges(v); ++e2) {
                _NodeBasedDynamicGraph::NodeIterator w = _nodeBasedGraph->GetTarget(e2);


                //if (u,v,w) is a forbidden turn, continue
                if(isOnlyAllowed && w != onlyToNode) {
                	//We are at an only_-restriction but not at the right turn.
                	 ++numberOfSkippedTurns;
                    continue;
                }

                bool isTurnRestricted(false);
                if( u != w || 1 == _nodeBasedGraph->GetOutDegree(v)) { //only add an edge if turn is not a U-turn except it is the end of dead-end street.
                    if(restrictionIterator != inputRestrictions.end() && u == restrictionIterator->fromNode) {
                        std::vector<_Restriction>::iterator secondRestrictionIterator = restrictionIterator;
                        do {
                            if(v == secondRestrictionIterator->viaNode) {
                                if(w == secondRestrictionIterator->toNode) {
                                    isTurnRestricted = true;
                                }
                            }
                            ++secondRestrictionIterator;
                        } while(secondRestrictionIterator != inputRestrictions.end() && u == secondRestrictionIterator->fromNode);
                    }

                    if( !isTurnRestricted || (isOnlyAllowed && w == onlyToNode) ) { //only add an edge if turn is not prohibited
                        if(isOnlyAllowed && w == onlyToNode) {
                            //                            INFO("Adding 'only_*'-turn <" << u << "," << v << "," << w << ">");
                        } else if(isOnlyAllowed && w != onlyToNode) {
                            assert(false);
                        }
                        //new costs for edge based edge (e1, e2) = cost (e1) + tc(e1,e2)
                        const _NodeBasedDynamicGraph::NodeIterator edgeBasedSource = _nodeBasedGraph->GetEdgeData(e1).edgeBasedNodeID;
                        if(edgeBasedSource > _nodeBasedGraph->GetNumberOfEdges()) {
                            ERR("edgeBasedTarget" << edgeBasedSource << ">" << _nodeBasedGraph->GetNumberOfEdges());
                        }
                        const _NodeBasedDynamicGraph::NodeIterator edgeBasedTarget = _nodeBasedGraph->GetEdgeData(e2).edgeBasedNodeID;
                        if(edgeBasedTarget > _nodeBasedGraph->GetNumberOfEdges()) {
                            ERR("edgeBasedTarget" << edgeBasedTarget << ">" << _nodeBasedGraph->GetNumberOfEdges());
                        }

                        //incorporate turn costs, this is just a simple model and can (read: must) be extended
//                        double angle = GetAngleBetweenTwoEdges(inputNodeInfoList[u], inputNodeInfoList[v], inputNodeInfoList[w]);

                        unsigned distance = _nodeBasedGraph->GetEdgeData(e1).distance;//(int)( _nodeBasedGraph->GetEdgeData(e1).distance *(1+std::abs((angle-180.)/180.)));
                        unsigned nameID = _nodeBasedGraph->GetEdgeData(e2).nameID;
                        short turnInstruction = AnalyzeTurn(u, v, w);

                        //create edge-based graph edge
                        EdgeBasedEdge newEdge(edgeBasedSource, edgeBasedTarget, v,  nameID, distance, true, false, turnInstruction);
                        edgeBasedEdges.push_back(newEdge);

                        if(_bollardNodes.Find(w) == true){
//                            INFO("[" << w << "] loc: " << inputNodeInfoList[w].lat << "," << inputNodeInfoList[w].lon << ", tgt: " << edgeBasedTarget);
                            //if node v is a bollard, then we need to add e2 as target node to the new set of edgebased nodes.
                            //Otherwise it will not be possible to route to route to this node
                            EdgeBasedNode currentNode;
                            currentNode.nameID = _nodeBasedGraph->GetEdgeData(e1).nameID;
                            currentNode.lat1 = inputNodeInfoList[v].lat;
                            currentNode.lon1 = inputNodeInfoList[v].lon;
                            currentNode.lat2 = inputNodeInfoList[w].lat;
                            currentNode.lon2 = inputNodeInfoList[w].lon;
                            currentNode.id = edgeBasedTarget;
                            currentNode.ignoreInGrid = _nodeBasedGraph->GetEdgeData(e2).ignoreInGrid;
                            edgeBasedNodes.push_back(currentNode);

                        }
                        if(_nodeBasedGraph->GetEdgeData(e1).type != SHRT_MAX ) {
                            EdgeBasedNode currentNode;
                            currentNode.nameID = _nodeBasedGraph->GetEdgeData(e1).nameID;
                            currentNode.lat1 = inputNodeInfoList[u].lat;
                            currentNode.lon1 = inputNodeInfoList[u].lon;
                            currentNode.lat2 = inputNodeInfoList[v].lat;
                            currentNode.lon2 = inputNodeInfoList[v].lon;
                            currentNode.id = edgeBasedSource;
                            currentNode.ignoreInGrid = _nodeBasedGraph->GetEdgeData(e1).ignoreInGrid;
//                            short startHeight = srtmLookup.height(currentNode.lon1/100000.,currentNode.lat1/100000. );
//                            short targetHeight = srtmLookup.height(currentNode.lon2/100000.,currentNode.lat2/100000. );
//                            short heightDiff = startHeight - targetHeight;
//                            double increase = (heightDiff/ApproximateDistance(currentNode.lat1, currentNode.lon1, currentNode.lat2, currentNode.lon2));
//                            if(heightDiff != 0)
//                                INFO("Increase at turn: " << heightDiff << ", edge length: " << ApproximateDistance(currentNode.lat1, currentNode.lon1, currentNode.lat2, currentNode.lon2) << ", percentage: " << increase );                            //incorporate height diff;
                            currentNode.weight = distance;
                            edgeBasedNodes.push_back(currentNode);
                        }
                    } else {
                        ++numberOfSkippedTurns;
                    }
                }
            }
        }
        p.printIncrement();
    }
    std::sort(edgeBasedNodes.begin(), edgeBasedNodes.end());
    edgeBasedNodes.erase( std::unique(edgeBasedNodes.begin(), edgeBasedNodes.end()), edgeBasedNodes.end() );
    INFO("Node-based graph contains " << nodeBasedEdgeCounter     << " edges");
    INFO("Edge-based graph contains " << edgeBasedEdges.size()    << " edges, blowup is " << (double)edgeBasedEdges.size()/(double)nodeBasedEdgeCounter);
    INFO("Edge-based graph skipped "  << numberOfSkippedTurns     << " turns, defined by " << inputRestrictions.size() << " restrictions.");
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
        if(data1.roundabout && (!data2.roundabout) )
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

EdgeBasedGraphFactory::~EdgeBasedGraphFactory() {
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
