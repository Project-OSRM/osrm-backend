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

#ifndef ALTERNATIVEROUTES_H_
#define ALTERNATIVEROUTES_H_

#include "BasicRoutingInterface.h"
#include "../DataStructures/SearchEngineData.h"

#include <boost/unordered_map.hpp>
#include <cmath>
#include <vector>

const double VIAPATH_ALPHA   = 0.15;
const double VIAPATH_EPSILON = 0.10; //alternative at most 15% longer
const double VIAPATH_GAMMA   = 0.75; //alternative shares at most 75% with the shortest.

template<class DataFacadeT>
class AlternativeRouting : private BasicRoutingInterface<DataFacadeT> {
    typedef BasicRoutingInterface<DataFacadeT> super;
    typedef typename DataFacadeT::EdgeData EdgeData;
    typedef SearchEngineData::QueryHeap QueryHeap;
    typedef std::pair<NodeID, NodeID> SearchSpaceEdge;

    struct RankedCandidateNode {
        RankedCandidateNode(const NodeID n, const int l, const int s) :
            node(n),
            length(l),
            sharing(s)
        { }

        NodeID node;
        int length;
        int sharing;
        bool operator<(const RankedCandidateNode& other) const {
            return (2*length + sharing) < (2*other.length + other.sharing);
        }
    };
    DataFacadeT * facade;
    SearchEngineData & engine_working_data;
public:

    AlternativeRouting(
        DataFacadeT * facade,
        SearchEngineData & engine_working_data
    ) :
        super(facade),
        facade(facade),
        engine_working_data(engine_working_data)
    { }

    ~AlternativeRouting() {}

