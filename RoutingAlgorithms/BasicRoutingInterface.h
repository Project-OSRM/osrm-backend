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

#ifndef BASICROUTINGINTERFACE_H_
#define BASICROUTINGINTERFACE_H_

#include "../DataStructures/RawRouteData.h"
#include "../DataStructures/SearchEngineData.h"
#include "../Util/ContainerUtils.h"
#include "../Util/SimpleLogger.h"

#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>

#include <climits>

#include <stack>

template<class DataFacadeT>
class BasicRoutingInterface : boost::noncopyable{
protected:
    DataFacadeT * facade;
public:
    BasicRoutingInterface(DataFacadeT * facade) : facade(facade) { }
    virtual ~BasicRoutingInterface(){ };

    inline void RoutingStep(
        SearchEngineData::QueryHeap & _forwardHeap,
        SearchEngineData::QueryHeap & _backwardHeap,
        NodeID *middle,
        int *_upperbound,
        const int edgeBasedOffset,
        const bool forwardDirection
    ) const {
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
        for(
            EdgeID edge = facade->BeginEdges( node );
            edge < facade->EndEdges(node);
            ++edge
         ) {
            const typename DataFacadeT::EdgeData & data = facade->GetEdgeData(edge);
            bool backwardDirectionFlag = (!forwardDirection) ? data.forward : data.backward;
            if(backwardDirectionFlag) {
                const NodeID to = facade->GetTarget(edge);
                const int edgeWeight = data.distance;

                BOOST_ASSERT_MSG( edgeWeight > 0, "edgeWeight invalid" );

                if(_forwardHeap.WasInserted( to )) {
                    if(_forwardHeap.GetKey( to ) + edgeWeight < distance) {
                        return;
                    }
                }
            }
        }

        for ( EdgeID edge = facade->BeginEdges( node ); edge < facade->EndEdges(node); ++edge ) {
            const typename DataFacadeT::EdgeData & data = facade->GetEdgeData(edge);
            bool forwardDirectionFlag = (forwardDirection ? data.forward : data.backward );
            if(forwardDirectionFlag) {

                const NodeID to = facade->GetTarget(edge);
                const int edgeWeight = data.distance;

                BOOST_ASSERT_MSG( edgeWeight > 0, "edgeWeight invalid" );
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

            EdgeID smallestEdge = SPECIAL_EDGEID;
            int smallestWeight = INT_MAX;
            for(EdgeID eit = facade->BeginEdges(edge.first);eit < facade->EndEdges(edge.first);++eit){
                const int weight = facade->GetEdgeData(eit).distance;
                if(facade->GetTarget(eit) == edge.second && weight < smallestWeight && facade->GetEdgeData(eit).forward){
                    smallestEdge = eit;
                    smallestWeight = weight;
                }
            }

            if(smallestEdge == SPECIAL_EDGEID){
                for(EdgeID eit = facade->BeginEdges(edge.second);eit < facade->EndEdges(edge.second);++eit){
                    const int weight = facade->GetEdgeData(eit).distance;
                    if(facade->GetTarget(eit) == edge.first && weight < smallestWeight && facade->GetEdgeData(eit).backward){
                        smallestEdge = eit;
                        smallestWeight = weight;
                    }
                }
            }
            BOOST_ASSERT_MSG(smallestWeight != INT_MAX, "edge id invalid");

            const typename DataFacadeT::EdgeData& ed = facade->GetEdgeData(smallestEdge);
            if(ed.shortcut) {//unpack
                const NodeID middle = ed.id;
                //again, we need to this in reversed order
                recursionStack.push(std::make_pair(middle, edge.second));
                recursionStack.push(std::make_pair(edge.first, middle));
            } else {
                BOOST_ASSERT_MSG(!ed.shortcut, "edge must be a shortcut");
                unpackedPath.push_back(
                    _PathData(
                        ed.id,
                        facade->GetNameIndexFromEdgeID(ed.id),
                        facade->GetTurnInstructionForEdgeID(ed.id),
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

            EdgeID smallestEdge = SPECIAL_EDGEID;
            int smallestWeight = INT_MAX;
            for(EdgeID eit = facade->BeginEdges(edge.first);eit < facade->EndEdges(edge.first);++eit){
                const int weight = facade->GetEdgeData(eit).distance;
                if(facade->GetTarget(eit) == edge.second && weight < smallestWeight && facade->GetEdgeData(eit).forward){
                    smallestEdge = eit;
                    smallestWeight = weight;
                }
            }

            if(smallestEdge == SPECIAL_EDGEID){
                for(EdgeID eit = facade->BeginEdges(edge.second);eit < facade->EndEdges(edge.second);++eit){
                    const int weight = facade->GetEdgeData(eit).distance;
                    if(facade->GetTarget(eit) == edge.first && weight < smallestWeight && facade->GetEdgeData(eit).backward){
                        smallestEdge = eit;
                        smallestWeight = weight;
                    }
                }
            }
            BOOST_ASSERT_MSG(smallestWeight != INT_MAX, "edge weight invalid");

            const typename DataFacadeT::EdgeData& ed = facade->GetEdgeData(smallestEdge);
            if(ed.shortcut) {//unpack
                const NodeID middle = ed.id;
                //again, we need to this in reversed order
                recursionStack.push(std::make_pair(middle, edge.second));
                recursionStack.push(std::make_pair(edge.first, middle));
            } else {
                BOOST_ASSERT_MSG(!ed.shortcut, "edge must be shortcut");
                unpackedPath.push_back(edge.first );
            }
        }
        unpackedPath.push_back(t);
    }

    inline void RetrievePackedPathFromHeap(
        SearchEngineData::QueryHeap & _fHeap,
        SearchEngineData::QueryHeap & _bHeap,
        const NodeID middle,
        std::vector<NodeID> & packedPath
    ) const {
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

    inline void RetrievePackedPathFromSingleHeap(SearchEngineData::QueryHeap & search_heap, const NodeID middle, std::vector<NodeID>& packed_path) const {
        NodeID pathNode = middle;
        while(pathNode != search_heap.GetData(pathNode).parent) {
            pathNode = search_heap.GetData(pathNode).parent;
            packed_path.push_back(pathNode);
        }
    }
};


#endif /* BASICROUTINGINTERFACE_H_ */
