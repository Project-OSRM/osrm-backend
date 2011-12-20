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
#include "SimpleStack.h"

#include <boost/thread.hpp>

#include "BinaryHeap.h"
#include "PhantomNodes.h"
#include "../Util/StringUtil.h"
#include "../typedefs.h"

struct _HeapData {
    NodeID parent;
    _HeapData( NodeID p ) : parent(p) { }
};

struct _ViaHeapData {
    NodeID parent;
    NodeID sourceNode;
    _ViaHeapData(NodeID id) :parent(id), sourceNode(id) { }
};

typedef boost::thread_specific_ptr<BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> > > HeapPtr;
typedef boost::thread_specific_ptr<BinaryHeap< NodeID, NodeID, int, _ViaHeapData, UnorderedMapStorage<NodeID, int> > > ViaHeapPtr;

template<class EdgeData, class GraphT>
class SearchEngine {
private:
    const GraphT * _graph;
    NodeInformationHelpDesk * nodeHelpDesk;
    std::vector<string> * _names;
    static HeapPtr _forwardHeap;
    static HeapPtr _backwardHeap;
    static ViaHeapPtr _forwardViaHeap;
    static ViaHeapPtr _backwardViaHeap;
    inline double absDouble(double input) { if(input < 0) return input*(-1); else return input;}
public:
    SearchEngine(GraphT * g, NodeInformationHelpDesk * nh, std::vector<string> * n = new std::vector<string>()) : _graph(g), nodeHelpDesk(nh), _names(n) {}
    ~SearchEngine() {}

    inline const void GetCoordinatesForNodeID(NodeID id, _Coordinate& result) const {
        result.lat = nodeHelpDesk->getLatitudeOfNode(id);
        result.lon = nodeHelpDesk->getLongitudeOfNode(id);
    }

    inline void InitializeThreadLocalStorageIfNecessary() {
        if(!_forwardHeap.get()) {
            _forwardHeap.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> >(nodeHelpDesk->getNumberOfNodes()));
        }
        else
            _forwardHeap->Clear();