    void operator()(
        const PhantomNodes & phantom_node_pair,
        RawRouteData & raw_route_data) {
        if( (!phantom_node_pair.AtLeastOnePhantomNodeIsUINTMAX()) ||
              phantom_node_pair.PhantomNodesHaveEqualLocation()
        ) {
            raw_route_data.lengthOfShortestPath = INT_MAX;
            raw_route_data.lengthOfAlternativePath = INT_MAX;
            return;
        }

        std::vector<NodeID>          alternative_path;
        std::vector<NodeID>          via_node_candidate_list;
        std::vector<SearchSpaceEdge> forward_search_space;
        std::vector<SearchSpaceEdge> reverse_search_space;

        //Init queues, semi-expensive because access to TSS invokes a sys-call
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

        int upper_bound_to_shortest_path_distance = INT_MAX;
        NodeID middle_node = UINT_MAX;
        forward_heap1.Insert(
            phantom_node_pair.startPhantom.edgeBasedNode,
            -phantom_node_pair.startPhantom.weight1,
            phantom_node_pair.startPhantom.edgeBasedNode
        );
        if(phantom_node_pair.startPhantom.isBidirected() ) {
            forward_heap1.Insert(
                phantom_node_pair.startPhantom.edgeBasedNode+1,
                -phantom_node_pair.startPhantom.weight2,
                phantom_node_pair.startPhantom.edgeBasedNode+1
            );
        }

        reverse_heap1.Insert(
            phantom_node_pair.targetPhantom.edgeBasedNode,
            phantom_node_pair.targetPhantom.weight1,
            phantom_node_pair.targetPhantom.edgeBasedNode
        );
        if(phantom_node_pair.targetPhantom.isBidirected() ) {
        	reverse_heap1.Insert(
                phantom_node_pair.targetPhantom.edgeBasedNode+1,
                phantom_node_pair.targetPhantom.weight2,
                phantom_node_pair.targetPhantom.edgeBasedNode+1
            );
        }

        const int forward_offset =  super::ComputeEdgeOffset(
                                        phantom_node_pair.startPhantom
                                    );
        const int reverse_offset =  super::ComputeEdgeOffset(
                                        phantom_node_pair.targetPhantom
                                    );

        //search from s and t till new_min/(1+epsilon) > length_of_shortest_path
        while(0 < (forward_heap1.Size() + reverse_heap1.Size())){
            if(0 < forward_heap1.Size()){
                AlternativeRoutingStep<true>(
                    forward_heap1,
                    reverse_heap1,
                    &middle_node,
                    &upper_bound_to_shortest_path_distance,
                    via_node_candidate_list,
                    forward_search_space,
                    forward_offset
                );
            }
            if(0 < reverse_heap1.Size()){
                AlternativeRoutingStep<false>(
                    reverse_heap1,
                    forward_heap1,
                    &middle_node,
                    &upper_bound_to_shortest_path_distance,
                    via_node_candidate_list,
                    reverse_search_space,
                    reverse_offset
                );
            }
        }
        sort_unique_resize(via_node_candidate_list);

        std::vector<NodeID> packed_forward_path;
        std::vector<NodeID> packed_reverse_path;

        super::RetrievePackedPathFromSingleHeap(
            forward_heap1,
            middle_node,
            packed_forward_path
        );
        super::RetrievePackedPathFromSingleHeap(
            reverse_heap1,
            middle_node,
            packed_reverse_path
        );

        boost::unordered_map<NodeID, int> approximated_forward_sharing;
        boost::unordered_map<NodeID, int> approximated_reverse_sharing;

        unsigned index_into_forward_path = 0;
        //sweep over search space, compute forward sharing for each current edge (u,v)
        BOOST_FOREACH(const SearchSpaceEdge & current_edge, forward_search_space) {
        	const NodeID u = current_edge.first;
        	const NodeID v = current_edge.second;
        	if(packed_forward_path.size() < index_into_forward_path && current_edge == forward_search_space[index_into_forward_path]) {
        		//current_edge is on shortest path => sharing(u):=queue.GetKey(u);
        		++index_into_forward_path;
        		approximated_forward_sharing[v] = forward_heap1.GetKey(u);
        	} else {
        		//sharing (s) = sharing (t)
        		approximated_forward_sharing[v] = approximated_forward_sharing[u];
        	}
        }

        unsigned index_into_reverse_path = 0;
        //sweep over search space, compute backward sharing
        BOOST_FOREACH(const SearchSpaceEdge & current_edge, reverse_search_space) {
        	const NodeID u = current_edge.first;
        	const NodeID v = current_edge.second;
        	if(packed_reverse_path.size() < index_into_reverse_path && current_edge == reverse_search_space[index_into_reverse_path]) {
        		//current_edge is on shortest path => sharing(u):=queue.GetKey(u);
        		++index_into_reverse_path;
        		approximated_reverse_sharing[v] = reverse_heap1.GetKey(u);
        	} else {
        		//sharing (s) = sharing (t)
        		approximated_reverse_sharing[v] = approximated_reverse_sharing[u];
        	}
        }
        std::vector<NodeID> nodes_that_passed_preselection;
        BOOST_FOREACH(const NodeID node, via_node_candidate_list) {
            int approximated_sharing = approximated_forward_sharing[node] + approximated_reverse_sharing[node];
            int approximated_length = forward_heap1.GetKey(node)+reverse_heap1.GetKey(node);
            bool lengthPassed = (approximated_length < upper_bound_to_shortest_path_distance*(1+VIAPATH_EPSILON));
            bool sharingPassed = (approximated_sharing <= upper_bound_to_shortest_path_distance*VIAPATH_GAMMA);
            bool stretchPassed = approximated_length - approximated_sharing < (1.+VIAPATH_EPSILON)*(upper_bound_to_shortest_path_distance-approximated_sharing);

            if(lengthPassed && sharingPassed && stretchPassed) {
                nodes_that_passed_preselection.push_back(node);
            }
        }

        std::vector<NodeID> & packedShortestPath = packed_forward_path;
        std::reverse(packedShortestPath.begin(), packedShortestPath.end());
        packedShortestPath.push_back(middle_node);
        packedShortestPath.insert(packedShortestPath.end(),packed_reverse_path.begin(), packed_reverse_path.end());
        std::vector<RankedCandidateNode > rankedCandidates;

        //prioritizing via nodes for deep inspection
        BOOST_FOREACH(const NodeID node, nodes_that_passed_preselection) {
            int lengthOfViaPath = 0, sharingOfViaPath = 0;
            computeLengthAndSharingOfViaPath(node, &lengthOfViaPath, &sharingOfViaPath, forward_offset+reverse_offset, packedShortestPath);
            if(sharingOfViaPath <= upper_bound_to_shortest_path_distance*VIAPATH_GAMMA) {
                rankedCandidates.push_back(RankedCandidateNode(node, lengthOfViaPath, sharingOfViaPath));
            }
        }
        std::sort(rankedCandidates.begin(), rankedCandidates.end());

        NodeID selectedViaNode = UINT_MAX;
        int lengthOfViaPath = INT_MAX;
        NodeID s_v_middle = UINT_MAX, v_t_middle = UINT_MAX;
        BOOST_FOREACH(const RankedCandidateNode & candidate, rankedCandidates){
            if(viaNodeCandidatePasses_T_Test(forward_heap1, reverse_heap1, forward_heap2, reverse_heap2, candidate, forward_offset+reverse_offset, upper_bound_to_shortest_path_distance, &lengthOfViaPath, &s_v_middle, &v_t_middle)) {
                // select first admissable
                selectedViaNode = candidate.node;
                break;
            }
        }

        //Unpack shortest path and alternative, if they exist
        if(INT_MAX != upper_bound_to_shortest_path_distance) {
            super::UnpackPath(packedShortestPath, raw_route_data.computedShortestPath);
            raw_route_data.lengthOfShortestPath = upper_bound_to_shortest_path_distance;
        } else {
            raw_route_data.lengthOfShortestPath = INT_MAX;
        }

        if(selectedViaNode != UINT_MAX) {
            retrievePackedViaPath(forward_heap1, reverse_heap1, forward_heap2, reverse_heap2, s_v_middle, v_t_middle, raw_route_data.computedAlternativePath);
            raw_route_data.lengthOfAlternativePath = lengthOfViaPath;
        } else {
            raw_route_data.lengthOfAlternativePath = INT_MAX;
        }
    }

private:
    //unpack <s,..,v,..,t> by exploring search spaces from v
    inline void retrievePackedViaPath(QueryHeap & _forwardHeap1, QueryHeap & _backwardHeap1, QueryHeap & _forwardHeap2, QueryHeap & _backwardHeap2,
            const NodeID s_v_middle, const NodeID v_t_middle, std::vector<_PathData> & unpackedPath) {
        //unpack [s,v)
        std::vector<NodeID> packed_s_v_path, packed_v_t_path;
        super::RetrievePackedPathFromHeap(_forwardHeap1, _backwardHeap2, s_v_middle, packed_s_v_path);
        packed_s_v_path.resize(packed_s_v_path.size()-1);
        //unpack [v,t]
        super::RetrievePackedPathFromHeap(_forwardHeap2, _backwardHeap1, v_t_middle, packed_v_t_path);
        packed_s_v_path.insert(packed_s_v_path.end(),packed_v_t_path.begin(), packed_v_t_path.end() );
        super::UnpackPath(packed_s_v_path, unpackedPath);
    }

