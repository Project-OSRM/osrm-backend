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



#ifndef SHORTESTPATHROUTING_H_
#define SHORTESTPATHROUTING_H_

#include "BasicRoutingInterface.h"

template<class QueryDataT>
class ShortestPathRouting : public BasicRoutingInterface<QueryDataT>{
    typedef BasicRoutingInterface<QueryDataT> super;
public:
    ShortestPathRouting(QueryDataT & qd) : super(qd) {}

    ~ShortestPathRouting() {}

    void operator()(std::vector<PhantomNodes> & phantomNodesVector,  RawRouteData & rawRouteData) {
        BOOST_FOREACH(PhantomNodes & phantomNodePair, phantomNodesVector) {
            if(!phantomNodePair.AtLeastOnePhantomNodeIsUINTMAX()) {
                rawRouteData.lengthOfShortestPath = rawRouteData.lengthOfAlternativePath = INT_MAX;
                return;
            }
        }
        int distance1 = 0;
        int distance2 = 0;

        bool searchFrom1stStartNode(true);
        bool searchFrom2ndStartNode(true);
        NodeID middle1 = ( NodeID ) UINT_MAX;
        NodeID middle2 = ( NodeID ) UINT_MAX;
        std::deque<NodeID> packedPath1;
        std::deque<NodeID> packedPath2;

        typename QueryDataT::HeapPtr & forwardHeap = super::_queryData.forwardHeap;
        typename QueryDataT::HeapPtr & backwardHeap = super::_queryData.backwardHeap;

        typename QueryDataT::HeapPtr & forwardHeap2 = super::_queryData.forwardHeap2;
        typename QueryDataT::HeapPtr & backwardHeap2 = super::_queryData.backwardHeap2;


        //Get distance to next pair of target nodes.
        BOOST_FOREACH(PhantomNodes & phantomNodePair, phantomNodesVector) {
            super::_queryData.InitializeOrClearFirstThreadLocalStorage();
            super::_queryData.InitializeOrClearSecondThreadLocalStorage();

            int _localUpperbound1 = INT_MAX;
            int _localUpperbound2 = INT_MAX;

            //insert new starting nodes into forward heap, adjusted by previous distances.
            if(searchFrom1stStartNode) {
                forwardHeap->Insert(phantomNodePair.startPhantom.edgeBasedNode, -phantomNodePair.startPhantom.weight1, phantomNodePair.startPhantom.edgeBasedNode);
                forwardHeap2->Insert(phantomNodePair.startPhantom.edgeBasedNode, -phantomNodePair.startPhantom.weight1, phantomNodePair.startPhantom.edgeBasedNode);
//              INFO("a 1,2)forw insert " << phantomNodePair.startPhantom.edgeBasedNode << " with weight " << phantomNodePair.startPhantom.weight1);
//          } else {
//              INFO("Skipping first start node");
            }
            if(phantomNodePair.startPhantom.isBidirected() && searchFrom2ndStartNode) {
                forwardHeap->Insert(phantomNodePair.startPhantom.edgeBasedNode+1, -phantomNodePair.startPhantom.weight2, phantomNodePair.startPhantom.edgeBasedNode+1);
                forwardHeap2->Insert(phantomNodePair.startPhantom.edgeBasedNode+1, -phantomNodePair.startPhantom.weight2, phantomNodePair.startPhantom.edgeBasedNode+1);
//              INFO("b 1,2)forw insert " << phantomNodePair.startPhantom.edgeBasedNode+1 << " with weight " << -phantomNodePair.startPhantom.weight1);
//          } else if(!searchFrom2ndStartNode) {
//              INFO("Skipping second start node");
            }

//            backwardHeap->Clear();
//            backwardHeap2->Clear();
            //insert new backward nodes into backward heap, unadjusted.
            backwardHeap->Insert(phantomNodePair.targetPhantom.edgeBasedNode, phantomNodePair.targetPhantom.weight1, phantomNodePair.targetPhantom.edgeBasedNode);
//          INFO("1) back insert " << phantomNodePair.targetPhantom.edgeBasedNode << " with weight " << phantomNodePair.targetPhantom.weight1);
            if(phantomNodePair.targetPhantom.isBidirected() ) {
//              INFO("2) back insert " << phantomNodePair.targetPhantom.edgeBasedNode+1 << " with weight " << phantomNodePair.targetPhantom.weight2);
                backwardHeap2->Insert(phantomNodePair.targetPhantom.edgeBasedNode+1, phantomNodePair.targetPhantom.weight2, phantomNodePair.targetPhantom.edgeBasedNode+1);
            }
            int offset = (phantomNodePair.startPhantom.isBidirected() ? std::max(phantomNodePair.startPhantom.weight1, phantomNodePair.startPhantom.weight2) : phantomNodePair.startPhantom.weight1) ;
            offset += (phantomNodePair.targetPhantom.isBidirected() ? std::max(phantomNodePair.targetPhantom.weight1, phantomNodePair.targetPhantom.weight2) : phantomNodePair.targetPhantom.weight1) ;

            //run two-Target Dijkstra routing step.
            while(forwardHeap->Size() + backwardHeap->Size() > 0){
                if(forwardHeap->Size() > 0){
                    super::RoutingStep(forwardHeap, backwardHeap, &middle1, &_localUpperbound1, 2*offset, true);
                }
                if(backwardHeap->Size() > 0){
                    super::RoutingStep(backwardHeap, forwardHeap, &middle1, &_localUpperbound1, 2*offset, false);
                }
            }
            if(backwardHeap2->Size() > 0) {
                while(forwardHeap2->Size() + backwardHeap2->Size() > 0){
                    if(forwardHeap2->Size() > 0){
                        super::RoutingStep(forwardHeap2, backwardHeap2, &middle2, &_localUpperbound2, 2*offset, true);
                    }
                    if(backwardHeap2->Size() > 0){
                        super::RoutingStep(backwardHeap2, forwardHeap2, &middle2, &_localUpperbound2, 2*offset, false);
                    }
                }
            }
//          INFO("upperbound1: " << _localUpperbound1 << ", distance1: " << distance1);
//          INFO("upperbound2: " << _localUpperbound2 << ", distance2: " << distance2);

            //No path found for both target nodes?
            if(INT_MAX == _localUpperbound1 && INT_MAX == _localUpperbound2) {
                rawRouteData.lengthOfShortestPath = rawRouteData.lengthOfAlternativePath = INT_MAX;
                return;
            }
            if(UINT_MAX == middle1) {
                searchFrom1stStartNode = false;
//              INFO("Next Search will not start from 1st");
            } else {
//              INFO("Next Search will start from 1st");
                searchFrom1stStartNode = true;
            }
            if(UINT_MAX == middle2) {
                searchFrom2ndStartNode = false;
//              INFO("Next Search will not start from 2nd");
            } else {
                searchFrom2ndStartNode = true;
//              INFO("Next Search will start from 2nd");
            }

            //Was at most one of the two paths not found?
            assert(!(INT_MAX == distance1 && INT_MAX == distance2));

//          INFO("middle1: " << middle1);

            //Unpack paths if they exist
            std::deque<NodeID> temporaryPackedPath1;
            std::deque<NodeID> temporaryPackedPath2;
            if(INT_MAX != _localUpperbound1) {
                super::RetrievePackedPathFromHeap(forwardHeap, backwardHeap, middle1, temporaryPackedPath1);
//              INFO("temporaryPackedPath1 ends with " << *(temporaryPackedPath1.end()-1) );
            }
//          INFO("middle2: " << middle2);

            if(INT_MAX != _localUpperbound2) {
                super::RetrievePackedPathFromHeap(forwardHeap2, backwardHeap2, middle2, temporaryPackedPath2);
//                INFO("temporaryPackedPath2 ends with " << *(temporaryPackedPath2.end()-1) );
            }

            //if one of the paths was not found, replace it with the other one.
            if(0 == temporaryPackedPath1.size()) {
//              INFO("Deleting path 1");
                temporaryPackedPath1.insert(temporaryPackedPath1.end(), temporaryPackedPath2.begin(), temporaryPackedPath2.end());
                _localUpperbound1 = _localUpperbound2;
            }
            if(0 == temporaryPackedPath2.size()) {
//              INFO("Deleting path 2");
                temporaryPackedPath2.insert(temporaryPackedPath2.end(), temporaryPackedPath1.begin(), temporaryPackedPath1.end());
                _localUpperbound2 = _localUpperbound1;
            }

            assert(0 < temporaryPackedPath1.size() && 0 < temporaryPackedPath2.size());

            //Plug paths together, s.t. end of packed path is begin of temporary packed path
            if(0 < packedPath1.size() && 0 < packedPath2.size() ) {
//              INFO("Both paths are non-empty");
                if( *(temporaryPackedPath1.begin()) == *(temporaryPackedPath2.begin())) {
//                  INFO("both paths start with the same node:" << *(temporaryPackedPath1.begin()));
                    //both new route segments start with the same node, thus one of the packedPath must go.
                    assert( (packedPath1.size() == packedPath2.size() ) || (*(packedPath1.end()-1) != *(packedPath2.end()-1)) );
                    if( *(packedPath1.end()-1) == *(temporaryPackedPath1.begin())) {
//                      INFO("Deleting packedPath2 that ends with " << *(packedPath2.end()-1) << ", other ends with " << *(packedPath1.end()-1));
                        packedPath2.clear();
                        packedPath2.insert(packedPath2.end(), packedPath1.begin(), packedPath1.end());
                        distance2 = distance1;
//                      INFO("packedPath2 now ends with " <<  *(packedPath2.end()-1));
                    } else {
//                      INFO("Deleting path1 that ends with " << *(packedPath1.end()-1) << ", other ends with " << *(packedPath2.end()-1));
                        packedPath1.clear();
                        packedPath1.insert(packedPath1.end(), packedPath2.begin(), packedPath2.end());
                        distance1 = distance2;
//                      INFO("Path1 now ends with " <<  *(packedPath1.end()-1));
                    }
                } else  {
                    //packed paths 1 and 2 may need to switch.
                    if(*(packedPath1.end()-1) != *(temporaryPackedPath1.begin())) {
//                      INFO("Switching");
                        packedPath1.swap(packedPath2);
                        std::swap(distance1, distance2);
                    }
                }
            }
            packedPath1.insert(packedPath1.end(), temporaryPackedPath1.begin(), temporaryPackedPath1.end());
            packedPath2.insert(packedPath2.end(), temporaryPackedPath2.begin(), temporaryPackedPath2.end());

            if( (packedPath1.back() == packedPath2.back()) && phantomNodePair.targetPhantom.isBidirected() ) {
//              INFO("both paths end in same direction on bidirected edge, make sure start only start with : " << packedPath1.back());

                NodeID lastNodeID = packedPath2.back();
                searchFrom1stStartNode &= !(lastNodeID == phantomNodePair.targetPhantom.edgeBasedNode+1);
                searchFrom2ndStartNode &= !(lastNodeID == phantomNodePair.targetPhantom.edgeBasedNode);
//                INFO("Next search from node " << phantomNodePair.targetPhantom.edgeBasedNode << ": " << (searchFrom1stStartNode ? "yes" : "no") );
//                INFO("Next search from node " << phantomNodePair.targetPhantom.edgeBasedNode+1 << ": " << (searchFrom2ndStartNode ? "yes" : "no") );
            }

            distance1 += _localUpperbound1;
            distance2 += _localUpperbound2;
        }

//      INFO("length path1: " << distance1);
//      INFO("length path2: " << distance2);
        if(distance1 <= distance2){
            //remove consecutive duplicates
//          std::cout << "unclean 1: ";
//          for(unsigned i = 0; i < packedPath1.size(); ++i)
//              std::cout << packedPath1[i] << " ";
//          std::cout << std::endl;

//          std::cout << "cleaned 1: ";
//          for(unsigned i = 0; i < packedPath1.size(); ++i)
//              std::cout << packedPath1[i] << " ";
//          std::cout << std::endl;
//            super::UnpackPath(packedPath1, rawRouteData.computedShortestPath);
        } else {
            std::swap(packedPath1, packedPath2);
//          std::cout << "unclean 2: ";
//          for(unsigned i = 0; i < packedPath2.size(); ++i)
//              std::cout << packedPath2[i] << " ";
//          std::cout << std::endl;
//            _RemoveConsecutiveDuplicatesFromContainer(packedPath2);
//          std::cout << "cleaned 2: ";
//          for(unsigned i = 0; i < packedPath2.size(); ++i)
//              std::cout << packedPath2[i] << " ";
//          std::cout << std::endl;
//            super::UnpackPath(packedPath2, unpackedPath);
        }
        _RemoveConsecutiveDuplicatesFromContainer(packedPath1);
        super::UnpackPath(packedPath1, rawRouteData.computedShortestPath);
        rawRouteData.lengthOfShortestPath = std::min(distance1, distance2);
//      INFO("Found via route with distance " << std::min(distance1, distance2));
        return;
    }
private:
    template<class ContainerT>
    void _RemoveConsecutiveDuplicatesFromContainer(ContainerT & packedPath) {
        //remove consecutive duplicates
        typename ContainerT::iterator it;
        // using default comparison:
        it = std::unique(packedPath.begin(), packedPath.end());
        packedPath.resize(it - packedPath.begin());
    }
};

#endif /* SHORTESTPATHROUTING_H_ */
