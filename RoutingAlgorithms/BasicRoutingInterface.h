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
#include "../DataStructures/TurnInstructions.h"
#include "../Util/ContainerUtils.h"
#include "../Util/SimpleLogger.h"

#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>

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
    explicit BasicRoutingInterface( DataFacadeT * facade ) : facade(facade) { }
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
        SimpleLogger().Write() << (forward_direction ? "fwd" : "rev") << " settled (" << forward_heap.GetData( node ).parent << "," << node << ")=" << distance;
        if(reverse_heap.WasInserted(node) ){
            const int new_distance = reverse_heap.GetKey(node) + distance;
            SimpleLogger().Write(logDEBUG) << "new_distance: " << new_distance;
            if(new_distance < *upper_bound ){
                if( new_distance >= 0 ) {
                    *middle_node_id = node;
                    *upper_bound = new_distance;
                }
            }
        }

        if( (distance-edge_expansion_offset) > *upper_bound ){
            SimpleLogger().Write() << "found path";
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
                        SimpleLogger().Write(logDEBUG) << "stalled";
                        return;
                    }
                }
            }
        }
        SimpleLogger().Write(logDEBUG) << "done stalling";
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
                    SimpleLogger().Write() << "insert (" << node << "," << to << "), distance: " << to_distance << ", edge id: " << edge;
                    forward_heap.Insert( to, to_distance, node );
                }
                //Found a shorter Path -> Update distance
                else if ( to_distance < forward_heap.GetKey( to ) ) {
                    forward_heap.GetData( to ).parent = node;
                    forward_heap.DecreaseKey( to, to_distance );
                    SimpleLogger().Write() << "decrease (" << node << "," << to << "), distance: " << to_distance;
                    //new parent
                }
            }
        }
    }


    //TODO: refactor parameters to only edge ids for start and end
    inline void UnpackPath(
        const std::vector<NodeID> & packed_path,
        const PhantomNodes & phantom_node_pair,
        std::vector<PathData> & unpacked_path
    ) const {
        SimpleLogger().Write(logDEBUG) << "packed_path.size: " << packed_path.size();
        const bool start_traversed_in_reverse = (packed_path.front() != phantom_node_pair.startPhantom.forward_node_id);
        const bool target_traversed_in_reverse = (packed_path.back() != phantom_node_pair.targetPhantom.forward_node_id);

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
            // bool segment_reversed = false;
            edge = recursion_stack.top();
            recursion_stack.pop();

            // facade->FindEdge does not suffice here in case of shortcuts.
            // The above explanation unclear? Think!
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
            BOOST_ASSERT_MSG(edge_weight != SPECIAL_EDGEID, "edge id invalid");


            // BOOST_ASSERT( facade->EndEdges(edge.first) != smaller_edge_id );

            const EdgeData& ed = facade->GetEdgeData(smaller_edge_id);
            if( ed.shortcut ) {//unpack
                const NodeID middle_node_id = ed.id;
                //again, we need to this in reversed order
                recursion_stack.push(std::make_pair(middle_node_id, edge.second));
                recursion_stack.push(std::make_pair(edge.first, middle_node_id));
            } else {
                BOOST_ASSERT_MSG(!ed.shortcut, "original edge flagged as shortcut");
                unsigned name_index = facade->GetNameIndexFromEdgeID(ed.id);
                const TurnInstruction turn_instruction = facade->GetTurnInstructionForEdgeID(ed.id);

                if ( !facade->EdgeIsCompressed(ed.id) ){
                    BOOST_ASSERT( !facade->EdgeIsCompressed(ed.id) );
                    unpacked_path.push_back(
                        PathData(
                            facade->GetGeometryIndexForEdgeID(ed.id),
                            name_index,
                            turn_instruction,
                            ed.distance
                        )
                    );
                } else {
                    std::vector<unsigned> id_vector;
                    facade->GetUncompressedGeometry(facade->GetGeometryIndexForEdgeID(ed.id), id_vector);

                    const int start_index = ( unpacked_path.empty() ? ( ( start_traversed_in_reverse ) ?  id_vector.size() - phantom_node_pair.startPhantom.fwd_segment_position - 1 : phantom_node_pair.startPhantom.fwd_segment_position ) : 0 );
                    const int end_index = id_vector.size();

                    BOOST_ASSERT( start_index >= 0 );
                    BOOST_ASSERT( start_index <= end_index );
                    for(
                        unsigned i = start_index;
                        i < end_index;
                        ++i
                    ) {
                        unpacked_path.push_back(
                            PathData(
                                id_vector[i],
                                name_index,
                                TurnInstructionsClass::NoTurn,
                                0
                            )
                        );
                    }
                    unpacked_path.back().turnInstruction = turn_instruction;
                    unpacked_path.back().durationOfSegment = ed.distance;
                }
            }
        }
        if(SPECIAL_EDGEID != phantom_node_pair.targetPhantom.packed_geometry_id ) {
            SimpleLogger().Write(logDEBUG) << "unpacking last segment " << phantom_node_pair.targetPhantom.packed_geometry_id;
            SimpleLogger().Write(logDEBUG) << "start_traversed_in_reverse: " << (start_traversed_in_reverse ? "y" : "n");
            SimpleLogger().Write(logDEBUG) << "target_traversed_in_reverse: " << (target_traversed_in_reverse ? "y" : "n");
            SimpleLogger().Write(logDEBUG) << "phantom_node_pair.startPhantom.fwd_segment_position: " << phantom_node_pair.startPhantom.fwd_segment_position << ", " <<
                                              "phantom_node_pair.targetPhantom.fwd_segment_position: " << phantom_node_pair.targetPhantom.fwd_segment_position;
            std::vector<unsigned> id_vector;
            facade->GetUncompressedGeometry(phantom_node_pair.targetPhantom.packed_geometry_id, id_vector);
            if( target_traversed_in_reverse ) {
                std::reverse(id_vector.begin(), id_vector.end() );
            }
            SimpleLogger().Write(logDEBUG) << "id_vector.size() " << id_vector.size();
            SimpleLogger().Write(logDEBUG) << "unpacked_path.empty()=" << (unpacked_path.empty() ? "y" : "n");

            const bool is_local_path = (phantom_node_pair.startPhantom.packed_geometry_id  == phantom_node_pair.targetPhantom.packed_geometry_id) && unpacked_path.empty();
            SimpleLogger().Write(logDEBUG) << "is_local_path: " << (is_local_path ? "y" : "n");


           //         const int start_index = ( unpacked_path.empty() ? ( ( start_traversed_in_reverse ) ?  id_vector.size() - phantom_node_pair.startPhantom.fwd_segment_position - 1 : phantom_node_pair.startPhantom.fwd_segment_position ) : 0 );

            int start_index = 0;
            int end_index = phantom_node_pair.targetPhantom.fwd_segment_position;
            SimpleLogger().Write(logDEBUG) << "case1";
            if (target_traversed_in_reverse)
            {
                // start_index = id_vector.size() -1;
                end_index = id_vector.size() - phantom_node_pair.targetPhantom.fwd_segment_position;
                // if (is_local_path)
                // {
                //     start_index = id_vector.size() - phantom_node_pair.startPhantom.fwd_segment_position -1;
                // }
            }
            if (is_local_path)
            {
                SimpleLogger().Write(logDEBUG) << "case3";
                start_index = phantom_node_pair.startPhantom.fwd_segment_position;
                end_index = phantom_node_pair.targetPhantom.fwd_segment_position;
                if (target_traversed_in_reverse)
                {
                    SimpleLogger().Write(logDEBUG) << "case4";
                    start_index = id_vector.size() - phantom_node_pair.startPhantom.fwd_segment_position - 1;
                    end_index = id_vector.size() - phantom_node_pair.targetPhantom.fwd_segment_position - 1;
                }
            }

            SimpleLogger().Write(logDEBUG) << "fetching from [" << start_index << "," << end_index << "]";

            BOOST_ASSERT( start_index >= 0 );
            // BOOST_ASSERT( start_index <= end_index );
            for(
                int i = start_index;
                i != end_index;
                ( start_index < end_index ? ++i :--i)
            ) {
                BOOST_ASSERT( i >= -1 );
                if ( i >= 0 )
                {
                    SimpleLogger().Write(logDEBUG) << "[" << i << "]" << facade->GetCoordinateOfNode(id_vector[i]);
                }
                unpacked_path.push_back(
                    PathData(
                        id_vector[i],
                        phantom_node_pair.targetPhantom.name_id,
                        TurnInstructionsClass::NoTurn,
                        0
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
        const SearchEngineData::QueryHeap & forward_heap,
        const SearchEngineData::QueryHeap & reverse_heap,
        const NodeID middle_node_id,
        std::vector<NodeID> & packed_path
    ) const {
        NodeID current_node_id = middle_node_id;
        while(current_node_id != forward_heap.GetData(current_node_id).parent) {
            current_node_id = forward_heap.GetData(current_node_id).parent;
            packed_path.push_back(current_node_id);
        }
        // SimpleLogger().Write() << "parent of last node. " << forward_heap.GetData(current_node_id).parent;
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

    int ComputeEdgeOffset(const PhantomNode & phantom_node) const {
        int weight_offset = 0;
        if (phantom_node.forward_node_id != SPECIAL_NODEID)
        {
            weight_offset += phantom_node.GetForwardWeightPlusOffset();
        }
        if (phantom_node.reverse_node_id != SPECIAL_NODEID)
        {
            weight_offset += phantom_node.GetReverseWeightPlusOffset();
        }
        return weight_offset;
        // return phantom_node.forward_weight + (phantom_node.isBidirected() ? phantom_node.reverse_weight : 0);
    }

};

#endif /* BASICROUTINGINTERFACE_H_ */