    inline void computeLengthAndSharingOfViaPath(const NodeID via_node, int *real_length_of_via_path, int *sharing_of_via_path,
            const int offset, const std::vector<NodeID> & packed_shortest_path) {
        //compute and unpack <s,..,v> and <v,..,t> by exploring search spaces from v and intersecting against queues
        //only half-searches have to be done at this stage
        engine_working_data.InitializeOrClearSecondThreadLocalStorage(
            super::facade->GetNumberOfNodes()
        );

        QueryHeap & existingForwardHeap  = *engine_working_data.forwardHeap;
        QueryHeap & existingBackwardHeap = *engine_working_data.backwardHeap;
        QueryHeap & newForwardHeap       = *engine_working_data.forwardHeap2;
        QueryHeap & newBackwardHeap      = *engine_working_data.backwardHeap2;

        std::vector < NodeID > packed_s_v_path;
        std::vector < NodeID > packed_v_t_path;

        std::vector<NodeID> partiallyUnpackedShortestPath;
        std::vector<NodeID> partiallyUnpackedViaPath;

        NodeID s_v_middle = UINT_MAX;
        int upperBoundFor_s_v_Path = INT_MAX;//compute path <s,..,v> by reusing forward search from s
        newBackwardHeap.Insert(via_node, 0, via_node);
        while (0 < newBackwardHeap.Size()) {
            super::RoutingStep(newBackwardHeap, existingForwardHeap, &s_v_middle, &upperBoundFor_s_v_Path, 2 * offset, false);
        }
        //compute path <v,..,t> by reusing backward search from node t
        NodeID v_t_middle = UINT_MAX;
        int upperBoundFor_v_t_Path = INT_MAX;
        newForwardHeap.Insert(via_node, 0, via_node);
        while (0 < newForwardHeap.Size() ) {
            super::RoutingStep(newForwardHeap, existingBackwardHeap, &v_t_middle, &upperBoundFor_v_t_Path, 2 * offset, true);
        }
        *real_length_of_via_path = upperBoundFor_s_v_Path + upperBoundFor_v_t_Path;

        if(UINT_MAX == s_v_middle || UINT_MAX == v_t_middle)
            return;

        //retrieve packed paths
        super::RetrievePackedPathFromHeap(existingForwardHeap, newBackwardHeap, s_v_middle, packed_s_v_path);
        super::RetrievePackedPathFromHeap(newForwardHeap, existingBackwardHeap, v_t_middle, packed_v_t_path);

        //partial unpacking, compute sharing
        //First partially unpack s-->v until paths deviate, note length of common path.
        for (unsigned i = 0, lengthOfPackedPath = std::min( packed_s_v_path.size(), packed_shortest_path.size()) - 1; (i < lengthOfPackedPath); ++i) {
            if (packed_s_v_path[i] == packed_shortest_path[i] && packed_s_v_path[i + 1] == packed_shortest_path[i + 1]) {
                EdgeID edgeID = facade->FindEdgeInEitherDirection(packed_s_v_path[i], packed_s_v_path[i + 1]);
                *sharing_of_via_path += facade->GetEdgeData(edgeID).distance;
            } else {
                if (packed_s_v_path[i] == packed_shortest_path[i]) {
                    super::UnpackEdge(packed_s_v_path[i], packed_s_v_path[i+1], partiallyUnpackedViaPath);
                    super::UnpackEdge(packed_shortest_path[i], packed_shortest_path[i+1], partiallyUnpackedShortestPath);
                    break;
                }
            }
        }
        //traverse partially unpacked edge and note common prefix
        for (int i = 0, lengthOfPackedPath = std::min( partiallyUnpackedViaPath.size(), partiallyUnpackedShortestPath.size()) - 1; (i < lengthOfPackedPath) && (partiallyUnpackedViaPath[i] == partiallyUnpackedShortestPath[i] && partiallyUnpackedViaPath[i+1] == partiallyUnpackedShortestPath[i+1]); ++i) {
            EdgeID edgeID = facade->FindEdgeInEitherDirection(partiallyUnpackedViaPath[i], partiallyUnpackedViaPath[i+1]);
            *sharing_of_via_path += facade->GetEdgeData(edgeID).distance;
        }

        //Second, partially unpack v-->t in reverse order until paths deviate and note lengths
        int viaPathIndex = packed_v_t_path.size() - 1;
        int shortestPathIndex = packed_shortest_path.size() - 1;
        for (; viaPathIndex > 0 && shortestPathIndex > 0; --viaPathIndex,--shortestPathIndex ) {
            if (packed_v_t_path[viaPathIndex - 1] == packed_shortest_path[shortestPathIndex - 1] && packed_v_t_path[viaPathIndex] == packed_shortest_path[shortestPathIndex]) {
                EdgeID edgeID = facade->FindEdgeInEitherDirection( packed_v_t_path[viaPathIndex - 1], packed_v_t_path[viaPathIndex]);
                *sharing_of_via_path += facade->GetEdgeData(edgeID).distance;
            } else {
                if (packed_v_t_path[viaPathIndex] == packed_shortest_path[shortestPathIndex]) {
                    super::UnpackEdge(packed_v_t_path[viaPathIndex-1], packed_v_t_path[viaPathIndex], partiallyUnpackedViaPath);
                    super::UnpackEdge(packed_shortest_path[shortestPathIndex-1] , packed_shortest_path[shortestPathIndex], partiallyUnpackedShortestPath);
                    break;
                }
            }
        }

        viaPathIndex = partiallyUnpackedViaPath.size() - 1;
        shortestPathIndex = partiallyUnpackedShortestPath.size() - 1;
        for (; viaPathIndex > 0 && shortestPathIndex > 0; --viaPathIndex,--shortestPathIndex) {
            if (partiallyUnpackedViaPath[viaPathIndex - 1] == partiallyUnpackedShortestPath[shortestPathIndex - 1] && partiallyUnpackedViaPath[viaPathIndex] == partiallyUnpackedShortestPath[shortestPathIndex]) {
                EdgeID edgeID = facade->FindEdgeInEitherDirection( partiallyUnpackedViaPath[viaPathIndex - 1], partiallyUnpackedViaPath[viaPathIndex]);
                *sharing_of_via_path += facade->GetEdgeData(edgeID).distance;
            } else {
                break;
            }
        }
        //finished partial unpacking spree! Amount of sharing is stored to appropriate pointer variable
    }

