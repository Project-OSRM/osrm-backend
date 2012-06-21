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

#include <cmath>

#include "BasicRoutingInterface.h"

const double VIAPATH_ALPHA   = 0.25;
const double VIAPATH_EPSILON = 0.25;
const double VIAPATH_GAMMA   = 0.80;

template<class QueryDataT>
class AlternativeRouting : private BasicRoutingInterface<QueryDataT>{
    typedef BasicRoutingInterface<QueryDataT> super;
    typedef std::pair<NodeID, int> PreselectedNode;
    typedef typename QueryDataT::HeapPtr HeapPtr;
    typedef std::pair<NodeID, NodeID> UnpackEdge;

    struct RankedCandidateNode {
        RankedCandidateNode(NodeID n, int l, int s) : node(n), length(l), sharing(s) {}
        NodeID node;
        int length;
        int sharing;
        const bool operator<(const RankedCandidateNode& other) const {
            return (2*length + sharing) < (2*other.length + other.sharing);
        }
    };
public:

    AlternativeRouting(QueryDataT & qd) : super(qd) { }

    ~AlternativeRouting() {}

    void operator()(const PhantomNodes & phantomNodePair, RawRouteData & rawRouteData) {
        if(!phantomNodePair.AtLeastOnePhantomNodeIsUINTMAX()) {
            rawRouteData.lengthOfShortestPath = rawRouteData.lengthOfAlternativePath = INT_MAX;
            return;
        }
        std::vector<NodeID> alternativePath;
        std::vector<NodeID> viaNodeCandidates;
        std::deque <NodeID> packedShortestPath;
        std::vector<PreselectedNode> nodesThatPassPreselection;

        HeapPtr & forwardHeap = super::_queryData.forwardHeap;
        HeapPtr & backwardHeap = super::_queryData.backwardHeap;
        HeapPtr & forwardHeap2 = super::_queryData.forwardHeap2;
        HeapPtr & backwardHeap2 = super::_queryData.backwardHeap2;

        //Initialize Queues
        super::_queryData.InitializeOrClearFirstThreadLocalStorage();
        int _upperBound = INT_MAX;
        NodeID middle = UINT_MAX;
        forwardHeap->Insert(phantomNodePair.startPhantom.edgeBasedNode, -phantomNodePair.startPhantom.weight1, phantomNodePair.startPhantom.edgeBasedNode);
        if(phantomNodePair.startPhantom.isBidirected() ) {
            forwardHeap->Insert(phantomNodePair.startPhantom.edgeBasedNode+1, -phantomNodePair.startPhantom.weight2, phantomNodePair.startPhantom.edgeBasedNode+1);
        }
        backwardHeap->Insert(phantomNodePair.targetPhantom.edgeBasedNode, phantomNodePair.targetPhantom.weight1, phantomNodePair.targetPhantom.edgeBasedNode);
        if(phantomNodePair.targetPhantom.isBidirected() ) {
            backwardHeap->Insert(phantomNodePair.targetPhantom.edgeBasedNode+1, phantomNodePair.targetPhantom.weight2, phantomNodePair.targetPhantom.edgeBasedNode+1);
        }

        const int offset = (phantomNodePair.startPhantom.isBidirected() ? std::max(phantomNodePair.startPhantom.weight1, phantomNodePair.startPhantom.weight2) : phantomNodePair.startPhantom.weight1)
            + (phantomNodePair.targetPhantom.isBidirected() ? std::max(phantomNodePair.targetPhantom.weight1, phantomNodePair.targetPhantom.weight2) : phantomNodePair.targetPhantom.weight1);

        //exploration dijkstra from nodes s and t until deletemin/(1+epsilon) > _lengthOfShortestPath
        while(forwardHeap->Size() + backwardHeap->Size() > 0){
            if(forwardHeap->Size() > 0){
                AlternativeRoutingStep(forwardHeap, backwardHeap, &middle, &_upperBound, 2*offset, true, viaNodeCandidates);
            }
            if(backwardHeap->Size() > 0){
                AlternativeRoutingStep(backwardHeap, forwardHeap, &middle, &_upperBound, 2*offset, false, viaNodeCandidates);
            }
        }
        std::sort(viaNodeCandidates.begin(), viaNodeCandidates.end());
        int size = std::unique(viaNodeCandidates.begin(), viaNodeCandidates.end())- viaNodeCandidates.begin();
        viaNodeCandidates.resize(size);

        //save (packed) shortest path of shortest path and keep it for later use.
        //we need it during the checks and dont want to recompute it always
        super::RetrievePackedPathFromHeap(forwardHeap, backwardHeap, middle, packedShortestPath);

        //ch-pruning of via nodes in both search spaces
        BOOST_FOREACH(const NodeID node, viaNodeCandidates) {
            if(node == middle) //subpath optimality tells us that this case is just the shortest path
                continue;

            int sharing = approximateAmountOfSharing(node, forwardHeap, backwardHeap, packedShortestPath);
            int length1 = forwardHeap->GetKey(node);
            int length2 = backwardHeap->GetKey(node);
            bool lengthPassed = (length1+length2 < _upperBound*(1+VIAPATH_EPSILON));
            bool sharingPassed = (sharing <= _upperBound*VIAPATH_GAMMA);
            bool stretchPassed = length1+length2 - sharing < (1.+VIAPATH_EPSILON)*(_upperBound-sharing);

            if(lengthPassed && sharingPassed && stretchPassed)
                nodesThatPassPreselection.push_back(std::make_pair(node, length1+length2));
        }

        std::vector<RankedCandidateNode > rankedCandidates;

        //prioritizing via nodes for deep inspection
        BOOST_FOREACH(const PreselectedNode node, nodesThatPassPreselection) {
            int lengthOfViaPath = 0, sharingOfViaPath = 0;

            computeLengthAndSharingOfViaPath(phantomNodePair, node, &lengthOfViaPath, &sharingOfViaPath, offset, packedShortestPath);
            if(sharingOfViaPath <= VIAPATH_GAMMA*_upperBound)
                rankedCandidates.push_back(RankedCandidateNode(node.first, lengthOfViaPath, sharingOfViaPath));
        }

        std::sort(rankedCandidates.begin(), rankedCandidates.end());

        NodeID selectedViaNode = UINT_MAX;
        int lengthOfViaPath = INT_MAX;
        NodeID s_v_middle, v_t_middle;
        BOOST_FOREACH(const RankedCandidateNode candidate, rankedCandidates){
            if(viaNodeCandidatePasses_T_Test(forwardHeap, backwardHeap, forwardHeap2, backwardHeap2, candidate, offset, _upperBound, &lengthOfViaPath, &s_v_middle, &v_t_middle)) {
                // select first admissable
                selectedViaNode = candidate.node;
                break;
            }
        }

        //Unpack shortest path and alternative, if they exist
        if(INT_MAX != _upperBound) {
            super::UnpackPath(packedShortestPath, rawRouteData.computedShortestPath);
            rawRouteData.lengthOfShortestPath = _upperBound;
        } else {
            rawRouteData.lengthOfShortestPath = INT_MAX;
        }

        if(selectedViaNode != UINT_MAX) {
            retrievePackedViaPath(forwardHeap, backwardHeap, forwardHeap2, backwardHeap2, s_v_middle, v_t_middle, rawRouteData.computedAlternativePath);
            rawRouteData.lengthOfAlternativePath = lengthOfViaPath;
        } else {
            rawRouteData.lengthOfAlternativePath = INT_MAX;
        }
    }

private:
    //unpack <s,..,v,..,t> by exploring search spaces from v
    inline void retrievePackedViaPath(const HeapPtr & _forwardHeap1, const HeapPtr & _backwardHeap1, const HeapPtr & _forwardHeap2, const HeapPtr & _backwardHeap2,
            const NodeID s_v_middle, const NodeID v_t_middle, std::vector<_PathData> & unpackedPath) {
        //unpack [s,v)
        std::deque<NodeID> packed_s_v_path, packed_v_t_path;
        super::RetrievePackedPathFromHeap(_forwardHeap1, _backwardHeap2, s_v_middle, packed_s_v_path);
        packed_s_v_path.resize(packed_s_v_path.size()-1);
        //unpack [v,t]
        super::RetrievePackedPathFromHeap(_forwardHeap2, _backwardHeap1, v_t_middle, packed_v_t_path);
        packed_s_v_path.insert(packed_s_v_path.end(),packed_v_t_path.begin(), packed_v_t_path.end() );
        super::UnpackPath(packed_s_v_path, unpackedPath);
    }

