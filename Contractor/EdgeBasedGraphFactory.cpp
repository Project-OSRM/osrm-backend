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

#include "EdgeBasedGraphFactory.h"
#include "../DataStructures/ExtractorStructs.h"

template<>
EdgeBasedGraphFactory::EdgeBasedGraphFactory(int nodes, std::vector<NodeBasedEdge> & inputEdges, std::vector<_Restriction> & irs, std::vector<NodeInfo> & nI)
: inputRestrictions(irs), inputNodeInfoList(nI) {

#ifdef _GLIBCXX_PARALLEL
    __gnu_parallel::sort(inputRestrictions.begin(), inputRestrictions.end(), CmpRestrictionByFrom);
#else
    std::sort(inputRestrictions.begin(), inputRestrictions.end(), CmpRestrictionByFrom);
#endif

    std::vector< _NodeBasedEdge > edges;
    edges.reserve( 2 * inputEdges.size() );
    for ( typename std::vector< NodeBasedEdge >::const_iterator i = inputEdges.begin(), e = inputEdges.end(); i != e; ++i ) {
        _NodeBasedEdge edge;
        edge.source = i->source();
        edge.target = i->target();

        if(edge.source == edge.target)
            continue;

        edge.data.distance = (std::max)((int)i->weight(), 1 );
        assert( edge.data.distance > 0 );
        edge.data.shortcut = false;
        edge.data.middleName.nameID = i->name();
        edge.data.type = i->type();
        edge.data.forward = i->isForward();
        edge.data.backward = i->isBackward();
        edge.data.edgeBasedNodeID = edges.size();
        edges.push_back( edge );
        std::swap( edge.source, edge.target );
        if( edge.data.backward ) {
            edge.data.forward = i->isBackward();
            edge.data.backward = i->isForward();
            edge.data.edgeBasedNodeID = edges.size();
            edges.push_back( edge );
        }
    }

    INFO("edges.size()=" << edges.size());

#ifdef _GLIBCXX_PARALLEL
    __gnu_parallel::sort( edges.begin(), edges.end() );
#else
    sort( edges.begin(), edges.end() );
#endif

    _nodeBasedGraph.reset(new _NodeBasedDynamicGraph( nodes, edges ));
    INFO("Converted " << inputEdges.size() << " node-based edges into " << _nodeBasedGraph->GetNumberOfEdges() << " edge-based nodes.");
}

template<>
void EdgeBasedGraphFactory::GetEdgeBasedEdges( std::vector< EdgeBasedEdge >& edges ) {

    GUARANTEE(0 == edges.size(), "Vector passed to " << __FUNCTION__ << " is not empty");
    GUARANTEE(0 != edgeBasedEdges.size(), "No edges in edge based graph");

    BOOST_FOREACH ( EdgeBasedEdge currentEdge, edgeBasedEdges) {
        edges.push_back(currentEdge);
    }
}

void EdgeBasedGraphFactory::GetEdgeBasedNodes( std::vector< EdgeBasedNode> & nodes) {
    for(unsigned i = 0; i < edgeBasedNodes.size(); ++i)
        nodes.push_back(edgeBasedNodes[i]);
}