    inline int approximateAmountOfSharing(const NodeID middleNodeIDOfAlternativePath, QueryHeap & _forwardHeap, QueryHeap & _backwardHeap, const std::vector<NodeID> & packedShortestPath) {
        std::vector<NodeID> packedAlternativePath;
        super::RetrievePackedPathFromHeap(_forwardHeap, _backwardHeap, middleNodeIDOfAlternativePath, packedAlternativePath);

        if(packedShortestPath.size() < 2 || packedAlternativePath.size() < 2)
            return 0;

        int sharing = 0;
        int aindex = 0;
        //compute forward sharing
        while( (packedAlternativePath[aindex] == packedShortestPath[aindex]) && (packedAlternativePath[aindex+1] == packedShortestPath[aindex+1]) ) {
            //            SimpleLogger().Write() << "retrieving edge (" << packedAlternativePath[aindex] << "," << packedAlternativePath[aindex+1] << ")";
            EdgeID edgeID = facade->FindEdgeInEitherDirection(packedAlternativePath[aindex], packedAlternativePath[aindex+1]);
            sharing += facade->GetEdgeData(edgeID).distance;
            ++aindex;
        }

        aindex = packedAlternativePath.size()-1;
        int bindex = packedShortestPath.size()-1;
        //compute backward sharing
        while( aindex > 0 && bindex > 0 && (packedAlternativePath[aindex] == packedShortestPath[bindex]) && (packedAlternativePath[aindex-1] == packedShortestPath[bindex-1]) ) {
            EdgeID edgeID = facade->FindEdgeInEitherDirection(packedAlternativePath[aindex], packedAlternativePath[aindex-1]);
            sharing += facade->GetEdgeData(edgeID).distance;
            --aindex; --bindex;
        }
        return sharing;
    }