    inline void computeLengthAndSharingOfViaPath(const PhantomNodes & phantomNodePair, const PreselectedNode& node, int *lengthOfViaPath, int *sharingOfViaPath,
            const int offset, const std::deque<NodeID> & packedShortestPath) {
        //compute and unpack <s,..,v> and <v,..,t> by exploring search spaces from v and intersecting against queues
        //only half-searches have to be done at this stage
        super::_queryData.InitializeOrClearSecondThreadLocalStorage();

        HeapPtr & existingForwardHeap  = super::_queryData.forwardHeap;
        HeapPtr & existingBackwardHeap = super::_queryData.backwardHeap;
        HeapPtr & newForwardHeap       = super::_queryData.forwardHeap2;
        HeapPtr & newBackwardHeap      = super::_queryData.backwardHeap2;

        std::deque < NodeID > packed_s_v_path;
        std::deque < NodeID > packed_v_t_path;

        std::vector<NodeID> partiallyUnpackedShortestPath;
        std::vector<NodeID> partiallyUnpackedViaPath;

        NodeID s_v_middle = UINT_MAX;
        int upperBoundFor_s_v_Path = INT_MAX;//compute path <s,..,v> by reusing forward search from s
        newBackwardHeap->Insert(node.first, 0, node.first);
        while (newBackwardHeap->Size() > 0) {
            super::RoutingStep(newBackwardHeap, existingForwardHeap, &s_v_middle, &upperBoundFor_s_v_Path, 2 * offset, false);
        }
        //compute path <v,..,t> by reusing backward search from node t
        NodeID v_t_middle = UINT_MAX;
        int upperBoundFor_v_t_Path = INT_MAX;
        newForwardHeap->Insert(node.first, 0, node.first);
        while (newForwardHeap->Size() > 0) {
            super::RoutingStep(newForwardHeap, existingBackwardHeap, &v_t_middle, &upperBoundFor_v_t_Path, 2 * offset, true);
        }
        *lengthOfViaPath = upperBoundFor_s_v_Path + upperBoundFor_v_t_Path;

        //retrieve packed paths
        super::RetrievePackedPathFromHeap(existingForwardHeap, newBackwardHeap, s_v_middle, packed_s_v_path);
        super::RetrievePackedPathFromHeap(newForwardHeap, existingBackwardHeap, v_t_middle, packed_v_t_path);

        //partial unpacking, compute sharing
        //First partially unpack s-->v until paths deviate, note length of common path.
        for (unsigned i = 0, lengthOfPackedPath = std::min( packed_s_v_path.size(), packedShortestPath.size()) - 1; (i < lengthOfPackedPath); ++i) {
            if (packed_s_v_path[i] == packedShortestPath[i] && packed_s_v_path[i + 1] == packedShortestPath[i + 1]) {
                typename QueryDataT::Graph::EdgeIterator edgeID = super::_queryData.graph->FindEdgeInEitherDirection(packed_s_v_path[i], packed_s_v_path[i + 1]);
                *sharingOfViaPath += super::_queryData.graph->GetEdgeData(edgeID).distance;
            } else {
                if (packed_s_v_path[i] == packedShortestPath[i]) {
                    super::UnpackEdge(packed_s_v_path[i], packed_s_v_path[i+1], partiallyUnpackedViaPath);
                    super::UnpackEdge(packedShortestPath[i], packedShortestPath[i+1], partiallyUnpackedShortestPath);
                    break;
                }
            }
        }
        //traverse partially unpacked edge and note common prefix
        for (int i = 0, lengthOfPackedPath = std::min( partiallyUnpackedViaPath.size(), partiallyUnpackedShortestPath.size()) - 1; (i < lengthOfPackedPath) && (partiallyUnpackedViaPath[i] == partiallyUnpackedShortestPath[i] && partiallyUnpackedViaPath[i+1] == partiallyUnpackedShortestPath[i+1]); ++i) {
            typename QueryDataT::Graph::EdgeIterator edgeID = super::_queryData.graph->FindEdgeInEitherDirection(partiallyUnpackedViaPath[i], partiallyUnpackedViaPath[i+1]);
            *sharingOfViaPath += super::_queryData.graph->GetEdgeData(edgeID).distance;
        }

        //Second, partially unpack v-->t in reverse order until paths deviate and note lengths
        int viaPathIndex = packed_v_t_path.size() - 1;
        int shortestPathIndex = packedShortestPath.size() - 1;
        for (; viaPathIndex > 0 && shortestPathIndex > 0; --viaPathIndex,--shortestPathIndex ) {
            if (packed_v_t_path[viaPathIndex - 1] == packedShortestPath[shortestPathIndex - 1] && packed_v_t_path[viaPathIndex] == packedShortestPath[shortestPathIndex]) {
                typename QueryDataT::Graph::EdgeIterator edgeID = super::_queryData.graph->FindEdgeInEitherDirection( packed_v_t_path[viaPathIndex - 1], packed_v_t_path[viaPathIndex]);
                *sharingOfViaPath += super::_queryData.graph->GetEdgeData(edgeID).distance;
            } else {
                if (packed_v_t_path[viaPathIndex] == packedShortestPath[shortestPathIndex]) {
                    super::UnpackEdge(packed_v_t_path[viaPathIndex-1], packed_v_t_path[viaPathIndex], partiallyUnpackedViaPath);
                    super::UnpackEdge(packedShortestPath[shortestPathIndex-1] , packedShortestPath[shortestPathIndex], partiallyUnpackedShortestPath);
                    break;
                }
            }
        }

        viaPathIndex = partiallyUnpackedViaPath.size() - 1;
        shortestPathIndex = partiallyUnpackedShortestPath.size() - 1;
        for (; viaPathIndex > 0 && shortestPathIndex > 0; --viaPathIndex,--shortestPathIndex) {
            if (partiallyUnpackedViaPath[viaPathIndex - 1] == partiallyUnpackedShortestPath[shortestPathIndex - 1] && partiallyUnpackedViaPath[viaPathIndex] == partiallyUnpackedShortestPath[shortestPathIndex]) {
                typename QueryDataT::Graph::EdgeIterator edgeID = super::_queryData.graph->FindEdgeInEitherDirection( partiallyUnpackedViaPath[viaPathIndex - 1], partiallyUnpackedViaPath[viaPathIndex]);
                *sharingOfViaPath += super::_queryData.graph->GetEdgeData(edgeID).distance;
            } else {
                break;
            }
        }
        //finished partial unpacking spree! Amount of sharing is stored to appropriate poiner variable
    }

