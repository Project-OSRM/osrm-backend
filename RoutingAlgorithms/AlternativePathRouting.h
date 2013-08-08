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

#ifndef ALTERNATIVEROUTES_H_
#define ALTERNATIVEROUTES_H_

#include "BasicRoutingInterface.h"
#include <boost/unordered_map.hpp>
#include <cmath>
#include <vector>

const double VIAPATH_ALPHA   = 0.15;
const double VIAPATH_EPSILON = 0.10; //alternative at most 15% longer
const double VIAPATH_GAMMA   = 0.75; //alternative shares at most 75% with the shortest.

template<class QueryDataT>
class AlternativeRouting : private BasicRoutingInterface<QueryDataT> {
    typedef BasicRoutingInterface<QueryDataT> super;
    typedef typename QueryDataT::Graph SearchGraph;
    typedef typename QueryDataT::QueryHeap QueryHeap;
    typedef std::pair<NodeID, NodeID> SearchSpaceEdge;

    struct RankedCandidateNode {
        RankedCandidateNode(const NodeID n, const int l, const int s) : node(n), length(l), sharing(s) {}
        NodeID node;
        int length;
        int sharing;
        bool operator<(const RankedCandidateNode& other) const {
            return (2*length + sharing) < (2*other.length + other.sharing);
        }
    };

    const SearchGraph * search_graph;

public:

    AlternativeRouting(QueryDataT & qd) : super(qd), search_graph(qd.graph) { }

    ~AlternativeRouting() {}