        if(!_backwardHeap.get()) {
            _backwardHeap.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> >(nodeHelpDesk->getNumberOfNodes()));
        }
        else
            _backwardHeap->Clear();
    }

    inline void InitializeThreadLocalViaStorageIfNecessary() {
        if(!_forwardViaHeap.get()) {
            _forwardViaHeap.reset(new BinaryHeap< NodeID, NodeID, int, _ViaHeapData, UnorderedMapStorage<NodeID, int> >(nodeHelpDesk->getNumberOfNodes()));
        }
        else
            _forwardViaHeap->Clear();

        if(!_backwardViaHeap.get()) {
            _backwardViaHeap.reset(new BinaryHeap< NodeID, NodeID, int, _ViaHeapData, UnorderedMapStorage<NodeID, int> >(nodeHelpDesk->getNumberOfNodes()));
        }
        else
            _backwardViaHeap->Clear();
    }

    inline int ComputeViaRoute(std::vector<PhantomNodes> & phantomNodesVector, std::vector<_PathData> & unpackedPath) {
        BOOST_FOREACH(PhantomNodes & phantomNodePair, phantomNodesVector) {
            if(!phantomNodePair.AtLeastOnePhantomNodeIsUINTMAX())
                return INT_MAX;
        }

        int distance1 = 0;
        int distance2 = 0;

        std::deque<NodeID> packedPath1;
        std::deque<NodeID> packedPath2;

        //Get distance to next pair of target nodes.
        BOOST_FOREACH(PhantomNodes & phantomNodePair, phantomNodesVector) {
            InitializeThreadLocalViaStorageIfNecessary();
            NodeID middle1 = ( NodeID ) UINT_MAX;
            NodeID middle2 = ( NodeID ) UINT_MAX;

            int _upperbound1 = INT_MAX;
            int _upperbound2 = INT_MAX;

            assert(INT_MAX != distance1);

            _forwardViaHeap->Clear();
            //insert new starting nodes into forward heap, adjusted by previous distances.
            _forwardViaHeap->Insert(phantomNodePair.startPhantom.edgeBasedNode, distance1-phantomNodePair.startPhantom.weight1, phantomNodePair.startPhantom.edgeBasedNode);
            if(phantomNodePair.startPhantom.isBidirected() ) {
                _forwardViaHeap->Insert(phantomNodePair.startPhantom.edgeBasedNode+1, distance1-phantomNodePair.startPhantom.weight2, phantomNodePair.startPhantom.edgeBasedNode+1);
            }

            _backwardViaHeap->Clear();
            //insert new backward nodes into backward heap, unadjusted.
            _backwardViaHeap->Insert(phantomNodePair.targetPhantom.edgeBasedNode, phantomNodePair.targetPhantom.weight1, phantomNodePair.targetPhantom.edgeBasedNode);
            if(phantomNodePair.targetPhantom.isBidirected() ) {
                _backwardViaHeap->Insert(phantomNodePair.targetPhantom.edgeBasedNode+1, phantomNodePair.targetPhantom.weight2, phantomNodePair.targetPhantom.edgeBasedNode+1);
            }
            int offset = (phantomNodePair.startPhantom.isBidirected() ? std::max(phantomNodePair.startPhantom.weight1, phantomNodePair.startPhantom.weight2) : phantomNodePair.startPhantom.weight1) ;
            offset += (phantomNodePair.targetPhantom.isBidirected() ? std::max(phantomNodePair.targetPhantom.weight1, phantomNodePair.targetPhantom.weight2) : phantomNodePair.targetPhantom.weight1) ;

            //run two-Target Dijkstra routing step.
            //TODO

            //No path found for both target nodes?
            if(INT_MAX == _upperbound1 && INT_MAX == _upperbound2) {
                return INT_MAX;
            }
            
            //Add distance of segments to current sums
            if(INT_MAX == distance1 || INT_MAX == _upperbound1)
                distance1 = 0;
            distance1 += _upperbound1;
            if(INT_MAX == distance2 || INT_MAX == _upperbound2)
                distance2 = 0;
            distance2 += _upperbound2;

            if(INT_MAX == distance1)
                packedPath1.clear();
            if(INT_MAX == distance2)
                packedPath2.clear();

            //Was one of the previous segments empty?
            bool empty1 = (INT_MAX != distance1 && 0 == packedPath1.size() && 0 != packedPath2.size());
            bool empty2 = (INT_MAX != distance2 && 0 == packedPath2.size() && 0 != packedPath1.size());
            assert(!(empty1 && empty2));
            if(empty1)
                packedPath1.insert(packedPath1.begin(), packedPath2.begin(), packedPath2.end());
            if(empty2)
                packedPath2.insert(packedPath2.begin(), packedPath1.begin(), packedPath2.end());
            
            //set packed paths to current paths.
            NodeID pathNode = middle1;
            std::deque<NodeID> temporaryPackedPath;
            while(phantomNodePair.startPhantom.edgeBasedNode != pathNode && (!phantomNodePair.startPhantom.isBidirected() || phantomNodePair.startPhantom.edgeBasedNode+1 != pathNode) ) {
                pathNode = _forwardHeap->GetData(pathNode).parent;
                temporaryPackedPath.push_front(pathNode);
            }
            temporaryPackedPath.push_back(middle1);
            pathNode = middle1;
            while(phantomNodePair.targetPhantom.edgeBasedNode != pathNode && (!phantomNodePair.targetPhantom.isBidirected() || phantomNodePair.targetPhantom.edgeBasedNode+1 != pathNode)) {
                pathNode = _backwardHeap->GetData(pathNode).parent;
                temporaryPackedPath.push_back(pathNode);
            }
            packedPath1.insert(packedPath1.end(), temporaryPackedPath.begin(), temporaryPackedPath.end());
            //TODO: add via node turn instruction

            pathNode = middle2;
            temporaryPackedPath.clear();
            while(phantomNodePair.startPhantom.edgeBasedNode != pathNode && (!phantomNodePair.startPhantom.isBidirected() || phantomNodePair.startPhantom.edgeBasedNode+1 != pathNode) ) {
                pathNode = _forwardHeap->GetData(pathNode).parent;
                temporaryPackedPath.push_front(pathNode);
            }
            temporaryPackedPath.push_back(middle2);
            pathNode = middle2;
            while(phantomNodePair.targetPhantom.edgeBasedNode != pathNode && (!phantomNodePair.targetPhantom.isBidirected() || phantomNodePair.targetPhantom.edgeBasedNode+1 != pathNode)) {
                pathNode = _backwardHeap->GetData(pathNode).parent;
                temporaryPackedPath.push_back(pathNode);
            }
            //TODO: add via node turn instruction
            packedPath2.insert(packedPath2.end(), temporaryPackedPath.begin(), temporaryPackedPath.end());
        }
        
        if(distance1 < distance2) {
            _UnpackPath(packedPath1, unpackedPath);
        } else {
            _UnpackPath(packedPath2, unpackedPath);
        }

        return std::min(distance1, distance2);
    }

    inline int ComputeRoute(PhantomNodes & phantomNodes, std::vector<_PathData> & path) {
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
        int offset = (phantomNodes.startPhantom.isBidirected() ? std::max(phantomNodes.startPhantom.weight1, phantomNodes.startPhantom.weight2) : phantomNodes.startPhantom.weight1) ;
        offset += (phantomNodes.targetPhantom.isBidirected() ? std::max(phantomNodes.targetPhantom.weight1, phantomNodes.targetPhantom.weight2) : phantomNodes.targetPhantom.weight1) ;

        while(_forwardHeap->Size() + _backwardHeap->Size() > 0){
            if(_forwardHeap->Size() > 0){
                _RoutingStep(_forwardHeap, _backwardHeap, true, &middle, &_upperbound, 2*offset);
            }
            if(_backwardHeap->Size() > 0){
                _RoutingStep(_backwardHeap, _forwardHeap, false, &middle, &_upperbound, 2*offset);
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
        _UnpackPath(packedPath, path);
        return _upperbound;
    }

    inline void FindRoutingStarts(const _Coordinate & start, const _Coordinate & target, PhantomNodes & routingStarts) const {
        nodeHelpDesk->FindRoutingStarts(start, target, routingStarts);
    }

    inline void FindPhantomNodeForCoordinate(const _Coordinate & location, PhantomNode & result) const {
        nodeHelpDesk->FindPhantomNodeForCoordinate(location, result);
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
    inline void _RoutingStep(HeapPtr & _forwardHeap, HeapPtr & _backwardHeap, const bool & forwardDirection, NodeID *middle, int *_upperbound, const int edgeBasedOffset) const {
        const NodeID node = _forwardHeap->DeleteMin();
        const int distance = _forwardHeap->GetKey(node);
//        INFO((forwardDirection ? "[forw]" : "[back]") << " settled node " << node << " at distance " << distance);
        if(_backwardHeap->WasInserted(node) ){
//        	INFO((forwardDirection ? "[forw]" : "[back]") << " scanned node " << node << " in both directions");
        	const int newDistance = _backwardHeap->GetKey(node) + distance;
        	if(newDistance < *_upperbound ){
        		if(newDistance>=0 ) {
//        			INFO((forwardDirection ? "[forw]" : "[back]") << " -> node " << node << " is new middle at total distance " << newDistance);
        			*middle = node;
        			*_upperbound = newDistance;
        		} else {
//        			INFO((forwardDirection ? "[forw]" : "[back]") << " -> ignored " << node << " as new middle at total distance " << newDistance);
        		}
            }
        }

        if(distance-edgeBasedOffset > *_upperbound){
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
//                    INFO((forwardDirection ? "[forw]" : "[back]") << " decrease and scanning edge (" << node << "," << to << ") from " << _forwardHeap->GetKey(to) << "to " << toDistance << ", edge length: " << data.distance);
                    _forwardHeap->GetData( to ).parent = node;
                    _forwardHeap->DecreaseKey( to, toDistance );
                    //new parent
                }
            }
        }
    }

    inline void _UnpackPath(const std::deque<NodeID> & packedPath, std::vector<_PathData> & unpackedPath) const {
    	const unsigned sizeOfPackedPath = packedPath.size();
    	SimpleStack<std::pair<NodeID, NodeID> > recursionStack(sizeOfPackedPath);

    	//We have to push the path in reverse order onto the stack because it's LIFO.
        for(unsigned i = sizeOfPackedPath-1; i > 0; --i){
        	recursionStack.push(std::make_pair(packedPath[i-1], packedPath[i]));
        }

        std::pair<NodeID, NodeID> edge;
        while(!recursionStack.empty()) {
        	edge = recursionStack.top();
        	recursionStack.pop();

            typename GraphT::EdgeIterator smallestEdge = SPECIAL_EDGEID;
            int smallestWeight = INT_MAX;
            for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(edge.first);eit < _graph->EndEdges(edge.first);++eit){
                const int weight = _graph->GetEdgeData(eit).distance;
                if(_graph->GetTarget(eit) == edge.second && weight < smallestWeight && _graph->GetEdgeData(eit).forward){
                    smallestEdge = eit;
                    smallestWeight = weight;
                }
            }

            if(smallestEdge == SPECIAL_EDGEID){
                for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(edge.second);eit < _graph->EndEdges(edge.second);++eit){
                    const int weight = _graph->GetEdgeData(eit).distance;
                    if(_graph->GetTarget(eit) == edge.first && weight < smallestWeight && _graph->GetEdgeData(eit).backward){
                        smallestEdge = eit;
                        smallestWeight = weight;
                    }
                }
            }

            assert(smallestWeight != INT_MAX);

            const EdgeData& ed = _graph->GetEdgeData(smallestEdge);
            if(ed.shortcut) {//unpack
                const NodeID middle = ed.via;
                //again, we need to this in reversed order
                recursionStack.push(std::make_pair(middle, edge.second));
                recursionStack.push(std::make_pair(edge.first, middle));
            } else {
                assert(!ed.shortcut);
                unpackedPath.push_back(_PathData(ed.via, ed.nameID, ed.turnInstruction, ed.distance) );
            }
        }
    }
};
template<class EdgeData, class GraphT> HeapPtr SearchEngine<EdgeData, GraphT>::_forwardHeap;
template<class EdgeData, class GraphT> HeapPtr SearchEngine<EdgeData, GraphT>::_backwardHeap;

template<class EdgeData, class GraphT> ViaHeapPtr SearchEngine<EdgeData, GraphT>::_forwardViaHeap;
template<class EdgeData, class GraphT> ViaHeapPtr SearchEngine<EdgeData, GraphT>::_backwardViaHeap;

#endif /* SEARCHENGINE_H_ */
