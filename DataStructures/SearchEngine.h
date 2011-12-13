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

#ifndef SEARCHENGINE_H_
#define SEARCHENGINE_H_

#include <climits>
#include <deque>

#include <boost/thread.hpp>

#include "BinaryHeap.h"
#include "PhantomNodes.h"
#include "../Util/StringUtil.h"
#include "../typedefs.h"

struct _HeapData {
    NodeID parent;
    _HeapData( NodeID p ) : parent(p) { }
};

typedef boost::thread_specific_ptr<BinaryHeap< NodeID, NodeID, int, _HeapData > > HeapPtr;

template<class EdgeData, class GraphT>
class SearchEngine {
private:
    const GraphT * _graph;
    NodeInformationHelpDesk * nodeHelpDesk;
    std::vector<string> * _names;
    static HeapPtr _forwardHeap;
    static HeapPtr _backwardHeap;
    inline double absDouble(double input) { if(input < 0) return input*(-1); else return input;}
public:
    SearchEngine(GraphT * g, NodeInformationHelpDesk * nh, std::vector<string> * n = new std::vector<string>()) : _graph(g), nodeHelpDesk(nh), _names(n) {}
    ~SearchEngine() {}

    inline const void GetCoordinatesForNodeID(NodeID id, _Coordinate& result) const {
        result.lat = nodeHelpDesk->getLatitudeOfNode(id);
        result.lon = nodeHelpDesk->getLongitudeOfNode(id);
    }

    inline void InitializeThreadLocalStorageIfNecessary() {
        if(!_forwardHeap.get())
            _forwardHeap.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData >(nodeHelpDesk->getNumberOfNodes()));
        else
            _forwardHeap->Clear();

