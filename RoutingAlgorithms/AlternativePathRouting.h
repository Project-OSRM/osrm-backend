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

        int offset = (phantomNodePair.startPhantom.isBidirected() ? std::max(phantomNodePair.startPhantom.weight1, phantomNodePair.startPhantom.weight2) : phantomNodePair.startPhantom.weight1) ;
        offset += (phantomNodePair.targetPhantom.isBidirected() ? std::max(phantomNodePair.targetPhantom.weight1, phantomNodePair.targetPhantom.weight2) : phantomNodePair.targetPhantom.weight1) ;

        //exploration from s and t until deletemin/(1+epsilon) > _lengthOfShortestPath
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
        //        std::cout << "middle: " << middle << ", other: ";
        //        for(unsigned i = 0; i < size; ++i)
        //            if(middle != viaNodeCandidates[i])
        //                std::cout << viaNodeCandidates[i] << " ";
        //        std::cout << std::endl;
        viaNodeCandidates.resize(size);
        //        INFO("found " << viaNodeCandidates.size() << " nodes in search space intersection");
        //
        //        INFO("upper bound: " << _upperBound);
        std::deque<NodeID> packedShortestPath;

        //save (packed) shortest path of shortest path and keep it for later use.
        //we need it during the checks and dont want to recompute it always
        super::RetrievePackedPathFromHeap(forwardHeap, backwardHeap, middle, packedShortestPath);

        //ch-pruning of via nodes in both search spaces

        std::vector< PreselectedNode> nodesThatPassPreselection;

        BOOST_FOREACH(const NodeID node, viaNodeCandidates) {
            if(node == middle)
                continue;

            //            std::cout << "via path over " << node << std::endl;
            int sharing = approximateAmountOfSharing(node, forwardHeap, backwardHeap, packedShortestPath);

            int length1 = forwardHeap->GetKey(node);
            int length2 = backwardHeap->GetKey(node);
            //            std::cout << "  length: " << length1+length2 << std::endl;
            bool lengthPassed = (length1+length2 < _upperBound*(1+VIAPATH_EPSILON));
            //            std::cout << "  length passed: " << (lengthPassed ?osrm "yes" : "no") << std::endl;
            //            std::cout << "  apx-sharing: " << sharing << std::endl;
            bool sharingPassed = (sharing <= _upperBound*VIAPATH_GAMMA);
            //            std::cout << "  apx-sharing passed: " << ( sharingPassed ? "yes" : "no") << std::endl;
            bool stretchPassed = length1+length2 - sharing < (1.+VIAPATH_EPSILON)*(_upperBound-sharing);
            //            std::cout << "  apx-stretch passed: " << ( stretchPassed ? "yes" : "no") << std::endl;

            if(lengthPassed && sharingPassed && stretchPassed)
                nodesThatPassPreselection.push_back(std::make_pair(node, length1+length2));
        }

        std::vector<RankedCandidateNode > rankedCandidates;

        //        INFO(nodesThatPassPreselection.size() << " out of " << viaNodeCandidates.size() << " passed preselection");
        //prioritizing via nodes
        BOOST_FOREACH(const PreselectedNode node, nodesThatPassPreselection) {
            int lengthOfViaPath = 0;
            int sharingOfViaPath = 0;

            computeLengthAndSharingOfViaPath(phantomNodePair, node, &lengthOfViaPath, &sharingOfViaPath, offset, packedShortestPath);
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
        if(INT_MAX != _upperBound)
            super::UnpackPath(packedShortestPath, rawRouteData.computedShortestPath);

        if(selectedViaNode != UINT_MAX)
            retrievePackedViaPath(forwardHeap, backwardHeap, forwardHeap2, backwardHeap2, s_v_middle, v_t_middle, rawRouteData.computedAlternativePath);

        rawRouteData.lengthOfShortestPath = _upperBound;
        rawRouteData.lengthOfAlternativePath = lengthOfViaPath;
    }