    template<bool forwardDirection>
    inline void AlternativeRoutingStep(
    		QueryHeap & _forward_heap,
    		QueryHeap & _reverse_heap,
    		NodeID *middle_node,
    		int *upper_bound_to_shortest_path_distance,
    		std::vector<NodeID>& searchSpaceIntersection,
    		std::vector<SearchSpaceEdge> & search_space,
    		const int edgeBasedOffset
    		) const {
        const NodeID node = _forward_heap.DeleteMin();
        const int distance = _forward_heap.GetKey(node);
        int scaledDistance = (distance-edgeBasedOffset)/(1.+VIAPATH_EPSILON);
        if(scaledDistance > *upper_bound_to_shortest_path_distance){
            _forward_heap.DeleteAll();
            return;
        }

        search_space.push_back(std::make_pair(_forward_heap.GetData( node ).parent, node));

        if(_reverse_heap.WasInserted(node) ){
            searchSpaceIntersection.push_back(node);

            const int newDistance = _reverse_heap.GetKey(node) + distance;
            if(newDistance < *upper_bound_to_shortest_path_distance ){
                if(newDistance>=0 ) {
                    *middle_node = node;
                    *upper_bound_to_shortest_path_distance = newDistance;
                }
            }
        }

        for ( EdgeID edge = facade->BeginEdges( node ); edge < facade->EndEdges(node); edge++ ) {
            const EdgeData & data = facade->GetEdgeData(edge);
            bool forwardDirectionFlag = (forwardDirection ? data.forward : data.backward );
            if(forwardDirectionFlag) {

                const NodeID to = facade->GetTarget(edge);
                const int edgeWeight = data.distance;

                assert( edgeWeight > 0 );
                const int toDistance = distance + edgeWeight;

                //New Node discovered -> Add to Heap + Node Info Storage
                if ( !_forward_heap.WasInserted( to ) ) {
                    _forward_heap.Insert( to, toDistance, node );

                }
                //Found a shorter Path -> Update distance
                else if ( toDistance < _forward_heap.GetKey( to ) ) {
                    _forward_heap.GetData( to ).parent = node;
                    _forward_heap.DecreaseKey( to, toDistance );
                    //new parent
                }
            }
        }
    }