        if(!_backwardHeap.get())
            _backwardHeap.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData >(nodeHelpDesk->getNumberOfNodes()));
        else
            _backwardHeap->Clear();
    }

    int ComputeRoute(PhantomNodes & phantomNodes, std::vector<_PathData> & path) {
        int _upperbound = INT_MAX;
        if(!phantomNodes.AtLeastOnePhantomNodeIsUINTMAX())
            return _upperbound;

        InitializeThreadLocalStorageIfNecessary();
        NodeID middle = ( NodeID ) UINT_MAX;
        //insert start and/or target node of start edge
        _forwardHeap->Insert(phantomNodes.startPhantom.edgeBasedNode, -phantomNodes.startPhantom.weight1, phantomNodes.startPhantom.edgeBasedNode);
//        INFO("a) forw insert " << phantomNodes.startPhantom.edgeBasedNode << ", weight: " << -phantomNodes.startPhantom.weight1);
        if(phantomNodes.startPhantom.isBidirected() ) {
//            INFO("b) forw insert " << phantomNodes.startPhantom.edgeBasedNode+1 << ", weight: " << -phantomNodes.startPhantom.weight2);
            _forwardHeap->Insert(phantomNodes.startPhantom.edgeBasedNode+1, -phantomNodes.startPhantom.weight2, phantomNodes.startPhantom.edgeBasedNode+1);
        }
        //insert start and/or target node of target edge id
        _backwardHeap->Insert(phantomNodes.targetPhantom.edgeBasedNode, phantomNodes.targetPhantom.weight1, phantomNodes.targetPhantom.edgeBasedNode);
//        INFO("c) back insert " << phantomNodes.targetPhantom.edgeBasedNode << ", weight: " << phantomNodes.targetPhantom.weight1);
        if(phantomNodes.targetPhantom.isBidirected() ) {
            _backwardHeap->Insert(phantomNodes.targetPhantom.edgeBasedNode+1, phantomNodes.targetPhantom.weight2, phantomNodes.targetPhantom.edgeBasedNode+1);
//            INFO("d) back insert " << phantomNodes.targetPhantom.edgeBasedNode+1 << ", weight: " << phantomNodes.targetPhantom.weight2);
        }
        int startOffset = (phantomNodes.startPhantom.isBidirected() ? std::max(phantomNodes.startPhantom.weight1, phantomNodes.startPhantom.weight2) : phantomNodes.startPhantom.weight1) ;
        int targetOffset = 0;//(phantomNodes.targetPhantom.isBidirected() ? std::max(phantomNodes.targetPhantom.weight1, phantomNodes.targetPhantom.weight2) : phantomNodes.targetPhantom.weight1) ;

        while(_forwardHeap->Size() + _backwardHeap->Size() > 0){
            if(_forwardHeap->Size() > 0){
                _RoutingStep(_forwardHeap, _backwardHeap, true, &middle, &_upperbound, startOffset);
            }
            if(_backwardHeap->Size() > 0){
                _RoutingStep(_backwardHeap, _forwardHeap, false, &middle, &_upperbound, targetOffset);
            }
        }

//        INFO("-> dist " << _upperbound);
        if ( _upperbound == INT_MAX ) {
            return _upperbound;
        }
        NodeID pathNode = middle;
        deque<NodeID> packedPath;
        while(phantomNodes.startPhantom.edgeBasedNode != pathNode && (!phantomNodes.startPhantom.isBidirected() || phantomNodes.startPhantom.edgeBasedNode+1 != pathNode) ) {
            pathNode = _forwardHeap->GetData(pathNode).parent;
            packedPath.push_front(pathNode);
        }
        packedPath.push_back(middle);
        pathNode = middle;
        while(phantomNodes.targetPhantom.edgeBasedNode != pathNode && (!phantomNodes.targetPhantom.isBidirected() || phantomNodes.targetPhantom.edgeBasedNode+1 != pathNode)) {
            pathNode = _backwardHeap->GetData(pathNode).parent;
            packedPath.push_back(pathNode);
        }
        for(deque<NodeID>::size_type i = 0;i < packedPath.size() - 1;i++){
            _UnpackEdge(packedPath[i], packedPath[i + 1], path);
        }

        return _upperbound;
    }

    inline bool FindRoutingStarts(const _Coordinate & start, const _Coordinate & target, PhantomNodes & routingStarts) const {
        nodeHelpDesk->FindRoutingStarts(start, target, routingStarts);
        return true;
    }

    inline bool FindPhantomNodeForCoordinate(const _Coordinate & location, PhantomNode & result) const {
        return nodeHelpDesk->FindPhantomNodeForCoordinate(location, result);
    }

    inline NodeID GetNameIDForOriginDestinationNodeID(NodeID s, NodeID t) const {
        if(s == t)
            return 0;

        EdgeID e = _graph->FindEdge(s, t);
        if(e == UINT_MAX)
            e = _graph->FindEdge( t, s );
        if(UINT_MAX == e) {
            return 0;
        }
        assert(e != UINT_MAX);
        const EdgeData ed = _graph->GetEdgeData(e);
        return ed.via;
    }

    inline std::string GetEscapedNameForNameID(const NodeID nameID) const {
        return ((nameID >= _names->size() || nameID == 0) ? std::string("") : HTMLEntitize(_names->at(nameID)));
    }

    inline std::string GetEscapedNameForEdgeBasedEdgeID(const unsigned edgeID) const {

        const unsigned nameID = _graph->GetEdgeData(edgeID).nameID1;
        return GetEscapedNameForNameID(nameID);
    }