private:
    //unpack <s,..,v,..,t> by exploring search spaces from v
    inline void retrievePackedViaPath(HeapPtr & _forwardHeap1, HeapPtr & _backwardHeap1, HeapPtr & _forwardHeap2, HeapPtr & _backwardHeap2, const NodeID s_v_middle, const NodeID v_t_middle, std::vector<_PathData> & unpackedPath) {
        //unpack s,v
        std::deque<NodeID> packed_s_v_path, packed_v_t_path;
        //        std::cout << "1" << std::endl;
        super::RetrievePackedPathFromHeap(_forwardHeap1, _backwardHeap2, s_v_middle, packed_s_v_path);
        //        std::cout << "2" << std::endl;
        packed_s_v_path.resize(packed_s_v_path.size()-1);
        super::RetrievePackedPathFromHeap(_forwardHeap2, _backwardHeap1, v_t_middle, packed_v_t_path);
        //        std::cout << "3" << std::endl;
        packed_s_v_path.insert(packed_s_v_path.end(),packed_v_t_path.begin(), packed_v_t_path.end() );
        //        std::cout << "4" << std::endl;

        //        for(unsigned i = 0; i < packed_s_v_path.size(); ++i)
        //            std::cout << packed_s_v_path[i] << " " << std::endl;
        //        std::cout << std::endl;

        super::UnpackPath(packed_s_v_path, unpackedPath);

    }

    inline void computeLengthAndSharingOfViaPath(const PhantomNodes & phantomNodePair, const PreselectedNode& node, int *lengthOfViaPath, int *sharingOfViaPath, const int offset, const std::deque<NodeID> & packedShortestPath) {
        //compute and unpack <s,..,v> and <v,..,t> by exploring search spaces from v and intersecting against queues
        //only half-searches have to be done at this stage
        //        std::cout << "deep check for via path " << node.first << std::endl;

        super::_queryData.InitializeOrClearSecondThreadLocalStorage();

        HeapPtr & existingForwardHeap  = super::_queryData.forwardHeap;
        HeapPtr & existingBackwardHeap = super::_queryData.backwardHeap;
        HeapPtr & newForwardHeap       = super::_queryData.forwardHeap2;
        HeapPtr & newBackwardHeap      = super::_queryData.backwardHeap2;

        NodeID s_v_middle = UINT_MAX;
        int upperBoundFor_s_v_Path = INT_MAX;//compute path <s,..,v> by reusing forward search from s
        newBackwardHeap->Insert(node.first, 0, node.first);
        while (newBackwardHeap->Size() > 0) {
            super::RoutingStep(newBackwardHeap, existingForwardHeap, &s_v_middle, &upperBoundFor_s_v_Path, 2 * offset, false);
        }
        //        std::cout << "  length of <s,..,v>: " << upperBoundFor_s_v_Path << " with middle node " << s_v_middle << std::endl;
        //compute path <v,..,t> by reusing backward search from t
        NodeID v_t_middle = UINT_MAX;
        int upperBoundFor_v_t_Path = INT_MAX;
        newForwardHeap->Insert(node.first, 0, node.first);
        while (newForwardHeap->Size() > 0) {
            super::RoutingStep(newForwardHeap, existingBackwardHeap, &v_t_middle, &upperBoundFor_v_t_Path, 2 * offset, true);
        }
        //        std::cout << "  length of <v,..,t>: " << upperBoundFor_v_t_Path << " with middle node " << v_t_middle << std::endl;
        *lengthOfViaPath = upperBoundFor_s_v_Path + upperBoundFor_v_t_Path;
        //        std::cout << "  exact length of via path: " << *lengthOfViaPath << std::endl;

        std::deque < NodeID > packed_s_v_path;
        std::deque < NodeID > packed_v_t_path;
        //retrieve packed paths
        super::RetrievePackedPathFromHeap(existingForwardHeap, newBackwardHeap, s_v_middle, packed_s_v_path);
        super::RetrievePackedPathFromHeap(newForwardHeap, existingBackwardHeap, v_t_middle, packed_v_t_path);
        typedef std::pair<NodeID, NodeID> UnpackEdge;
        std::stack<UnpackEdge> unpackStack;
        //partial unpacking, compute sharing
        //First partially unpack s-->v until paths deviate, note length of common path.
        //        std::cout << "length of packed sv-path: " << packed_s_v_path.size() << ", length of packed shortest path: " << packedShortestPath.size() << std::endl;
        for (unsigned i = 0, lengthOfPackedPath = std::min( packed_s_v_path.size(), packedShortestPath.size()) - 1; (i < lengthOfPackedPath) && unpackStack.empty(); ++i) {
            //            std::cout << "   checking indices [" << i << "] and [" << (i + 1) << "]" << std::endl;
            if (packed_s_v_path[i] == packedShortestPath[i] && packed_s_v_path[i + 1] == packedShortestPath[i + 1]) {
                typename QueryDataT::Graph::EdgeIterator edgeID = super::_queryData.graph->FindEdgeInEitherDirection(packed_s_v_path[i], packed_s_v_path[i + 1]);
                *sharingOfViaPath += super::_queryData.graph->GetEdgeData(edgeID).distance;
            } else {
                if (packed_s_v_path[i] == packedShortestPath[i]) {
                    unpackStack.push( std::make_pair(packed_s_v_path[i], packed_s_v_path[i + 1]));
                    unpackStack.push( std::make_pair(packedShortestPath[i], packedShortestPath[i + 1]));
                }
            }

        }

        while (!unpackStack.empty()) {
            const UnpackEdge shortestPathEdge = unpackStack.top();
            unpackStack.pop();
            const UnpackEdge viaPathEdge = unpackStack.top();
            unpackStack.pop();
            //            std::cout << "  unpacking edges (" << shortestPathEdge.first << "," << shortestPathEdge.second << ") and (" << viaPathEdge.first << "," << viaPathEdge.second << ")" << std::endl;
            typename QueryDataT::Graph::EdgeIterator edgeIDInShortestPath = super::_queryData.graph->FindEdgeInEitherDirection( shortestPathEdge.first, shortestPathEdge.second);
            typename QueryDataT::Graph::EdgeIterator edgeIDInViaPath = super::_queryData.graph->FindEdgeInEitherDirection(viaPathEdge.first, viaPathEdge.second);
            //            std::cout << "   ids are " << edgeIDInShortestPath << " (shortest) and " << edgeIDInViaPath << " (via)" << std::endl;
            bool IsShortestPathEdgeShortCut = super::_queryData.graph->GetEdgeData(edgeIDInShortestPath).shortcut;
            bool IsViaEdgeShortCut = super::_queryData.graph->GetEdgeData(edgeIDInViaPath).shortcut;

            const NodeID middleOfShortestPath   = !IsShortestPathEdgeShortCut   ? UINT_MAX : super::_queryData.graph->GetEdgeData(edgeIDInShortestPath).id;
            const NodeID middleOfViaPath        = !IsViaEdgeShortCut            ? UINT_MAX : super::_queryData.graph->GetEdgeData(edgeIDInViaPath     ).id;

            if (IsShortestPathEdgeShortCut || IsViaEdgeShortCut) {
                if (middleOfShortestPath != middleOfViaPath) { // unpack first segment
                    //put first segment of via edge on stack, else take the segment already available
                    if (IsViaEdgeShortCut)
                        unpackStack.push( std::make_pair(viaPathEdge.first, middleOfViaPath));
                    else unpackStack.push(viaPathEdge);

                    //put first segment of shortest path edge on stack if not a shortcut, else take the segment already available
                    if (IsShortestPathEdgeShortCut)
                        unpackStack.push( std::make_pair(shortestPathEdge.first, middleOfShortestPath));
                    else unpackStack.push(shortestPathEdge);

                } else { // unpack second segment
                    if (IsViaEdgeShortCut)
                        unpackStack.push( std::make_pair(middleOfViaPath, viaPathEdge.second));
                    else unpackStack.push(viaPathEdge);

                    //put first segment of shortest path edge on stack if not a shortcut, else take the segment already available
                    if (IsShortestPathEdgeShortCut)
                        unpackStack.push( std::make_pair(middleOfShortestPath, shortestPathEdge.second));
                    else unpackStack.push(shortestPathEdge);

                    //add length of first segment to amount of sharing
                    typename QueryDataT::Graph::EdgeIterator edgeIDInViaPath = super::_queryData.graph->FindEdgeInEitherDirection(viaPathEdge.first, viaPathEdge.second);
                    *sharingOfViaPath += super::_queryData.graph->GetEdgeData(edgeIDInViaPath).distance;
                }
            }
        }
        //        std::cout << "sharing of SV-Path: " << *sharingOfViaPath << std::endl;
        //Second, partially unpack v-->t in reverse until paths deviate and note lengths
        unsigned viaPathIndex = packed_v_t_path.size() - 1;
        unsigned shortestPathIndex = packedShortestPath.size() - 1;
        //        std::cout << "length of packed vt-path: " << packed_v_t_path.size() << ", length of packed shortest path: " << packedShortestPath.size() << std::endl;
        for (; viaPathIndex > 0 && shortestPathIndex > 0;) {
            //            std::cout << "   checking indices [" << shortestPathIndex << "] and [" << (shortestPathIndex-1) << "] (shortest) as well as [" <<  shortestPathIndex << "] and [" << (shortestPathIndex-1) << "]" << std::endl;
            if (packed_v_t_path[viaPathIndex - 1] == packedShortestPath[shortestPathIndex - 1] && packed_v_t_path[viaPathIndex] == packedShortestPath[shortestPathIndex]) {
                typename QueryDataT::Graph::EdgeIterator edgeID = super::_queryData.graph->FindEdgeInEitherDirection( packed_v_t_path[viaPathIndex - 1], packed_v_t_path[viaPathIndex]);
                //                std::cout << "Id of edge (" << packed_v_t_path[viaPathIndex-1] << "," << packed_v_t_path[viaPathIndex] << ") : " << edgeID << std::endl;
                *sharingOfViaPath += super::_queryData.graph->GetEdgeData( edgeID).distance;
            } else {
                if (packed_v_t_path[viaPathIndex] == packedShortestPath[shortestPathIndex]) {
                    unpackStack.push( std::make_pair( packed_v_t_path[viaPathIndex - 1]         , packed_v_t_path[viaPathIndex]         ));
                    unpackStack.push( std::make_pair( packedShortestPath[shortestPathIndex - 1] , packedShortestPath[shortestPathIndex] ));
                }
            }
            --viaPathIndex;
            --shortestPathIndex;
        }

        while (!unpackStack.empty()) {
            const UnpackEdge shortestPathEdge = unpackStack.top();
            unpackStack.pop();
            const UnpackEdge viaPathEdge = unpackStack.top();
            unpackStack.pop();
            //            std::cout << "  unpacking edges (" << shortestPathEdge.first << "," << shortestPathEdge.second << ") and (" << viaPathEdge.first << "," << viaPathEdge.second << ")" << std::endl;
            typename QueryDataT::Graph::EdgeIterator edgeIDInShortestPath = super::_queryData.graph->FindEdgeInEitherDirection(shortestPathEdge.first, shortestPathEdge.second);
            //            std::cout << "!" << std::endl;
            typename QueryDataT::Graph::EdgeIterator edgeIDInViaPath = super::_queryData.graph->FindEdgeInEitherDirection( viaPathEdge.first, viaPathEdge.second);
            //            std::cout << "   ids are " << edgeIDInShortestPath << " (shortest) and " << edgeIDInViaPath << " (via)" << std::endl;
            bool IsShortestPathEdgeShortCut = super::_queryData.graph->GetEdgeData(edgeIDInShortestPath).shortcut;
            bool IsViaEdgeShortCut = super::_queryData.graph->GetEdgeData( edgeIDInViaPath).shortcut;

            const NodeID middleOfShortestPath = !IsShortestPathEdgeShortCut ? UINT_MAX : super::_queryData.graph->GetEdgeData(edgeIDInShortestPath).id;
            const NodeID middleOfViaPath      = !IsViaEdgeShortCut          ? UINT_MAX : super::_queryData.graph->GetEdgeData(edgeIDInViaPath     ).id;

            //            std::cout << "  shortest shrtcut: " << (IsShortestPathEdgeShortCut ? "yes" : "no") << "(" << middleOfShortestPath << ") , via shrtcut: " << (IsViaEdgeShortCut ? "yes" : "no") << "(" << middleOfViaPath << ")" << std::endl;

            if (IsShortestPathEdgeShortCut || IsViaEdgeShortCut) {
                if (middleOfShortestPath == middleOfViaPath) { // unpack first segment
                    //put first segment of via edge on stack, else take the segment already available
                    //                    std::cout << "  unpacking first segment" << std::endl;
                    if (IsViaEdgeShortCut)
                        unpackStack.push( std::make_pair(viaPathEdge.first, middleOfViaPath));
                    else unpackStack.push(viaPathEdge);

                    //put first segment of shortest path edge on stack if not a shortcut, else take the segment already available
                    if (IsShortestPathEdgeShortCut)
                        unpackStack.push( std::make_pair(shortestPathEdge.first, middleOfShortestPath));
                    else unpackStack.push(shortestPathEdge);

                    //add length of first segment to amount of sharing
                    typename QueryDataT::Graph::EdgeIterator edgeIDInViaPath = super::_queryData.graph->FindEdgeInEitherDirection(viaPathEdge.first, viaPathEdge.second);
                    *sharingOfViaPath += super::_queryData.graph->GetEdgeData( edgeIDInViaPath).distance;
                } else { // unpack second segment
                    //                    std::cout << "  unpacking second segment" << std::endl;
                    if (IsViaEdgeShortCut)
                        unpackStack.push( std::make_pair(middleOfViaPath, viaPathEdge.second));
                    else unpackStack.push(viaPathEdge);

                    //put first segment of shortest path edge on stack if not a shortcut, else take the segment already available
                    if (IsShortestPathEdgeShortCut)
                        unpackStack.push( std::make_pair(middleOfShortestPath, shortestPathEdge.second));
                    else unpackStack.push(shortestPathEdge);
                }
            }
        }
        //        std::cout << "sharing of SVT-Path: " << *sharingOfViaPath << std::endl;
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
//                    INFO("upper bound decrease to: " << newDistance);
                    *middle = node;
                    *_upperbound = newDistance;
                }
            }
        }

        //0.8 implies an epsilon of 25%
        if((distance-edgeBasedOffset)*VIAPATH_GAMMA > *_upperbound){
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

    unsigned computeOverlap(const NodeID s, const NodeID t, const NodeID v) {
        return 0;
    }

    //conduct T-Test
    inline bool viaNodeCandidatePasses_T_Test( HeapPtr& existingForwardHeap, HeapPtr& existingBackwardHeap, HeapPtr& newForwardHeap, HeapPtr& newBackwardHeap, const RankedCandidateNode& candidate, const int offset, const int lengthOfShortestPath, int * lengthOfViaPath, NodeID * s_v_middle, NodeID * v_t_middle) {
        //    std::cout << "computing via path for T-Test " << candidate.node << std::endl;
//        int lengthOfViaPath = 0;
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

        //    std::cout << "  length of <s,..,v>: " << upperBoundFor_s_v_Path << " with middle node " << s_v_middle << std::endl;
        //compute path <v,..,t> by reusing backward search from t
        *v_t_middle = UINT_MAX;
        int upperBoundFor_v_t_Path = INT_MAX;
        newForwardHeap->Insert(candidate.node, 0, candidate.node);
        while (newForwardHeap->Size() > 0) {
            super::RoutingStep(newForwardHeap, existingBackwardHeap, v_t_middle, &upperBoundFor_v_t_Path, 2*offset, true);
        }

        if(INT_MAX == upperBoundFor_v_t_Path)
            return false;

        //    std::cout << "  length of <v,..,t>: " << upperBoundFor_v_t_Path << " with middle node " << v_t_middle << std::endl;
        *lengthOfViaPath = upperBoundFor_s_v_Path + upperBoundFor_v_t_Path;
        //    std::cout << "  exact length of via path: " << lengthOfViaPath << std::endl;
        //    std::cout << "  T-Test shall pass with length 0.25*" << (lengthOfShortestPath) << "=" << 0.25 * (lengthOfShortestPath) << std::endl;
        std::deque < NodeID > packed_s_v_path;
        std::deque < NodeID > packed_v_t_path;
        //retrieve packed paths
        //    std::cout << "  retrieving packed path for middle nodes " << middleOfShortestPath << "," << s_v_middle << "," << v_t_middle << " (shorstest, sv, vt)" << std::endl;
        super::RetrievePackedPathFromHeap(existingForwardHeap, newBackwardHeap, *s_v_middle, packed_s_v_path);
        super::RetrievePackedPathFromHeap(newForwardHeap, existingBackwardHeap, *v_t_middle, packed_v_t_path);
        //    std::cout << "packed sv: ";
        //    for (unsigned i = 0; i < packed_s_v_path.size(); ++i) {
        //        std::cout << packed_s_v_path[i] << " ";
        //    }
        //    std::cout << std::endl;
        //    std::cout << "packed vt: ";
        //    for (unsigned i = 0; i < packed_v_t_path.size(); ++i) {
        //        std::cout << packed_v_t_path[i] << " ";
        //    }
        //    std::cout << std::endl;
        //        std::cout << "packed shortest: ";
        //        for(unsigned i = 0; i < packedShortestPath.size(); ++i) {
        //            std::cout << packedShortestPath[i] << " ";
        //        }
        //        std::cout << std::endl;
        NodeID s_P = *s_v_middle, t_P = *v_t_middle;
        const int T_threshold = VIAPATH_EPSILON * lengthOfShortestPath;
        int unpackedUntilDistance = 0;
        typedef std::pair<NodeID, NodeID> UnpackEdge;
        std::stack<UnpackEdge> unpackStack;
        //partial unpacking until target of edge is the first endpoint of a non-shortcut edge farther away than threshold
        //First partially unpack s-->v until paths deviate, note length of common path.
        //    std::cout << "unpacking sv-path until a node of non-shortcut edge is farther away than " << T_threshold << std::endl;
        for (unsigned i = packed_s_v_path.size() - 1; (i > 0) && unpackStack.empty(); --i) {
            //        std::cout << "   checking indices [" << i << "] and [" << (i + 1) << "]" << std::endl;
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
            //        std::cout << "  unpacking edge (" << viaPathEdge.first << "," << viaPathEdge.second << ")" << std::endl;
            typename QueryDataT::Graph::EdgeIterator edgeIDInViaPath = super::_queryData.graph->FindEdgeInEitherDirection(viaPathEdge.first, viaPathEdge.second);
            //        std::cout << "   id is " << edgeIDInViaPath << " (via)" << std::endl;
            if(UINT_MAX == edgeIDInViaPath)
                return false;
            typename QueryDataT::Graph::EdgeData currentEdgeData = super::_queryData.graph->GetEdgeData(edgeIDInViaPath);
            bool IsViaEdgeShortCut = currentEdgeData.shortcut;
            if (IsViaEdgeShortCut) {
                const NodeID middleOfViaPath = currentEdgeData.id;
//                typename QueryDataT::Graph::EdgeIterator edgeIDOfFirstSegment = super::_queryData.graph->FindEdgeInEitherDirection(viaPathEdge.first, middleOfViaPath);
                typename QueryDataT::Graph::EdgeIterator edgeIDOfSecondSegment = super::_queryData.graph->FindEdgeInEitherDirection(middleOfViaPath, viaPathEdge.second);
//                int lengthOfFirstSegment = super::_queryData.graph->GetEdgeData(edgeIDOfFirstSegment).distance;
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

        //    std::cout << "threshold: " << T_threshold << ", unpackedDistance: " << unpackedUntilDistance << ", s_P: " << s_P << std::endl;
        int lengthOfPathT_Test_Path = unpackedUntilDistance;
        unpackedUntilDistance = 0;
        //partial unpacking until target of edge is the first endpoint of a non-shortcut edge farther away than threshold
        //First partially unpack s-->v until paths deviate, note length of common path.
        //    std::cout << "unpacking vt-path until a node of non-shortcut edge is farther away than " << T_threshold << std::endl;
        for (unsigned i = 0, lengthOfPackedPath = packed_v_t_path.size() - 1; (i < lengthOfPackedPath) && unpackStack.empty(); ++i) {
            //        std::cout << "   checking indices [" << i << "] and [" << (i + 1) << "]" << std::endl;
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
            //        std::cout << "  unpacking edge (" << viaPathEdge.first << "," << viaPathEdge.second << ")" << std::endl;
            typename QueryDataT::Graph::EdgeIterator edgeIDInViaPath = super::_queryData.graph->FindEdgeInEitherDirection(viaPathEdge.first, viaPathEdge.second);
            //        std::cout << "   id is " << edgeIDInViaPath << " (via)" << std::endl;
            if(UINT_MAX == edgeIDInViaPath)
                return false;
            typename QueryDataT::Graph::EdgeData currentEdgeData = super::_queryData.graph->GetEdgeData(edgeIDInViaPath);
            bool IsViaEdgeShortCut = currentEdgeData.shortcut;
            if (IsViaEdgeShortCut) {
                const NodeID middleOfViaPath = currentEdgeData.id;
                typename QueryDataT::Graph::EdgeIterator edgeIDOfFirstSegment = super::_queryData.graph->FindEdgeInEitherDirection(viaPathEdge.first, middleOfViaPath);
//                typename QueryDataT::Graph::EdgeIterator edgeIDOfSecondSegment = super::_queryData.graph->FindEdgeInEitherDirection(middleOfViaPath, viaPathEdge.second);
                int lengthOfFirstSegment = super::_queryData.graph->GetEdgeData( edgeIDOfFirstSegment).distance;
//                int lengthOfSecondSegment = super::_queryData.graph->GetEdgeData( edgeIDOfSecondSegment).distance;
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
        //    std::cout << "check if path (" << s_P << "," << t_P << ") is not less than " << lengthOfPathT_Test_Path << ", while shortest path has length: " << lengthOfShortestPath << std::endl;
        //Run query and compare distances.
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
        //    std::cout << "lengthOfPathT_Test_Path: " << lengthOfPathT_Test_Path << ", _upperBound: " << _upperBound << std::endl;
        bool hasPassed_T_Test = (_upperBound == lengthOfPathT_Test_Path);
        //    std::cout << "passed T-Test: " << (hasPassed_T_Test ? "yes" : "no") << std::endl;
        return hasPassed_T_Test;
    }
};

#endif /* ALTERNATIVEROUTES_H_ */
