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

#ifndef SHORTESTPATHROUTING_H_
#define SHORTESTPATHROUTING_H_

#include "BasicRoutingInterface.h"

#include "../DataStructures/SearchEngineData.h"

template<class DataFacadeT>
class ShortestPathRouting : public BasicRoutingInterface<DataFacadeT>{
    typedef BasicRoutingInterface<DataFacadeT> super;
    typedef SearchEngineData::QueryHeap QueryHeap;
    SearchEngineData & engine_working_data;
public:
    ShortestPathRouting( DataFacadeT * facade, SearchEngineData & engine_working_data) : super(facade), engine_working_data(engine_working_data) {}

    ~ShortestPathRouting() {}

    void operator()(std::vector<PhantomNodes> & phantomNodesVector,  RawRouteData & rawRouteData) const {
        BOOST_FOREACH(const PhantomNodes & phantomNodePair, phantomNodesVector) {
            if(!phantomNodePair.AtLeastOnePhantomNodeIsUINTMAX()) {
                rawRouteData.lengthOfShortestPath = rawRouteData.lengthOfAlternativePath = INT_MAX;
                return;
            }
        }
        int distance1 = 0;
        int distance2 = 0;

        bool searchFrom1stStartNode = true;
        bool searchFrom2ndStartNode = true;
        NodeID middle1 = UINT_MAX;
        NodeID middle2 = UINT_MAX;
        std::vector<NodeID> packedPath1;
        std::vector<NodeID> packedPath2;

        super::_queryData.InitializeOrClearFirstThreadLocalStorage();
        super::_queryData.InitializeOrClearSecondThreadLocalStorage();
        super::_queryData.InitializeOrClearThirdThreadLocalStorage();

        QueryHeap & forward_heap1 = *(super::_queryData.forwardHeap);
        QueryHeap & reverse_heap1 = *(super::_queryData.backwardHeap);
        QueryHeap & forward_heap2 = *(super::_queryData.forwardHeap2);
        QueryHeap & reverse_heap2 = *(super::_queryData.backwardHeap2);

        //Get distance to next pair of target nodes.
        BOOST_FOREACH(const PhantomNodes & phantomNodePair, phantomNodesVector) {
            forward_heap1.Clear();	forward_heap2.Clear();
            reverse_heap1.Clear();	reverse_heap2.Clear();
            int _localUpperbound1 = INT_MAX;
            int _localUpperbound2 = INT_MAX;

            middle1 = UINT_MAX;
            middle2 = UINT_MAX;

            //insert new starting nodes into forward heap, adjusted by previous distances.
            if(searchFrom1stStartNode) {
                forward_heap1.Insert(phantomNodePair.startPhantom.edgeBasedNode, distance1-phantomNodePair.startPhantom.weight1, phantomNodePair.startPhantom.edgeBasedNode);
                // INFO("fw1: " << phantomNodePair.startPhantom.edgeBasedNode << "´, w: " << -phantomNodePair.startPhantom.weight1);
                forward_heap2.Insert(phantomNodePair.startPhantom.edgeBasedNode, distance1-phantomNodePair.startPhantom.weight1, phantomNodePair.startPhantom.edgeBasedNode);
                // INFO("fw2: " << phantomNodePair.startPhantom.edgeBasedNode << "´, w: " << -phantomNodePair.startPhantom.weight1);
           }
            if(phantomNodePair.startPhantom.isBidirected() && searchFrom2ndStartNode) {
                forward_heap1.Insert(phantomNodePair.startPhantom.edgeBasedNode+1, distance2-phantomNodePair.startPhantom.weight2, phantomNodePair.startPhantom.edgeBasedNode+1);
                // INFO("fw1: " << phantomNodePair.startPhantom.edgeBasedNode+1 << "´, w: " << -phantomNodePair.startPhantom.weight2);
                forward_heap2.Insert(phantomNodePair.startPhantom.edgeBasedNode+1, distance2-phantomNodePair.startPhantom.weight2, phantomNodePair.startPhantom.edgeBasedNode+1);
                // INFO("fw2: " << phantomNodePair.startPhantom.edgeBasedNode+1 << "´, w: " << -phantomNodePair.startPhantom.weight2);
            }

            //insert new backward nodes into backward heap, unadjusted.
            reverse_heap1.Insert(phantomNodePair.targetPhantom.edgeBasedNode, phantomNodePair.targetPhantom.weight1, phantomNodePair.targetPhantom.edgeBasedNode);
            // INFO("rv1: " << phantomNodePair.targetPhantom.edgeBasedNode << ", w;" << phantomNodePair.targetPhantom.weight1 );
            if(phantomNodePair.targetPhantom.isBidirected() ) {
                reverse_heap2.Insert(phantomNodePair.targetPhantom.edgeBasedNode+1, phantomNodePair.targetPhantom.weight2, phantomNodePair.targetPhantom.edgeBasedNode+1);
                // INFO("rv2: " << phantomNodePair.targetPhantom.edgeBasedNode+1 << ", w;" << phantomNodePair.targetPhantom.weight2 );
           }
            const int forward_offset = phantomNodePair.startPhantom.weight1 + (phantomNodePair.startPhantom.isBidirected() ? phantomNodePair.startPhantom.weight2 : 0);
            const int reverse_offset = phantomNodePair.targetPhantom.weight1 + (phantomNodePair.targetPhantom.isBidirected() ? phantomNodePair.targetPhantom.weight2 : 0);

            //run two-Target Dijkstra routing step.
            while(0 < (forward_heap1.Size() + reverse_heap1.Size() )){
                if(0 < forward_heap1.Size()){
                    super::RoutingStep(forward_heap1, reverse_heap1, &middle1, &_localUpperbound1, forward_offset, true);
                }
                if(0 < reverse_heap1.Size() ){
                    super::RoutingStep(reverse_heap1, forward_heap1, &middle1, &_localUpperbound1, reverse_offset, false);
                }
            }
            if(0 < reverse_heap2.Size()) {
                while(0 < (forward_heap2.Size() + reverse_heap2.Size() )){
                    if(0 < forward_heap2.Size()){
                        super::RoutingStep(forward_heap2, reverse_heap2, &middle2, &_localUpperbound2, forward_offset, true);
                    }
                    if(0 < reverse_heap2.Size()){
                        super::RoutingStep(reverse_heap2, forward_heap2, &middle2, &_localUpperbound2, reverse_offset, false);
                    }
                }
            }

            //No path found for both target nodes?
            if((INT_MAX == _localUpperbound1) && (INT_MAX == _localUpperbound2)) {
                rawRouteData.lengthOfShortestPath = rawRouteData.lengthOfAlternativePath = INT_MAX;
                return;
            }
            if(UINT_MAX == middle1) {
                searchFrom1stStartNode = false;
            }
            if(UINT_MAX == middle2) {
                searchFrom2ndStartNode = false;
            }

            //Was at most one of the two paths not found?
            assert(!(INT_MAX == distance1 && INT_MAX == distance2));

            //Unpack paths if they exist
            std::vector<NodeID> temporaryPackedPath1;
            std::vector<NodeID> temporaryPackedPath2;
            if(INT_MAX != _localUpperbound1) {
                super::RetrievePackedPathFromHeap(forward_heap1, reverse_heap1, middle1, temporaryPackedPath1);
            }

            if(INT_MAX != _localUpperbound2) {
                super::RetrievePackedPathFromHeap(forward_heap2, reverse_heap2, middle2, temporaryPackedPath2);
            }

            //if one of the paths was not found, replace it with the other one.
            if(0 == temporaryPackedPath1.size()) {
                temporaryPackedPath1.insert(temporaryPackedPath1.end(), temporaryPackedPath2.begin(), temporaryPackedPath2.end());
                _localUpperbound1 = _localUpperbound2;
            }
            if(0 == temporaryPackedPath2.size()) {
                temporaryPackedPath2.insert(temporaryPackedPath2.end(), temporaryPackedPath1.begin(), temporaryPackedPath1.end());
                _localUpperbound2 = _localUpperbound1;
            }

            assert(0 < temporaryPackedPath1.size() && 0 < temporaryPackedPath2.size());

            //Plug paths together, s.t. end of packed path is begin of temporary packed path
            if(0 < packedPath1.size() && 0 < packedPath2.size() ) {
                if( *(temporaryPackedPath1.begin()) == *(temporaryPackedPath2.begin())) {
                    //both new route segments start with the same node, thus one of the packedPath must go.
                    assert( (packedPath1.size() == packedPath2.size() ) || (*(packedPath1.end()-1) != *(packedPath2.end()-1)) );
                    if( *(packedPath1.end()-1) == *(temporaryPackedPath1.begin())) {
                        packedPath2.clear();
                        packedPath2.insert(packedPath2.end(), packedPath1.begin(), packedPath1.end());
                    } else {
                        packedPath1.clear();
                        packedPath1.insert(packedPath1.end(), packedPath2.begin(), packedPath2.end());
                    }
                } else  {
                    //packed paths 1 and 2 may need to switch.
                    if(*(packedPath1.end()-1) != *(temporaryPackedPath1.begin())) {
                        packedPath1.swap(packedPath2);
                        std::swap(distance1, distance2);
                    }
                }
            }
            packedPath1.insert(packedPath1.end(), temporaryPackedPath1.begin(), temporaryPackedPath1.end());
            packedPath2.insert(packedPath2.end(), temporaryPackedPath2.begin(), temporaryPackedPath2.end());

            if( (packedPath1.back() == packedPath2.back()) && phantomNodePair.targetPhantom.isBidirected() ) {

                NodeID lastNodeID = packedPath2.back();
                searchFrom1stStartNode &= !(lastNodeID == phantomNodePair.targetPhantom.edgeBasedNode+1);
                searchFrom2ndStartNode &= !(lastNodeID == phantomNodePair.targetPhantom.edgeBasedNode);
            }

            distance1 = _localUpperbound1;
            distance2 = _localUpperbound2;
        }

        if(distance1 > distance2){
            std::swap(packedPath1, packedPath2);
        }
        remove_consecutive_duplicates_from_vector(packedPath1);
        super::UnpackPath(packedPath1, rawRouteData.computedShortestPath);
        rawRouteData.lengthOfShortestPath = std::min(distance1, distance2);
        return;
    }
};

#endif /* SHORTESTPATHROUTING_H_ */
