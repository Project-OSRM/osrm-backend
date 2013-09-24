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

SearchEngineData::SearchEngineHeapPtr SearchEngineData::forwardHeap;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::backwardHeap;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::forwardHeap2;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::backwardHeap2;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::forwardHeap3;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::backwardHeap3;

template<class DataFacadeT>
class BasicRoutingInterface : boost::noncopyable {
private:
    typedef typename DataFacadeT::EdgeData EdgeData;
protected:
    DataFacadeT * facade;
public:
    BasicRoutingInterface( DataFacadeT * facade ) : facade(facade) { }
    virtual ~BasicRoutingInterface(){ };

    inline void RoutingStep(
        SearchEngineData::QueryHeap & forward_heap,
        SearchEngineData::QueryHeap & reverse_heap,
        NodeID * middle_node_id,
        int * upper_bound,
        const int edge_expansion_offset,
        const bool forward_direction
    ) const {
        const NodeID node = forward_heap.DeleteMin();
        const int distance = forward_heap.GetKey(node);
        //SimpleLogger().Write() << "Settled (" << forward_heap.GetData( node ).parent << "," << node << ")=" << distance;
        if(reverse_heap.WasInserted(node) ){
            const int new_distance = reverse_heap.GetKey(node) + distance;
            if(new_distance < *upper_bound ){
                if( new_distance >= 0 ) {
                    *middle_node_id = node;
                    *upper_bound = new_distance;
                }
            }
        }

        if( (distance-edge_expansion_offset) > *upper_bound ){
            forward_heap.DeleteAll();
            return;
        }

        //Stalling
        for(
            EdgeID edge = facade->BeginEdges( node );
            edge < facade->EndEdges(node);
            ++edge
         ) {
            const EdgeData & data = facade->GetEdgeData(edge);
            const bool reverse_flag = (!forward_direction) ? data.forward : data.backward;
            if( reverse_flag ) {
                const NodeID to = facade->GetTarget(edge);
                const int edge_weight = data.distance;

                BOOST_ASSERT_MSG( edge_weight > 0, "edge_weight invalid" );

                if(forward_heap.WasInserted( to )) {
                    if(forward_heap.GetKey( to ) + edge_weight < distance) {
                        return;
                    }
                }
            }
        }

        for(
            EdgeID edge = facade->BeginEdges(node), end_edge = facade->EndEdges(node);
            edge < end_edge;
            ++edge
        ) {
            const EdgeData & data = facade->GetEdgeData(edge);
            bool forward_directionFlag = (forward_direction ? data.forward : data.backward );
            if( forward_directionFlag ) {

                const NodeID to = facade->GetTarget(edge);
                const int edge_weight = data.distance;

                BOOST_ASSERT_MSG( edge_weight > 0, "edge_weight invalid" );
                const int to_distance = distance + edge_weight;

                //New Node discovered -> Add to Heap + Node Info Storage
                if ( !forward_heap.WasInserted( to ) ) {
                    forward_heap.Insert( to, to_distance, node );
                }
                //Found a shorter Path -> Update distance
                else if ( to_distance < forward_heap.GetKey( to ) ) {
                    forward_heap.GetData( to ).parent = node;
                    forward_heap.DecreaseKey( to, to_distance );
                    //new parent
                }
            }
        }
    }