    inline int approximateAmountOfSharing(const NodeID middleNodeIDOfAlternativePath, HeapPtr & _forwardHeap, HeapPtr & _backwardHeap, const std::deque<NodeID> & packedShortestPath) {
        std::deque<NodeID> packedAlternativePath;
        super::RetrievePackedPathFromHeap(_forwardHeap, _backwardHeap, middleNodeIDOfAlternativePath, packedAlternativePath);

        if(packedShortestPath.size() < 2 || packedAlternativePath.size() < 2)
            return 0;

        int sharing = 0;
        int aindex = 0;
        //compute forward sharing
        while( (packedAlternativePath[aindex] == packedShortestPath[aindex]) && (packedAlternativePath[aindex+1] == packedShortestPath[aindex+1]) ) {
            //            INFO("retrieving edge (" << packedAlternativePath[aindex] << "," << packedAlternativePath[aindex+1] << ")");
            typename QueryDataT::Graph::EdgeIterator edgeID = super::_queryData.graph->FindEdgeInEitherDirection(packedAlternativePath[aindex], packedAlternativePath[aindex+1]);
            sharing += super::_queryData.graph->GetEdgeData(edgeID).distance;
            ++aindex;
        }

        aindex = packedAlternativePath.size()-1;
        int bindex = packedShortestPath.size()-1;
        //compute backward sharing
        while( aindex > 0 && bindex > 0 && (packedAlternativePath[aindex] == packedShortestPath[bindex]) && (packedAlternativePath[aindex-1] == packedShortestPath[bindex-1]) ) {
            typename QueryDataT::Graph::EdgeIterator edgeID = super::_queryData.graph->FindEdgeInEitherDirection(packedAlternativePath[aindex], packedAlternativePath[aindex-1]);
            sharing += super::_queryData.graph->GetEdgeData(edgeID).distance;
            --aindex; --bindex;
        }
        return sharing;
    }