    //conduct T-Test
    inline bool viaNodeCandidatePasses_T_Test( QueryHeap& existingForwardHeap, QueryHeap& existingBackwardHeap, QueryHeap& newForwardHeap, QueryHeap& newBackwardHeap, const RankedCandidateNode& candidate, const int offset, const int lengthOfShortestPath, int * lengthOfViaPath, NodeID * s_v_middle, NodeID * v_t_middle) {
    	newForwardHeap.Clear();
    	newBackwardHeap.Clear();
        std::vector < NodeID > packed_s_v_path;
        std::vector < NodeID > packed_v_t_path;

        *s_v_middle = UINT_MAX;
        int upperBoundFor_s_v_Path = INT_MAX;
        //compute path <s,..,v> by reusing forward search from s
        newBackwardHeap.Insert(candidate.node, 0, candidate.node);
        while (newBackwardHeap.Size() > 0) {
            super::RoutingStep(newBackwardHeap, existingForwardHeap, s_v_middle, &upperBoundFor_s_v_Path, 2*offset, false);
        }

        if(INT_MAX == upperBoundFor_s_v_Path)
            return false;

        //compute path <v,..,t> by reusing backward search from t
        *v_t_middle = UINT_MAX;
        int upperBoundFor_v_t_Path = INT_MAX;
        newForwardHeap.Insert(candidate.node, 0, candidate.node);
        while (newForwardHeap.Size() > 0) {
            super::RoutingStep(newForwardHeap, existingBackwardHeap, v_t_middle, &upperBoundFor_v_t_Path, 2*offset, true);
        }

        if(INT_MAX == upperBoundFor_v_t_Path)
            return false;

        *lengthOfViaPath = upperBoundFor_s_v_Path + upperBoundFor_v_t_Path;

        //retrieve packed paths
        super::RetrievePackedPathFromHeap(existingForwardHeap, newBackwardHeap, *s_v_middle, packed_s_v_path);
        super::RetrievePackedPathFromHeap(newForwardHeap, existingBackwardHeap, *v_t_middle, packed_v_t_path);

        NodeID s_P = *s_v_middle, t_P = *v_t_middle;
        if(UINT_MAX == s_P) {
            return false;
        }

        if(UINT_MAX == t_P) {
            return false;
        }
        const int T_threshold = VIAPATH_EPSILON * lengthOfShortestPath;
        int unpackedUntilDistance = 0;

        std::stack<SearchSpaceEdge> unpackStack;
        //Traverse path s-->v
        for (unsigned i = packed_s_v_path.size() - 1; (i > 0) && unpackStack.empty(); --i) {
            EdgeID edgeID = facade->FindEdgeInEitherDirection( packed_s_v_path[i - 1], packed_s_v_path[i]);
            int lengthOfCurrentEdge = facade->GetEdgeData(edgeID).distance;
            if (lengthOfCurrentEdge + unpackedUntilDistance >= T_threshold) {
                unpackStack.push(std::make_pair(packed_s_v_path[i - 1], packed_s_v_path[i]));
            } else {
                unpackedUntilDistance += lengthOfCurrentEdge;
                s_P = packed_s_v_path[i - 1];
            }
        }

        while (!unpackStack.empty()) {
            const SearchSpaceEdge viaPathEdge = unpackStack.top();
            unpackStack.pop();
            EdgeID edgeIDInViaPath = facade->FindEdgeInEitherDirection(viaPathEdge.first, viaPathEdge.second);
            if(UINT_MAX == edgeIDInViaPath)
                return false;
            EdgeData currentEdgeData = facade->GetEdgeData(edgeIDInViaPath);
            bool IsViaEdgeShortCut = currentEdgeData.shortcut;
            if (IsViaEdgeShortCut) {
                const NodeID middleOfViaPath = currentEdgeData.id;
                EdgeID edgeIDOfSecondSegment = facade->FindEdgeInEitherDirection(middleOfViaPath, viaPathEdge.second);
                int lengthOfSecondSegment = facade->GetEdgeData(edgeIDOfSecondSegment).distance;
                //attention: !unpacking in reverse!
                //Check if second segment is the one to go over treshold? if yes add second segment to stack, else push first segment to stack and add distance of second one.
                if (unpackedUntilDistance + lengthOfSecondSegment >= T_threshold) {
                    unpackStack.push(std::make_pair(middleOfViaPath, viaPathEdge.second));
                } else {
                    unpackedUntilDistance += lengthOfSecondSegment;
                    unpackStack.push(std::make_pair(viaPathEdge.first, middleOfViaPath));
                }
            } else {
                // edge is not a shortcut, set the start node for T-Test to end of edge.
                unpackedUntilDistance += currentEdgeData.distance;
                s_P = viaPathEdge.first;
            }
        }

        int lengthOfPathT_Test_Path = unpackedUntilDistance;
        unpackedUntilDistance = 0;
        //Traverse path s-->v
        for (unsigned i = 0, lengthOfPackedPath = packed_v_t_path.size() - 1; (i < lengthOfPackedPath) && unpackStack.empty(); ++i) {
            EdgeID edgeID = facade->FindEdgeInEitherDirection( packed_v_t_path[i], packed_v_t_path[i + 1]);
            int lengthOfCurrentEdge = facade->GetEdgeData(edgeID).distance;
            if (lengthOfCurrentEdge + unpackedUntilDistance >= T_threshold) {
                unpackStack.push( std::make_pair(packed_v_t_path[i], packed_v_t_path[i + 1]));
            } else {
                unpackedUntilDistance += lengthOfCurrentEdge;
                t_P = packed_v_t_path[i + 1];
            }
        }

        while (!unpackStack.empty()) {
            const SearchSpaceEdge viaPathEdge = unpackStack.top();
            unpackStack.pop();
            EdgeID edgeIDInViaPath = facade->FindEdgeInEitherDirection(viaPathEdge.first, viaPathEdge.second);
            if(UINT_MAX == edgeIDInViaPath)
                return false;
            EdgeData currentEdgeData = facade->GetEdgeData(edgeIDInViaPath);
            const bool IsViaEdgeShortCut = currentEdgeData.shortcut;
            if (IsViaEdgeShortCut) {
                const NodeID middleOfViaPath = currentEdgeData.id;
                EdgeID edgeIDOfFirstSegment = facade->FindEdgeInEitherDirection(viaPathEdge.first, middleOfViaPath);
                int lengthOfFirstSegment = facade->GetEdgeData( edgeIDOfFirstSegment).distance;
                //Check if first segment is the one to go over treshold? if yes first segment to stack, else push second segment to stack and add distance of first one.
                if (unpackedUntilDistance + lengthOfFirstSegment >= T_threshold) {
                    unpackStack.push( std::make_pair(viaPathEdge.first, middleOfViaPath));
                } else {
                    unpackedUntilDistance += lengthOfFirstSegment;
                    unpackStack.push( std::make_pair(middleOfViaPath, viaPathEdge.second));
                }
            } else {
                // edge is not a shortcut, set the start node for T-Test to end of edge.
                unpackedUntilDistance += currentEdgeData.distance;
                t_P = viaPathEdge.second;
            }
        }

        lengthOfPathT_Test_Path += unpackedUntilDistance;
        //Run actual T-Test query and compare if distances equal.
        engine_working_data.InitializeOrClearThirdThreadLocalStorage(
            super::facade->GetNumberOfNodes()
        );

        QueryHeap& forward_heap3 = *engine_working_data.forwardHeap3;
        QueryHeap& backward_heap3 = *engine_working_data.backwardHeap3;
        int _upperBound = INT_MAX;
        NodeID middle = UINT_MAX;
        forward_heap3.Insert(s_P, 0, s_P);
        backward_heap3.Insert(t_P, 0, t_P);
        //exploration from s and t until deletemin/(1+epsilon) > _lengthOfShortestPath
        while (forward_heap3.Size() + backward_heap3.Size() > 0) {
            if (forward_heap3.Size() > 0) {
                super::RoutingStep(forward_heap3, backward_heap3, &middle, &_upperBound, offset, true);
            }
            if (backward_heap3.Size() > 0) {
                super::RoutingStep(backward_heap3, forward_heap3, &middle, &_upperBound, offset, false);
            }
        }
        return (_upperBound <= lengthOfPathT_Test_Path);
    }
};

#endif /* ALTERNATIVEROUTES_H_ */
