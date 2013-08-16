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



#ifndef BASICROUTINGINTERFACE_H_
#define BASICROUTINGINTERFACE_H_

#include "../DataStructures/RawRouteData.h"
#include "../Util/ContainerUtils.h"
#include "../Util/SimpleLogger.h"

#include <boost/noncopyable.hpp>

#include <cassert>
#include <climits>

#include <stack>

template<class QueryDataT>
class BasicRoutingInterface : boost::noncopyable{
protected:
    QueryDataT & _queryData;
public:
    BasicRoutingInterface(QueryDataT & qd) : _queryData(qd) { }
    virtual ~BasicRoutingInterface(){ };

    inline void RoutingStep(typename QueryDataT::QueryHeap & _forwardHeap, typename QueryDataT::QueryHeap & _backwardHeap, NodeID *middle, int *_upperbound, const int edgeBasedOffset, const bool forwardDirection) const {
        const NodeID node = _forwardHeap.DeleteMin();
        const int distance = _forwardHeap.GetKey(node);
        //SimpleLogger().Write() << "Settled (" << _forwardHeap.GetData( node ).parent << "," << node << ")=" << distance;
        if(_backwardHeap.WasInserted(node) ){
            const int newDistance = _backwardHeap.GetKey(node) + distance;
            if(newDistance < *_upperbound ){
                if(newDistance>=0 ) {
                    *middle = node;
                    *_upperbound = newDistance;
                } else {
                }
            }
        }

        if(distance-edgeBasedOffset > *_upperbound){
            _forwardHeap.DeleteAll();
            return;
        }

        //Stalling
        for ( typename QueryDataT::Graph::EdgeIterator edge = _queryData.graph->BeginEdges( node ); edge < _queryData.graph->EndEdges(node); ++edge ) {
            const typename QueryDataT::Graph::EdgeData & data = _queryData.graph->GetEdgeData(edge);
            bool backwardDirectionFlag = (!forwardDirection) ? data.forward : data.backward;
            if(backwardDirectionFlag) {
                const NodeID to = _queryData.graph->GetTarget(edge);
                const int edgeWeight = data.distance;

                assert( edgeWeight > 0 );

                if(_forwardHeap.WasInserted( to )) {
                    if(_forwardHeap.GetKey( to ) + edgeWeight < distance) {
                        return;
                    }
                }
            }
        }

        for ( typename QueryDataT::Graph::EdgeIterator edge = _queryData.graph->BeginEdges( node ); edge < _queryData.graph->EndEdges(node); ++edge ) {
            const typename QueryDataT::Graph::EdgeData & data = _queryData.graph->GetEdgeData(edge);
            bool forwardDirectionFlag = (forwardDirection ? data.forward : data.backward );
            if(forwardDirectionFlag) {

                const NodeID to = _queryData.graph->GetTarget(edge);
                const int edgeWeight = data.distance;

                assert( edgeWeight > 0 );
                const int toDistance = distance + edgeWeight;

                //New Node discovered -> Add to Heap + Node Info Storage
                if ( !_forwardHeap.WasInserted( to ) ) {
                    _forwardHeap.Insert( to, toDistance, node );
                }
                //Found a shorter Path -> Update distance
                else if ( toDistance < _forwardHeap.GetKey( to ) ) {
                    _forwardHeap.GetData( to ).parent = node;
                    _forwardHeap.DecreaseKey( to, toDistance );
                    //new parent
                }
            }
        }
    }