    inline void AlternativeRoutingStep(HeapPtr & _forwardHeap, HeapPtr & _backwardHeap, NodeID *middle, int *_upperbound, const int edgeBasedOffset, const bool forwardDirection, std::vector<NodeID>& searchSpaceIntersection) const {
        const NodeID node = _forwardHeap->DeleteMin();

        const int distance = _forwardHeap->GetKey(node);
        if(_backwardHeap->WasInserted(node) ){
            searchSpaceIntersection.push_back(node);

            const int newDistance = _backwardHeap->GetKey(node) + distance;
            if(newDistance < *_upperbound ){
                if(newDistance>=0 ) {
                    *middle = node;
                    *_upperbound = newDistance;
                }
            }
        }

        if((distance-edgeBasedOffset)*(1+VIAPATH_EPSILON) > *_upperbound){
            _forwardHeap->DeleteAll();
            return;
        }

        for ( typename QueryDataT::Graph::EdgeIterator edge = super::_queryData.graph->BeginEdges( node ); edge < super::_queryData.graph->EndEdges(node); edge++ ) {
            const typename QueryDataT::Graph::EdgeData & data = super::_queryData.graph->GetEdgeData(edge);
            bool forwardDirectionFlag = (forwardDirection ? data.forward : data.backward );
            if(forwardDirectionFlag) {

                const NodeID to = super::_queryData.graph->GetTarget(edge);
                const int edgeWeight = data.distance;

                assert( edgeWeight > 0 );
                const int toDistance = distance + edgeWeight;

                //New Node discovered -> Add to Heap + Node Info Storage
                if ( !_forwardHeap->WasInserted( to ) ) {
                    _forwardHeap->Insert( to, toDistance, node );

                }
                //Found a shorter Path -> Update distance
                else if ( toDistance < _forwardHeap->GetKey( to ) ) {
                    _forwardHeap->GetData( to ).parent = node;
                    _forwardHeap->DecreaseKey( to, toDistance );
                    //new parent
                }
            }
        }
    }