    void operator()(const PhantomNodes & phantomNodePair, RawRouteData & rawRouteData) {
        if(!phantomNodePair.AtLeastOnePhantomNodeIsUINTMAX() || phantomNodePair.PhantomNodesHaveEqualLocation()) {
            rawRouteData.lengthOfShortestPath = rawRouteData.lengthOfAlternativePath = INT_MAX;
            return;
        }

        std::vector<NodeID> alternativePath;
        std::vector<NodeID> viaNodeCandidates;
        std::vector<SearchSpaceEdge> forward_search_space;
        std::vector<SearchSpaceEdge> reverse_search_space;

        //Initialize Queues, semi-expensive because access to TSS invokes a system call
        super::_queryData.InitializeOrClearFirstThreadLocalStorage();
        super::_queryData.InitializeOrClearSecondThreadLocalStorage();
        super::_queryData.InitializeOrClearThirdThreadLocalStorage();

        QueryHeap & forward_heap1 = *(super::_queryData.forwardHeap);
        QueryHeap & reverse_heap1 = *(super::_queryData.backwardHeap);
        QueryHeap & forward_heap2 = *(super::_queryData.forwardHeap2);
        QueryHeap & reverse_heap2 = *(super::_queryData.backwardHeap2);

        int upper_bound_to_shortest_path_distance = INT_MAX;
        NodeID middle_node = UINT_MAX;
        forward_heap1.Insert(phantomNodePair.startPhantom.edgeBasedNode, -phantomNodePair.startPhantom.weight1, phantomNodePair.startPhantom.edgeBasedNode);
        if(phantomNodePair.startPhantom.isBidirected() ) {
            forward_heap1.Insert(phantomNodePair.startPhantom.edgeBasedNode+1, -phantomNodePair.startPhantom.weight2, phantomNodePair.startPhantom.edgeBasedNode+1);
        }
        reverse_heap1.Insert(phantomNodePair.targetPhantom.edgeBasedNode, phantomNodePair.targetPhantom.weight1, phantomNodePair.targetPhantom.edgeBasedNode);
        if(phantomNodePair.targetPhantom.isBidirected() ) {
        	reverse_heap1.Insert(phantomNodePair.targetPhantom.edgeBasedNode+1, phantomNodePair.targetPhantom.weight2, phantomNodePair.targetPhantom.edgeBasedNode+1);
        }

        const int forward_offset = phantomNodePair.startPhantom.weight1 + (phantomNodePair.startPhantom.isBidirected() ? phantomNodePair.startPhantom.weight2 : 0);
        const int reverse_offset = phantomNodePair.targetPhantom.weight1 + (phantomNodePair.targetPhantom.isBidirected() ? phantomNodePair.targetPhantom.weight2 : 0);

        //exploration dijkstra from nodes s and t until deletemin/(1+epsilon) > _lengthOfShortestPath
        while(0 < (forward_heap1.Size() + reverse_heap1.Size())){
            if(0 < forward_heap1.Size()){
                AlternativeRoutingStep<true >(forward_heap1, reverse_heap1, &middle_node, &upper_bound_to_shortest_path_distance, viaNodeCandidates, forward_search_space, forward_offset);
            }
            if(0 < reverse_heap1.Size()){
                AlternativeRoutingStep<false>(reverse_heap1, forward_heap1, &middle_node, &upper_bound_to_shortest_path_distance, viaNodeCandidates, reverse_search_space, reverse_offset);
            }
        }
        sort_unique_resize(viaNodeCandidates);

        std::vector<NodeID> packed_forward_path;
        std::vector<NodeID> packed_reverse_path;

        super::RetrievePackedPathFromSingleHeap(forward_heap1, middle_node, packed_forward_path);
        super::RetrievePackedPathFromSingleHeap(reverse_heap1, middle_node, packed_reverse_path);
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
        BOOST_FOREACH(const NodeID node, viaNodeCandidates) {
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
            super::UnpackPath(packedShortestPath, rawRouteData.computedShortestPath);
            rawRouteData.lengthOfShortestPath = upper_bound_to_shortest_path_distance;
        } else {
            rawRouteData.lengthOfShortestPath = INT_MAX;
        }

        if(selectedViaNode != UINT_MAX) {
            retrievePackedViaPath(forward_heap1, reverse_heap1, forward_heap2, reverse_heap2, s_v_middle, v_t_middle, rawRouteData.computedAlternativePath);
            rawRouteData.lengthOfAlternativePath = lengthOfViaPath;
        } else {
            rawRouteData.lengthOfAlternativePath = INT_MAX;
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
        super::_queryData.InitializeOrClearSecondThreadLocalStorage();

        QueryHeap & existingForwardHeap  = *super::_queryData.forwardHeap;
        QueryHeap & existingBackwardHeap = *super::_queryData.backwardHeap;
        QueryHeap & newForwardHeap       = *super::_queryData.forwardHeap2;
        QueryHeap & newBackwardHeap      = *super::_queryData.backwardHeap2;

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
                typename SearchGraph::EdgeIterator edgeID = search_graph->FindEdgeInEitherDirection(packed_s_v_path[i], packed_s_v_path[i + 1]);
                *sharing_of_via_path += search_graph->GetEdgeData(edgeID).distance;
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
            typename SearchGraph::EdgeIterator edgeID = search_graph->FindEdgeInEitherDirection(partiallyUnpackedViaPath[i], partiallyUnpackedViaPath[i+1]);
            *sharing_of_via_path += search_graph->GetEdgeData(edgeID).distance;
        }

        //Second, partially unpack v-->t in reverse order until paths deviate and note lengths
        int viaPathIndex = packed_v_t_path.size() - 1;
        int shortestPathIndex = packed_shortest_path.size() - 1;
        for (; viaPathIndex > 0 && shortestPathIndex > 0; --viaPathIndex,--shortestPathIndex ) {
            if (packed_v_t_path[viaPathIndex - 1] == packed_shortest_path[shortestPathIndex - 1] && packed_v_t_path[viaPathIndex] == packed_shortest_path[shortestPathIndex]) {
                typename SearchGraph::EdgeIterator edgeID = search_graph->FindEdgeInEitherDirection( packed_v_t_path[viaPathIndex - 1], packed_v_t_path[viaPathIndex]);
                *sharing_of_via_path += search_graph->GetEdgeData(edgeID).distance;
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
                typename SearchGraph::EdgeIterator edgeID = search_graph->FindEdgeInEitherDirection( partiallyUnpackedViaPath[viaPathIndex - 1], partiallyUnpackedViaPath[viaPathIndex]);
                *sharing_of_via_path += search_graph->GetEdgeData(edgeID).distance;
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
            typename SearchGraph::EdgeIterator edgeID = search_graph->FindEdgeInEitherDirection(packedAlternativePath[aindex], packedAlternativePath[aindex+1]);
            sharing += search_graph->GetEdgeData(edgeID).distance;
            ++aindex;
        }

        aindex = packedAlternativePath.size()-1;
        int bindex = packedShortestPath.size()-1;
        //compute backward sharing
        while( aindex > 0 && bindex > 0 && (packedAlternativePath[aindex] == packedShortestPath[bindex]) && (packedAlternativePath[aindex-1] == packedShortestPath[bindex-1]) ) {
            typename SearchGraph::EdgeIterator edgeID = search_graph->FindEdgeInEitherDirection(packedAlternativePath[aindex], packedAlternativePath[aindex-1]);
            sharing += search_graph->GetEdgeData(edgeID).distance;
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

        for ( typename SearchGraph::EdgeIterator edge = search_graph->BeginEdges( node ); edge < search_graph->EndEdges(node); edge++ ) {
            const typename SearchGraph::EdgeData & data = search_graph->GetEdgeData(edge);
            bool forwardDirectionFlag = (forwardDirection ? data.forward : data.backward );
            if(forwardDirectionFlag) {

                const NodeID to = search_graph->GetTarget(edge);
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
            typename SearchGraph::EdgeIterator edgeID = search_graph->FindEdgeInEitherDirection( packed_s_v_path[i - 1], packed_s_v_path[i]);
            int lengthOfCurrentEdge = search_graph->GetEdgeData(edgeID).distance;
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
            typename SearchGraph::EdgeIterator edgeIDInViaPath = search_graph->FindEdgeInEitherDirection(viaPathEdge.first, viaPathEdge.second);
            if(UINT_MAX == edgeIDInViaPath)
                return false;
            typename SearchGraph::EdgeData currentEdgeData = search_graph->GetEdgeData(edgeIDInViaPath);
            bool IsViaEdgeShortCut = currentEdgeData.shortcut;
            if (IsViaEdgeShortCut) {
                const NodeID middleOfViaPath = currentEdgeData.id;
                typename SearchGraph::EdgeIterator edgeIDOfSecondSegment = search_graph->FindEdgeInEitherDirection(middleOfViaPath, viaPathEdge.second);
                int lengthOfSecondSegment = search_graph->GetEdgeData(edgeIDOfSecondSegment).distance;
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
            typename SearchGraph::EdgeIterator edgeID = search_graph->FindEdgeInEitherDirection( packed_v_t_path[i], packed_v_t_path[i + 1]);
            int lengthOfCurrentEdge = search_graph->GetEdgeData(edgeID).distance;
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
            typename SearchGraph::EdgeIterator edgeIDInViaPath = search_graph->FindEdgeInEitherDirection(viaPathEdge.first, viaPathEdge.second);
            if(UINT_MAX == edgeIDInViaPath)
                return false;
            typename SearchGraph::EdgeData currentEdgeData = search_graph->GetEdgeData(edgeIDInViaPath);
            const bool IsViaEdgeShortCut = currentEdgeData.shortcut;
            if (IsViaEdgeShortCut) {
                const NodeID middleOfViaPath = currentEdgeData.id;
                typename SearchGraph::EdgeIterator edgeIDOfFirstSegment = search_graph->FindEdgeInEitherDirection(viaPathEdge.first, middleOfViaPath);
                int lengthOfFirstSegment = search_graph->GetEdgeData( edgeIDOfFirstSegment).distance;
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
        super::_queryData.InitializeOrClearThirdThreadLocalStorage();

        QueryHeap& forward_heap3 = *super::_queryData.forwardHeap3;
        QueryHeap& backward_heap3 = *super::_queryData.backwardHeap3;
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