private:
    inline void _RoutingStep(HeapPtr & _forwardHeap, HeapPtr & _backwardHeap, const bool & forwardDirection, NodeID *middle, int *_upperbound, const int negativeOffset) const {
        const NodeID node = _forwardHeap->DeleteMin();
        const int distance = _forwardHeap->GetKey(node);
//        INFO((forwardDirection ? "[forw]" : "[back]") << " settled node " << node << " at distance " << distance);
        if(_backwardHeap->WasInserted(node) ){
//            INFO((forwardDirection ? "[forw]" : "[back]") << " scanned node " << node << " in both directions");
            const int newDistance = _backwardHeap->GetKey(node) + distance;
            if(newDistance < *_upperbound ){
                if(newDistance>=0 ) {
//                INFO((forwardDirection ? "[forw]" : "[back]") << " settled node " << node << " is new middle at total distance " << newDistance);
                *middle = node;
                *_upperbound = newDistance;
//                } else {
//                    INFO((forwardDirection ? "[forw]" : "[back]") << " ignored " << node << " as new middle at total distance " << newDistance);
                }
            }
        }

        if(distance-negativeOffset > *_upperbound){
            _forwardHeap->DeleteAll();
            return;
        }

        for ( typename GraphT::EdgeIterator edge = _graph->BeginEdges( node ); edge < _graph->EndEdges(node); edge++ ) {
            const EdgeData & data = _graph->GetEdgeData(edge);
            bool backwardDirectionFlag = (!forwardDirection) ? data.forward : data.backward;
            if(backwardDirectionFlag) {
                const NodeID to = _graph->GetTarget(edge);
                const int edgeWeight = data.distance;

                assert( edgeWeight > 0 );

                //Stalling
                if(_forwardHeap->WasInserted( to )) {
                    if(_forwardHeap->GetKey( to ) + edgeWeight < distance) {
                        return;
                    }
                }
            }
        }

        for ( typename GraphT::EdgeIterator edge = _graph->BeginEdges( node ); edge < _graph->EndEdges(node); edge++ ) {
            const EdgeData & data = _graph->GetEdgeData(edge);
            bool forwardDirectionFlag = (forwardDirection ? data.forward : data.backward );
            if(forwardDirectionFlag) {

                const NodeID to = _graph->GetTarget(edge);
                const int edgeWeight = data.distance;

                assert( edgeWeight > 0 );
                const int toDistance = distance + edgeWeight;

                //New Node discovered -> Add to Heap + Node Info Storage
                if ( !_forwardHeap->WasInserted( to ) ) {
//                    INFO((forwardDirection ? "[forw]" : "[back]") << " scanning edge (" << node << "," << to << ") with distance " << toDistance << ", edge length: " << data.distance);
                    _forwardHeap->Insert( to, toDistance, node );
                }
                //Found a shorter Path -> Update distance
                else if ( toDistance < _forwardHeap->GetKey( to ) ) {
//                    INFO((forwardDirection ? "[forw]" : "[back]") << " decrease and scanning edge (" << node << "," << to << ") with distance " << toDistance << ", edge length: " << data.distance);
                    _forwardHeap->GetData( to ).parent = node;
                    _forwardHeap->DecreaseKey( to, toDistance );
                    //new parent
                }
            }
        }
    }

    inline bool _UnpackEdge(const NodeID source, const NodeID target, std::vector<_PathData> & path) const {
        assert(source != target);
        //find edge first.
        typename GraphT::EdgeIterator smallestEdge = SPECIAL_EDGEID;
        int smallestWeight = INT_MAX;
        for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(source);eit < _graph->EndEdges(source);eit++){
            const int weight = _graph->GetEdgeData(eit).distance;
            if(_graph->GetTarget(eit) == target && weight < smallestWeight && _graph->GetEdgeData(eit).forward){
                smallestEdge = eit;
                smallestWeight = weight;
            }
        }

        if(smallestEdge == SPECIAL_EDGEID){
            for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(target);eit < _graph->EndEdges(target);eit++){
                const int weight = _graph->GetEdgeData(eit).distance;
                if(_graph->GetTarget(eit) == source && weight < smallestWeight && _graph->GetEdgeData(eit).backward){
                    smallestEdge = eit;
                    smallestWeight = weight;
                }
            }
        }

        assert(smallestWeight != INT_MAX);

        const EdgeData& ed = _graph->GetEdgeData(smallestEdge);
        if(ed.shortcut) {//unpack
            const NodeID middle = ed.via;
            _UnpackEdge(source, middle, path);
            _UnpackEdge(middle, target, path);
            return false;
        } else {
            assert(!ed.shortcut);
            path.push_back(_PathData(ed.via, ed.nameID, ed.turnInstruction, ed.distance) );
            return true;
        }
    }
};
template<class EdgeData, class GraphT> HeapPtr SearchEngine<EdgeData, GraphT>::_forwardHeap;
template<class EdgeData, class GraphT> HeapPtr SearchEngine<EdgeData, GraphT>::_backwardHeap;

#endif /* SEARCHENGINE_H_ */
