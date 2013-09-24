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

#include <boost/assert.hpp>

#include "BasicRoutingInterface.h"
#include "../DataStructures/SearchEngineData.h"

template<class DataFacadeT>
class ShortestPathRouting : public BasicRoutingInterface<DataFacadeT>{
    typedef BasicRoutingInterface<DataFacadeT> super;
    typedef SearchEngineData::QueryHeap QueryHeap;
    SearchEngineData & engine_working_data;

public:
    ShortestPathRouting(
        DataFacadeT * facade,
        SearchEngineData & engine_working_data
    ) :
        super(facade),
        engine_working_data(engine_working_data)
    {}

    ~ShortestPathRouting() {}

    void operator()(
        std::vector<PhantomNodes> & phantom_nodes_vector,
        RawRouteData & raw_route_data
    ) const {
        BOOST_FOREACH(
            const PhantomNodes & phantom_node_pair,
            phantom_nodes_vector
        ){
            if(!phantom_node_pair.AtLeastOnePhantomNodeIsUINTMAX()) {
                raw_route_data.lengthOfShortestPath = INT_MAX;
                raw_route_data.lengthOfAlternativePath = INT_MAX;
                return;
            }
        }
        int distance1 = 0;
        int distance2 = 0;

        bool search_from_1st_node = true;
        bool search_from_2nd_node = true;
        NodeID middle1 = UINT_MAX;
        NodeID middle2 = UINT_MAX;
        std::vector<NodeID> packed_path1;
        std::vector<NodeID> packed_path2;

        engine_working_data.InitializeOrClearFirstThreadLocalStorage(
            super::facade->GetNumberOfNodes()
        );
        engine_working_data.InitializeOrClearSecondThreadLocalStorage(
            super::facade->GetNumberOfNodes()
        );
        engine_working_data.InitializeOrClearThirdThreadLocalStorage(
            super::facade->GetNumberOfNodes()
        );

        QueryHeap & forward_heap1 = *(engine_working_data.forwardHeap);
        QueryHeap & reverse_heap1 = *(engine_working_data.backwardHeap);
        QueryHeap & forward_heap2 = *(engine_working_data.forwardHeap2);
        QueryHeap & reverse_heap2 = *(engine_working_data.backwardHeap2);

        //Get distance to next pair of target nodes.
        BOOST_FOREACH(
            const PhantomNodes & phantom_node_pair,
            phantom_nodes_vector
        ){
            forward_heap1.Clear();	forward_heap2.Clear();
            reverse_heap1.Clear();	reverse_heap2.Clear();
            int local_upper_bound1 = INT_MAX;
            int local_upper_bound2 = INT_MAX;

            middle1 = UINT_MAX;
            middle2 = UINT_MAX;

            //insert new starting nodes into forward heap, adjusted by previous distances.
            if(search_from_1st_node) {
                forward_heap1.Insert(
                    phantom_node_pair.startPhantom.edgeBasedNode,
                    distance1-phantom_node_pair.startPhantom.weight1,
                    phantom_node_pair.startPhantom.edgeBasedNode
                );
                // INFO("fw1: " << phantom_node_pair.startPhantom.edgeBasedNode << "´, w: " << -phantomNodePair.startPhantom.weight1);
                forward_heap2.Insert(
                    phantom_node_pair.startPhantom.edgeBasedNode,
                    distance1-phantom_node_pair.startPhantom.weight1,
                    phantom_node_pair.startPhantom.edgeBasedNode
                );
                // INFO("fw2: " << phantom_node_pair.startPhantom.edgeBasedNode << "´, w: " << -phantomNodePair.startPhantom.weight1);
           }
            if(phantom_node_pair.startPhantom.isBidirected() && search_from_2nd_node) {
                forward_heap1.Insert(
                    phantom_node_pair.startPhantom.edgeBasedNode+1,
                    distance2-phantom_node_pair.startPhantom.weight2,
                    phantom_node_pair.startPhantom.edgeBasedNode+1
                );
                // INFO("fw1: " << phantom_node_pair.startPhantom.edgeBasedNode+1 << "´, w: " << -phantomNodePair.startPhantom.weight2);
                forward_heap2.Insert(
                    phantom_node_pair.startPhantom.edgeBasedNode+1,
                    distance2-phantom_node_pair.startPhantom.weight2,
                    phantom_node_pair.startPhantom.edgeBasedNode+1
                );
                // INFO("fw2: " << phantom_node_pair.startPhantom.edgeBasedNode+1 << "´, w: " << -phantomNodePair.startPhantom.weight2);
            }

            //insert new backward nodes into backward heap, unadjusted.
            reverse_heap1.Insert(
                phantom_node_pair.targetPhantom.edgeBasedNode,
                phantom_node_pair.targetPhantom.weight1,
                phantom_node_pair.targetPhantom.edgeBasedNode
            );
            // INFO("rv1: " << phantom_node_pair.targetPhantom.edgeBasedNode << ", w;" << phantom_node_pair.targetPhantom.weight1 );
            if(phantom_node_pair.targetPhantom.isBidirected() ) {
                reverse_heap2.Insert(
                    phantom_node_pair.targetPhantom.edgeBasedNode+1,
                    phantom_node_pair.targetPhantom.weight2,
                    phantom_node_pair.targetPhantom.edgeBasedNode+1
                );
                // INFO("rv2: " << phantom_node_pair.targetPhantom.edgeBasedNode+1 << ", w;" << phantom_node_pair.targetPhantom.weight2 );
           }
            const int forward_offset =  super::ComputeEdgeOffset(
                                            phantom_node_pair.startPhantom
                                        );
            const int reverse_offset =  super::ComputeEdgeOffset(
                                            phantom_node_pair.targetPhantom
                                        );

            //run two-Target Dijkstra routing step.
            while(0 < (forward_heap1.Size() + reverse_heap1.Size() )){
                if( !forward_heap1.Empty()){
                    super::RoutingStep(
                        forward_heap1,
                        reverse_heap1,
                        &middle1,
                        &local_upper_bound1,
                        forward_offset,
                        true
                    );
                }
                if( !reverse_heap1.Empty() ){
                    super::RoutingStep(
                        reverse_heap1,
                        forward_heap1,
                        &middle1,
                        &local_upper_bound1,
                        reverse_offset,
                        false
                    );
                }
            }
            if( !reverse_heap2.Empty() ) {
                while(0 < (forward_heap2.Size() + reverse_heap2.Size() )){
                    if( !forward_heap2.Empty() ){
                        super::RoutingStep(
                            forward_heap2,
                            reverse_heap2,
                            &middle2,
                            &local_upper_bound2,
                            forward_offset,
                            true
                        );
                    }
                    if( !reverse_heap2.Empty() ){
                        super::RoutingStep(
                            reverse_heap2,
                            forward_heap2,
                            &middle2,
                            &local_upper_bound2,
                            reverse_offset,
                            false
                        );
                    }
                }
            }

            //No path found for both target nodes?
            if(
                (INT_MAX == local_upper_bound1) &&
                (INT_MAX == local_upper_bound2)
            ) {
                raw_route_data.lengthOfShortestPath = INT_MAX;
                raw_route_data.lengthOfAlternativePath = INT_MAX;
                return;
            }
            if(UINT_MAX == middle1) {
                search_from_1st_node = false;
            }
            if(UINT_MAX == middle2) {
                search_from_2nd_node = false;
            }

            //Was at most one of the two paths not found?
            BOOST_ASSERT_MSG(
                (INT_MAX != distance1 || INT_MAX != distance2),
                "no path found"
            );

            //Unpack paths if they exist
            std::vector<NodeID> temporary_packed_path1;
            std::vector<NodeID> temporary_packed_path2;
            if(INT_MAX != local_upper_bound1) {
                super::RetrievePackedPathFromHeap(
                    forward_heap1,
                    reverse_heap1,
                    middle1,
                    temporary_packed_path1
                );
            }

            if(INT_MAX != local_upper_bound2) {
                super::RetrievePackedPathFromHeap(
                    forward_heap2,
                    reverse_heap2,
                    middle2,
                    temporary_packed_path2
                );
            }

            //if one of the paths was not found, replace it with the other one.
            if( temporary_packed_path1.empty() ) {
                temporary_packed_path1.insert(
                    temporary_packed_path1.end(),
                    temporary_packed_path2.begin(),
                    temporary_packed_path2.end()
                );
                local_upper_bound1 = local_upper_bound2;
            }
            if( temporary_packed_path2.empty() ) {
                temporary_packed_path2.insert(
                    temporary_packed_path2.end(),
                    temporary_packed_path1.begin(),
                    temporary_packed_path1.end()
                );
                local_upper_bound2 = local_upper_bound1;
            }

            BOOST_ASSERT_MSG(
                !temporary_packed_path1.empty() ||
                !temporary_packed_path2.empty(),
                "tempory packed paths empty"
            );

            //Plug paths together, s.t. end of packed path is begin of temporary packed path
            if( !packed_path1.empty() && !packed_path2.empty() ) {
                if(
                    temporary_packed_path1.front() ==
                    temporary_packed_path2.front()
                ) {
                    //both new route segments start with the same node
                    //thus, one of the packedPath must go.
                    BOOST_ASSERT_MSG(
                        (packed_path1.size() == packed_path2.size() ) ||
                        (packed_path1.back() != packed_path2.back() ),
                        "packed paths must be different"
                    );

                    if( packed_path1.back() == temporary_packed_path1.front()) {
                        packed_path2.clear();
                        packed_path2.insert(
                            packed_path2.end(),
                            packed_path1.begin(),
                            packed_path1.end()
                        );
                    } else {
                        packed_path1.clear();
                        packed_path1.insert(
                            packed_path1.end(),
                            packed_path2.begin(),
                            packed_path2.end()
                        );
                    }
                } else  {
                    //packed paths 1 and 2 may need to switch.
                    if( packed_path1.back() != temporary_packed_path1.front()) {
                        packed_path1.swap(packed_path2);
                        std::swap(distance1, distance2);
                    }
                }
            }
            packed_path1.insert(
                packed_path1.end(),
                temporary_packed_path1.begin(),
                temporary_packed_path1.end()
            );
            packed_path2.insert(
                packed_path2.end(),
                temporary_packed_path2.begin(),
                temporary_packed_path2.end()
            );

            if(
                (packed_path1.back() == packed_path2.back()) &&
                phantom_node_pair.targetPhantom.isBidirected()
            ) {
                const NodeID last_node_id = packed_path2.back();
                search_from_1st_node &= !(last_node_id == phantom_node_pair.targetPhantom.edgeBasedNode+1);
                search_from_2nd_node &= !(last_node_id == phantom_node_pair.targetPhantom.edgeBasedNode);
            }

            distance1 = local_upper_bound1;
            distance2 = local_upper_bound2;
        }

        if( distance1 > distance2 ) {
            std::swap( packed_path1, packed_path2 );
        }
        remove_consecutive_duplicates_from_vector(packed_path1);
        super::UnpackPath(packed_path1, raw_route_data.computedShortestPath);
        raw_route_data.lengthOfShortestPath = std::min(distance1, distance2);
    }
};

#endif /* SHORTESTPATHROUTING_H_ */