void EdgeBasedGraphFactory::Run() {
    INFO("Generating Edge based representation of input data");

    _edgeBasedGraph.reset(new _EdgeBasedDynamicGraph(_nodeBasedGraph->GetNumberOfEdges() ) );

    std::vector<_Restriction>::iterator restrictionIterator = inputRestrictions.begin();
    Percent p(_nodeBasedGraph->GetNumberOfNodes());
    int numberOfResolvedRestrictions(0);
    int nodeBasedEdgeCounter(0);
    //Loop over all nodes u. Three nested loop look super-linear, but we are dealing with a number linear in the turns only.
    for(_NodeBasedDynamicGraph::NodeIterator u = 0; u < _nodeBasedGraph->GetNumberOfNodes(); ++u ) {
        //loop over all adjacent edge (u,v)
        while(restrictionIterator->fromNode < u && inputRestrictions.end() != restrictionIterator) {
            ++restrictionIterator;
        }
        for(_NodeBasedDynamicGraph::EdgeIterator e1 = _nodeBasedGraph->BeginEdges(u); e1 < _nodeBasedGraph->EndEdges(u); ++e1) {
            ++nodeBasedEdgeCounter;
            _NodeBasedDynamicGraph::NodeIterator v = _nodeBasedGraph->GetTarget(e1);
            //loop over all reachable edges (v,w)
            for(_NodeBasedDynamicGraph::EdgeIterator e2 = _nodeBasedGraph->BeginEdges(v); e2 < _nodeBasedGraph->EndEdges(v); ++e2) {
                _NodeBasedDynamicGraph::NodeIterator w = _nodeBasedGraph->GetTarget(e2);
                //if (u,v,w) is a forbidden turn, continue
                bool isTurnProhibited = false;
                if( u != w ) { //only add an edge if turn is not a U-turn
                    if(u == restrictionIterator->fromNode) {
                        std::vector<_Restriction>::iterator secondRestrictionIterator = restrictionIterator;
                        do {
                            if( v == secondRestrictionIterator->viaNode && w == secondRestrictionIterator->toNode) {
                                isTurnProhibited = true;
                            }
                            ++secondRestrictionIterator;
                        } while(u == secondRestrictionIterator->fromNode);
                    }
                    if( !isTurnProhibited ) { //only add an edge if turn is not prohibited
                        //new costs for edge based edge (e1, e2) = cost (e1) + tc(e1,e2)
                        const _NodeBasedDynamicGraph::NodeIterator edgeBasedSource = _nodeBasedGraph->GetEdgeData(e1).edgeBasedNodeID;
                        //                        INFO("edgeBasedSource: " << edgeBasedSource);
                        if(edgeBasedSource > _nodeBasedGraph->GetNumberOfEdges()) {
                            ERR("edgeBasedTarget" << edgeBasedSource << ">" << _nodeBasedGraph->GetNumberOfEdges());
                        }
                        const _NodeBasedDynamicGraph::NodeIterator edgeBasedTarget = _nodeBasedGraph->GetEdgeData(e2).edgeBasedNodeID;
                        //                        INFO("edgeBasedTarget: " << edgeBasedTarget);
                        if(edgeBasedTarget > _nodeBasedGraph->GetNumberOfEdges()) {
                            ERR("edgeBasedTarget" << edgeBasedTarget << ">" << _nodeBasedGraph->GetNumberOfEdges());
                        }

                        _EdgeBasedEdge newEdge;
                        newEdge.source = edgeBasedSource;
                        newEdge.target = edgeBasedTarget;
                        //Todo: incorporate turn costs
                        newEdge.data.distance = _nodeBasedGraph->GetEdgeData(e1).distance;
                        newEdge.data.forward = true;
                        newEdge.data.backward = false;
                        newEdge.data.via = v;
                        newEdge.data.nameID1 = _nodeBasedGraph->GetEdgeData(e1).middleName.nameID;
                        newEdge.data.nameID2 = _nodeBasedGraph->GetEdgeData(e2).middleName.nameID;
                        //Todo: turn instruction angeben

                        if(newEdge.data.nameID1 == newEdge.data.nameID2)
                            newEdge.data.turnInstruction = 0;
                        else {
                            //TODO: Winkel berechnen und angepasste Anweisung geben.
                            newEdge.data.turnInstruction = 1;
                        }
                        //create Edge for NearestNeighborlookup
                        edgeBasedEdges.push_back(newEdge);
                        EdgeBasedNode currentNode;

                        if(_nodeBasedGraph->GetEdgeData(e1).type != 14) {
                            currentNode.nameID = newEdge.data.nameID1;
                            currentNode.lat1 = inputNodeInfoList[u].lat;
                            currentNode.lon1 = inputNodeInfoList[u].lon;
                            currentNode.lat2 = inputNodeInfoList[v].lat;
                            currentNode.lon2 = inputNodeInfoList[v].lon;
                            currentNode.id = edgeBasedSource;
                            currentNode.weight = _nodeBasedGraph->GetEdgeData(e1).distance;
                            edgeBasedNodes.push_back(currentNode);
                        }
                    } else {
                        ++numberOfResolvedRestrictions;
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
    INFO("Edge-based graph obeys "    << numberOfResolvedRestrictions   << " turn restrictions, " << (inputRestrictions.size() - numberOfResolvedRestrictions )<< " skipped.");
    INFO("Generated " << edgeBasedNodes.size() << " edge based nodes");
}

unsigned EdgeBasedGraphFactory::GetNumberOfNodes() const {
    return edgeBasedEdges.size();
}

EdgeBasedGraphFactory::~EdgeBasedGraphFactory() {
}