    inline void UnpackPath(const std::vector<NodeID> & packedPath, std::vector<_PathData> & unpackedPath) const {
        const unsigned sizeOfPackedPath = packedPath.size();
        std::stack<std::pair<NodeID, NodeID> > recursionStack;

        //We have to push the path in reverse order onto the stack because it's LIFO.
        for(unsigned i = sizeOfPackedPath-1; i > 0; --i){
            recursionStack.push(std::make_pair(packedPath[i-1], packedPath[i]));
        }

        std::pair<NodeID, NodeID> edge;
        while(!recursionStack.empty()) {
            edge = recursionStack.top();
            recursionStack.pop();

            typename QueryDataT::Graph::EdgeIterator smallestEdge = SPECIAL_EDGEID;
            int smallestWeight = INT_MAX;
            for(typename QueryDataT::Graph::EdgeIterator eit = _queryData.graph->BeginEdges(edge.first);eit < _queryData.graph->EndEdges(edge.first);++eit){
                const int weight = _queryData.graph->GetEdgeData(eit).distance;
                if(_queryData.graph->GetTarget(eit) == edge.second && weight < smallestWeight && _queryData.graph->GetEdgeData(eit).forward){
                    smallestEdge = eit;
                    smallestWeight = weight;
                }
            }

            if(smallestEdge == SPECIAL_EDGEID){
                for(typename QueryDataT::Graph::EdgeIterator eit = _queryData.graph->BeginEdges(edge.second);eit < _queryData.graph->EndEdges(edge.second);++eit){
                    const int weight = _queryData.graph->GetEdgeData(eit).distance;
                    if(_queryData.graph->GetTarget(eit) == edge.first && weight < smallestWeight && _queryData.graph->GetEdgeData(eit).backward){
                        smallestEdge = eit;
                        smallestWeight = weight;
                    }
                }
            }
            assert(smallestWeight != INT_MAX);

            const typename QueryDataT::Graph::EdgeData& ed = _queryData.graph->GetEdgeData(smallestEdge);
            if(ed.shortcut) {//unpack
                const NodeID middle = ed.id;
                //again, we need to this in reversed order
                recursionStack.push(std::make_pair(middle, edge.second));
                recursionStack.push(std::make_pair(edge.first, middle));
            } else {
                assert(!ed.shortcut);
                unpackedPath.push_back(
                    _PathData(
                        ed.id,
                        _queryData.nodeHelpDesk->GetNameIndexFromEdgeID(ed.id),
                        _queryData.nodeHelpDesk->GetTurnInstructionForEdgeID(ed.id),
                        ed.distance
                    )
                );
            }
        }
    }

    inline void UnpackEdge(const NodeID s, const NodeID t, std::vector<NodeID> & unpackedPath) const {
        std::stack<std::pair<NodeID, NodeID> > recursionStack;
        recursionStack.push(std::make_pair(s,t));

        std::pair<NodeID, NodeID> edge;
        while(!recursionStack.empty()) {
            edge = recursionStack.top();
            recursionStack.pop();

            typename QueryDataT::Graph::EdgeIterator smallestEdge = SPECIAL_EDGEID;
            int smallestWeight = INT_MAX;
            for(typename QueryDataT::Graph::EdgeIterator eit = _queryData.graph->BeginEdges(edge.first);eit < _queryData.graph->EndEdges(edge.first);++eit){
                const int weight = _queryData.graph->GetEdgeData(eit).distance;
                if(_queryData.graph->GetTarget(eit) == edge.second && weight < smallestWeight && _queryData.graph->GetEdgeData(eit).forward){
                    smallestEdge = eit;
                    smallestWeight = weight;
                }
            }

            if(smallestEdge == SPECIAL_EDGEID){
                for(typename QueryDataT::Graph::EdgeIterator eit = _queryData.graph->BeginEdges(edge.second);eit < _queryData.graph->EndEdges(edge.second);++eit){
                    const int weight = _queryData.graph->GetEdgeData(eit).distance;
                    if(_queryData.graph->GetTarget(eit) == edge.first && weight < smallestWeight && _queryData.graph->GetEdgeData(eit).backward){
                        smallestEdge = eit;
                        smallestWeight = weight;
                    }
                }
            }
            assert(smallestWeight != INT_MAX);

            const typename QueryDataT::Graph::EdgeData& ed = _queryData.graph->GetEdgeData(smallestEdge);
            if(ed.shortcut) {//unpack
                const NodeID middle = ed.id;
                //again, we need to this in reversed order
                recursionStack.push(std::make_pair(middle, edge.second));
                recursionStack.push(std::make_pair(edge.first, middle));
            } else {
                assert(!ed.shortcut);
                unpackedPath.push_back(edge.first );
            }
        }
        unpackedPath.push_back(t);
    }

    inline void RetrievePackedPathFromHeap(typename QueryDataT::QueryHeap & _fHeap, typename QueryDataT::QueryHeap & _bHeap, const NodeID middle, std::vector<NodeID>& packedPath) const {
        NodeID pathNode = middle;
        while(pathNode != _fHeap.GetData(pathNode).parent) {
            pathNode = _fHeap.GetData(pathNode).parent;
            packedPath.push_back(pathNode);
        }

        std::reverse(packedPath.begin(), packedPath.end());
        packedPath.push_back(middle);
        pathNode = middle;
        while (pathNode != _bHeap.GetData(pathNode).parent){
            pathNode = _bHeap.GetData(pathNode).parent;
            packedPath.push_back(pathNode);
    	}
    }

    inline void RetrievePackedPathFromSingleHeap(typename QueryDataT::QueryHeap & search_heap, const NodeID middle, std::vector<NodeID>& packed_path) const {
        NodeID pathNode = middle;
        while(pathNode != search_heap.GetData(pathNode).parent) {
            pathNode = search_heap.GetData(pathNode).parent;
            packed_path.push_back(pathNode);
        }
    }
};


#endif /* BASICROUTINGINTERFACE_H_ */