    //conduct T-Test
    inline bool viaNodeCandidatePasses_T_Test( HeapPtr& existingForwardHeap, HeapPtr& existingBackwardHeap, HeapPtr& newForwardHeap, HeapPtr& newBackwardHeap, const RankedCandidateNode& candidate, const int offset, const int lengthOfShortestPath, int * lengthOfViaPath, NodeID * s_v_middle, NodeID * v_t_middle) {
        std::deque < NodeID > packed_s_v_path;
        std::deque < NodeID > packed_v_t_path;

        super::_queryData.InitializeOrClearSecondThreadLocalStorage();
        *s_v_middle = UINT_MAX;
        int upperBoundFor_s_v_Path = INT_MAX;
        //compute path <s,..,v> by reusing forward search from s
        newBackwardHeap->Insert(candidate.node, 0, candidate.node);
        while (newBackwardHeap->Size() > 0) {
            super::RoutingStep(newBackwardHeap, existingForwardHeap, s_v_middle, &upperBoundFor_s_v_Path, 2*offset, false);
        }

        if(INT_MAX == upperBoundFor_s_v_Path)
            return false;

        //compute path <v,..,t> by reusing backward search from t
        *v_t_middle = UINT_MAX;
        int upperBoundFor_v_t_Path = INT_MAX;
        newForwardHeap->Insert(candidate.node, 0, candidate.node);
        while (newForwardHeap->Size() > 0) {
            super::RoutingStep(newForwardHeap, existingBackwardHeap, v_t_middle, &upperBoundFor_v_t_Path, 2*offset, true);
        }

        if(INT_MAX == upperBoundFor_v_t_Path)
            return false;

        *lengthOfViaPath = upperBoundFor_s_v_Path + upperBoundFor_v_t_Path;

        //retrieve packed paths
        super::RetrievePackedPathFromHeap(existingForwardHeap, newBackwardHeap, *s_v_middle, packed_s_v_path);
        super::RetrievePackedPathFromHeap(newForwardHeap, existingBackwardHeap, *v_t_middle, packed_v_t_path);

        NodeID s_P = *s_v_middle, t_P = *v_t_middle;
        const int T_threshold = VIAPATH_EPSILON * lengthOfShortestPath;
        int unpackedUntilDistance = 0;

        std::stack<UnpackEdge> unpackStack;
        //Traverse path s-->v
        for (unsigned i = packed_s_v_path.size() - 1; (i > 0) && unpackStack.empty(); --i) {
            typename QueryDataT::Graph::EdgeIterator edgeID = super::_queryData.graph->FindEdgeInEitherDirection( packed_s_v_path[i - 1], packed_s_v_path[i]);
            int lengthOfCurrentEdge = super::_queryData.graph->GetEdgeData(edgeID).distance;
            if (lengthOfCurrentEdge + unpackedUntilDistance >= T_threshold) {
                unpackStack.push(std::make_pair(packed_s_v_path[i - 1], packed_s_v_path[i]));
            } else {
                unpackedUntilDistance += lengthOfCurrentEdge;
                s_P = packed_s_v_path[i - 1];
            }
        }

        while (!unpackStack.empty()) {
            const UnpackEdge viaPathEdge = unpackStack.top();
            unpackStack.pop();
            typename QueryDataT::Graph::EdgeIterator edgeIDInViaPath = super::_queryData.graph->FindEdgeInEitherDirection(viaPathEdge.first, viaPathEdge.second);
            if(UINT_MAX == edgeIDInViaPath)
                return false;
            typename QueryDataT::Graph::EdgeData currentEdgeData = super::_queryData.graph->GetEdgeData(edgeIDInViaPath);
            bool IsViaEdgeShortCut = currentEdgeData.shortcut;
            if (IsViaEdgeShortCut) {
                const NodeID middleOfViaPath = currentEdgeData.id;
                typename QueryDataT::Graph::EdgeIterator edgeIDOfSecondSegment = super::_queryData.graph->FindEdgeInEitherDirection(middleOfViaPath, viaPathEdge.second);
                int lengthOfSecondSegment = super::_queryData.graph->GetEdgeData(edgeIDOfSecondSegment).distance;
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
            typename QueryDataT::Graph::EdgeIterator edgeID = super::_queryData.graph->FindEdgeInEitherDirection( packed_v_t_path[i], packed_v_t_path[i + 1]);
            int lengthOfCurrentEdge = super::_queryData.graph->GetEdgeData(edgeID).distance;
            if (lengthOfCurrentEdge + unpackedUntilDistance >= T_threshold) {
                unpackStack.push( std::make_pair(packed_v_t_path[i], packed_v_t_path[i + 1]));
            } else {
                unpackedUntilDistance += lengthOfCurrentEdge;
                t_P = packed_v_t_path[i + 1];
            }
        }

        while (!unpackStack.empty()) {
            const UnpackEdge viaPathEdge = unpackStack.top();
            unpackStack.pop();
            typename QueryDataT::Graph::EdgeIterator edgeIDInViaPath = super::_queryData.graph->FindEdgeInEitherDirection(viaPathEdge.first, viaPathEdge.second);
            if(UINT_MAX == edgeIDInViaPath)
                return false;
            typename QueryDataT::Graph::EdgeData currentEdgeData = super::_queryData.graph->GetEdgeData(edgeIDInViaPath);
            const bool IsViaEdgeShortCut = currentEdgeData.shortcut;
            if (IsViaEdgeShortCut) {
                const NodeID middleOfViaPath = currentEdgeData.id;
                typename QueryDataT::Graph::EdgeIterator edgeIDOfFirstSegment = super::_queryData.graph->FindEdgeInEitherDirection(viaPathEdge.first, middleOfViaPath);
                int lengthOfFirstSegment = super::_queryData.graph->GetEdgeData( edgeIDOfFirstSegment).distance;
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
        HeapPtr& forwardHeap = super::_queryData.forwardHeap3;
        HeapPtr& backwardHeap = super::_queryData.backwardHeap3;
        super::_queryData.InitializeOrClearThirdThreadLocalStorage();
        int _upperBound = INT_MAX;
        NodeID middle = UINT_MAX;
        forwardHeap->Insert(s_P, 0, s_P);
        backwardHeap->Insert(t_P, 0, t_P);
        //exploration from s and t until deletemin/(1+epsilon) > _lengthOfShortestPath
        while (forwardHeap->Size() + backwardHeap->Size() > 0) {
            if (forwardHeap->Size() > 0) {
                super::RoutingStep(forwardHeap, backwardHeap, &middle, &_upperBound, offset, true);
            }
            if (backwardHeap->Size() > 0) {
                super::RoutingStep(backwardHeap, forwardHeap, &middle, &_upperBound, offset, false);
            }
        }
        return (_upperBound <= lengthOfPathT_Test_Path);
    }
};

#endif /* ALTERNATIVEROUTES_H_ */