    inline void UnpackPath(
        const std::vector<NodeID> & packed_path,
        std::vector<_PathData> & unpacked_path
    ) const {
        const unsigned packed_path_size = packed_path.size();
        std::stack<std::pair<NodeID, NodeID> > recursion_stack;

        //We have to push the path in reverse order onto the stack because it's LIFO.
        for(unsigned i = packed_path_size-1; i > 0; --i){
            recursion_stack.push(
                std::make_pair(packed_path[i-1], packed_path[i])
            );
        }

        std::pair<NodeID, NodeID> edge;
        while(!recursion_stack.empty()) {
            edge = recursion_stack.top();
            recursion_stack.pop();

            EdgeID smaller_edge_id = SPECIAL_EDGEID;
            int edge_weight = INT_MAX;
            for(
                EdgeID edge_id = facade->BeginEdges(edge.first);
                edge_id < facade->EndEdges(edge.first);
                ++edge_id
            ){
                const int weight = facade->GetEdgeData(edge_id).distance;
                if(
                    (facade->GetTarget(edge_id) == edge.second) &&
                    (weight < edge_weight)                      &&
                    facade->GetEdgeData(edge_id).forward
                ){
                    smaller_edge_id = edge_id;
                    edge_weight = weight;
                }
            }

            if( SPECIAL_EDGEID == smaller_edge_id ){
                for(
                    EdgeID edge_id = facade->BeginEdges(edge.second);
                    edge_id < facade->EndEdges(edge.second);
                    ++edge_id
                ){
                    const int weight = facade->GetEdgeData(edge_id).distance;
                    if(
                        (facade->GetTarget(edge_id) == edge.first) &&
                        (weight < edge_weight)              &&
                        facade->GetEdgeData(edge_id).backward
                    ){
                        smaller_edge_id = edge_id;
                        edge_weight = weight;
                    }
                }
            }
            BOOST_ASSERT_MSG(edge_weight != INT_MAX, "edge id invalid");

            const EdgeData& ed = facade->GetEdgeData(smaller_edge_id);
            if( ed.shortcut ) {//unpack
                const NodeID middle_node_id = ed.id;
                //again, we need to this in reversed order
                recursion_stack.push(std::make_pair(middle_node_id, edge.second));
                recursion_stack.push(std::make_pair(edge.first, middle_node_id));
            } else {
                BOOST_ASSERT_MSG(!ed.shortcut, "edge must be a shortcut");
                unpacked_path.push_back(
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

    inline void UnpackEdge(
        const NodeID s,
        const NodeID t,
        std::vector<NodeID> & unpacked_path
    ) const {
        std::stack<std::pair<NodeID, NodeID> > recursion_stack;
        recursion_stack.push(std::make_pair(s,t));

        std::pair<NodeID, NodeID> edge;
        while(!recursion_stack.empty()) {
            edge = recursion_stack.top();
            recursion_stack.pop();

            EdgeID smaller_edge_id = SPECIAL_EDGEID;
            int edge_weight = INT_MAX;
            for(
                EdgeID edge_id = facade->BeginEdges(edge.first);
                edge_id < facade->EndEdges(edge.first);
                ++edge_id
            ){
                const int weight = facade->GetEdgeData(edge_id).distance;
                if(
                    (facade->GetTarget(edge_id) == edge.second) &&
                    (weight < edge_weight)               &&
                    facade->GetEdgeData(edge_id).forward
                ){
                    smaller_edge_id = edge_id;
                    edge_weight = weight;
                }
            }

            if( SPECIAL_EDGEID == smaller_edge_id ){
                for(
                    EdgeID edge_id = facade->BeginEdges(edge.second);
                    edge_id < facade->EndEdges(edge.second);
                    ++edge_id
                ){
                    const int weight = facade->GetEdgeData(edge_id).distance;
                    if(
                        (facade->GetTarget(edge_id) == edge.first) &&
                        (weight < edge_weight)              &&
                        facade->GetEdgeData(edge_id).backward
                    ){
                        smaller_edge_id = edge_id;
                        edge_weight = weight;
                    }
                }
            }
            BOOST_ASSERT_MSG(edge_weight != INT_MAX, "edge weight invalid");

            const EdgeData& ed = facade->GetEdgeData(smaller_edge_id);
            if(ed.shortcut) {//unpack
                const NodeID middle_node_id = ed.id;
                //again, we need to this in reversed order
                recursion_stack.push(
                    std::make_pair(middle_node_id, edge.second)
                );
                recursion_stack.push(
                    std::make_pair(edge.first, middle_node_id)
                );
            } else {
                BOOST_ASSERT_MSG(!ed.shortcut, "edge must be shortcut");
                unpacked_path.push_back(edge.first );
            }
        }
        unpacked_path.push_back(t);
    }

    inline void RetrievePackedPathFromHeap(
        SearchEngineData::QueryHeap & forward_heap,
        SearchEngineData::QueryHeap & reverse_heap,
        const NodeID middle_node_id,
        std::vector<NodeID> & packed_path
    ) const {
        NodeID current_node_id = middle_node_id;
        while(current_node_id != forward_heap.GetData(current_node_id).parent) {
            current_node_id = forward_heap.GetData(current_node_id).parent;
            packed_path.push_back(current_node_id);
        }

        std::reverse(packed_path.begin(), packed_path.end());
        packed_path.push_back(middle_node_id);
        current_node_id = middle_node_id;
        while (current_node_id != reverse_heap.GetData(current_node_id).parent){
            current_node_id = reverse_heap.GetData(current_node_id).parent;
            packed_path.push_back(current_node_id);
    	}
    }

//TODO: reorder parameters
    inline void RetrievePackedPathFromSingleHeap(
        SearchEngineData::QueryHeap & search_heap,
        const NodeID middle_node_id,
        std::vector<NodeID>& packed_path
    ) const {
        NodeID current_node_id = middle_node_id;
        while(current_node_id != search_heap.GetData(current_node_id).parent) {
            current_node_id = search_heap.GetData(current_node_id).parent;
            packed_path.push_back(current_node_id);
        }
    }

    int ComputeEdgeOffset(const PhantomNode & phantom) const {
        return phantom.weight1 + (phantom.isBidirected() ? phantom.weight2 : 0);
    }

};

#endif /* BASICROUTINGINTERFACE_H_ */
