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

#ifndef SEARCHENGINE_H_
#define SEARCHENGINE_H_

#include <climits>
#include <deque>
#include "SimpleStack.h"

#include <boost/thread.hpp>

#include "BinaryHeap.h"
#include "PhantomNodes.h"
#include "../Util/StringUtil.h"
#include "../typedefs.h"

struct _HeapData {
	NodeID parent;
	_HeapData( NodeID p ) : parent(p) { }
};

typedef boost::thread_specific_ptr<BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> > > HeapPtr;

template<class EdgeData, class GraphT>
class SearchEngine {
private:
	const GraphT * _graph;
	NodeInformationHelpDesk * nodeHelpDesk;
	std::vector<string> * _names;
	static HeapPtr _forwardHeap;
	static HeapPtr _backwardHeap;
	static HeapPtr _forwardHeap2;
	static HeapPtr _backwardHeap2;
	inline double absDouble(double input) { if(input < 0) return input*(-1); else return input;}
public:
	SearchEngine(GraphT * g, NodeInformationHelpDesk * nh, std::vector<string> * n = new std::vector<string>()) : _graph(g), nodeHelpDesk(nh), _names(n) {}
	~SearchEngine() {}

	inline const void GetCoordinatesForNodeID(NodeID id, _Coordinate& result) const {
		result.lat = nodeHelpDesk->getLatitudeOfNode(id);
		result.lon = nodeHelpDesk->getLongitudeOfNode(id);
	}

	inline void InitializeThreadLocalStorageIfNecessary() {
		if(!_forwardHeap.get()) {
			_forwardHeap.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> >(nodeHelpDesk->getNumberOfNodes()));
		}
		else
			_forwardHeap->Clear();

		if(!_backwardHeap.get()) {
			_backwardHeap.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> >(nodeHelpDesk->getNumberOfNodes()));
		}
		else
			_backwardHeap->Clear();
	}

	inline void InitializeThreadLocalViaStorageIfNecessary() {
		if(!_forwardHeap2.get()) {
			_forwardHeap2.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> >(nodeHelpDesk->getNumberOfNodes()));
		}
		else
			_forwardHeap2->Clear();

		if(!_backwardHeap2.get()) {
			_backwardHeap2.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData, UnorderedMapStorage<NodeID, int> >(nodeHelpDesk->getNumberOfNodes()));
		}
		else
			_backwardHeap2->Clear();
	}

	template<class ContainerT>
	void _RemoveConsecutiveDuplicatesFromContainer(ContainerT & packedPath) {
	    //remove consecutive duplicates
		typename ContainerT::iterator it;
		// using default comparison:
		it = std::unique(packedPath.begin(), packedPath.end());
		packedPath.resize(it - packedPath.begin());
	}

	int ComputeViaRoute(std::vector<PhantomNodes> & phantomNodesVector, std::vector<_PathData> & unpackedPath) {
		BOOST_FOREACH(PhantomNodes & phantomNodePair, phantomNodesVector) {
			if(!phantomNodePair.AtLeastOnePhantomNodeIsUINTMAX())
				return INT_MAX;
		}
		int distance1 = 0;
		int distance2 = 0;

		bool searchFrom1stStartNode(true);
		bool searchFrom2ndStartNode(true);
		NodeID middle1 = ( NodeID ) UINT_MAX;
		NodeID middle2 = ( NodeID ) UINT_MAX;
		std::deque<NodeID> packedPath1;
		std::deque<NodeID> packedPath2;
		//Get distance to next pair of target nodes.
		BOOST_FOREACH(PhantomNodes & phantomNodePair, phantomNodesVector) {
			InitializeThreadLocalStorageIfNecessary();
			InitializeThreadLocalViaStorageIfNecessary();

			int _localUpperbound1 = INT_MAX;
			int _localUpperbound2 = INT_MAX;

			_forwardHeap->Clear();
			_forwardHeap2->Clear();
			//insert new starting nodes into forward heap, adjusted by previous distances.
			if(searchFrom1stStartNode) {
				_forwardHeap->Insert(phantomNodePair.startPhantom.edgeBasedNode, -phantomNodePair.startPhantom.weight1, phantomNodePair.startPhantom.edgeBasedNode);
				_forwardHeap2->Insert(phantomNodePair.startPhantom.edgeBasedNode, -phantomNodePair.startPhantom.weight1, phantomNodePair.startPhantom.edgeBasedNode);
//				INFO("a 1,2)forw insert " << phantomNodePair.startPhantom.edgeBasedNode << " with weight " << phantomNodePair.startPhantom.weight1);
//			} else {
//				INFO("Skipping first start node");
			}
			if(phantomNodePair.startPhantom.isBidirected() && searchFrom2ndStartNode) {
				_forwardHeap->Insert(phantomNodePair.startPhantom.edgeBasedNode+1, -phantomNodePair.startPhantom.weight2, phantomNodePair.startPhantom.edgeBasedNode+1);
				_forwardHeap2->Insert(phantomNodePair.startPhantom.edgeBasedNode+1, -phantomNodePair.startPhantom.weight2, phantomNodePair.startPhantom.edgeBasedNode+1);
//				INFO("b 1,2)forw insert " << phantomNodePair.startPhantom.edgeBasedNode+1 << " with weight " << -phantomNodePair.startPhantom.weight1);
//			} else if(!searchFrom2ndStartNode) {
//				INFO("Skipping second start node");
			}

			_backwardHeap->Clear();
			_backwardHeap2->Clear();
			//insert new backward nodes into backward heap, unadjusted.
			_backwardHeap->Insert(phantomNodePair.targetPhantom.edgeBasedNode, phantomNodePair.targetPhantom.weight1, phantomNodePair.targetPhantom.edgeBasedNode);
//			INFO("1) back insert " << phantomNodePair.targetPhantom.edgeBasedNode << " with weight " << phantomNodePair.targetPhantom.weight1);
			if(phantomNodePair.targetPhantom.isBidirected() ) {
//				INFO("2) back insert " << phantomNodePair.targetPhantom.edgeBasedNode+1 << " with weight " << phantomNodePair.targetPhantom.weight2);
				_backwardHeap2->Insert(phantomNodePair.targetPhantom.edgeBasedNode+1, phantomNodePair.targetPhantom.weight2, phantomNodePair.targetPhantom.edgeBasedNode+1);
			}
			int offset = (phantomNodePair.startPhantom.isBidirected() ? std::max(phantomNodePair.startPhantom.weight1, phantomNodePair.startPhantom.weight2) : phantomNodePair.startPhantom.weight1) ;
			offset += (phantomNodePair.targetPhantom.isBidirected() ? std::max(phantomNodePair.targetPhantom.weight1, phantomNodePair.targetPhantom.weight2) : phantomNodePair.targetPhantom.weight1) ;

			//run two-Target Dijkstra routing step.
			while(_forwardHeap->Size() + _backwardHeap->Size() > 0){
				if(_forwardHeap->Size() > 0){
					_RoutingStep(_forwardHeap, _backwardHeap, true, &middle1, &_localUpperbound1, 2*offset);
				}
				if(_backwardHeap->Size() > 0){
					_RoutingStep(_backwardHeap, _forwardHeap, false, &middle1, &_localUpperbound1, 2*offset);
				}
			}
			if(_backwardHeap2->Size() > 0) {
				while(_forwardHeap2->Size() + _backwardHeap2->Size() > 0){
					if(_forwardHeap2->Size() > 0){
						_RoutingStep(_forwardHeap2, _backwardHeap2, true, &middle2, &_localUpperbound2, 2*offset);
					}
					if(_backwardHeap2->Size() > 0){
						_RoutingStep(_backwardHeap2, _forwardHeap2, false, &middle2, &_localUpperbound2, 2*offset);
					}
				}
			}
//			INFO("upperbound1: " << _localUpperbound1 << ", distance1: " << distance1);
//			INFO("upperbound2: " << _localUpperbound2 << ", distance2: " << distance2);

			//No path found for both target nodes?
			if(INT_MAX == _localUpperbound1 && INT_MAX == _localUpperbound2) {
				return INT_MAX;
			}
			if(UINT_MAX == middle1) {
				searchFrom1stStartNode = false;
//				INFO("Next Search will not start from 1st");
			} else {
//				INFO("Next Search will start from 1st");
				searchFrom1stStartNode = true;
			}
			if(UINT_MAX == middle2) {
				searchFrom2ndStartNode = false;
//				INFO("Next Search will not start from 2nd");
			} else {
				searchFrom2ndStartNode = true;
//				INFO("Next Search will start from 2nd");
			}
			//Add distance of segments to current sums
			if(INT_MAX == distance1 || INT_MAX == _localUpperbound1) {
//				INFO("setting distance1 = 0");
				distance1 = 0;
			}
//			distance1 += _localUpperbound1;
			if(INT_MAX == distance2 || INT_MAX == _localUpperbound2) {
//				INFO("Setting distance2 = 0");
				distance2 = 0;
			}
//			distance2 += _localUpperbound2;

//			INFO("distance1: " << distance1 << ", distance2: " << distance2);

			//Was at most one of the two paths not found?
			assert(!(INT_MAX == distance1 && INT_MAX == distance2));

//			INFO("middle1: " << middle1);

			//Unpack paths if they exist
			std::deque<NodeID> temporaryPackedPath1;
			std::deque<NodeID> temporaryPackedPath2;
			if(INT_MAX != _localUpperbound1) {
				_RetrievePackedPathFromHeap(_forwardHeap, _backwardHeap, middle1, temporaryPackedPath1);
//				INFO("temporaryPackedPath1 ends with " << *(temporaryPackedPath1.end()-1) );
			}
//			INFO("middle2: " << middle2);

			if(INT_MAX != _localUpperbound2) {
				_RetrievePackedPathFromHeap(_forwardHeap2, _backwardHeap2, middle2, temporaryPackedPath2);
//                INFO("temporaryPackedPath2 ends with " << *(temporaryPackedPath2.end()-1) );
			}

			//if one of the paths was not found, replace it with the other one.
			if(0 == temporaryPackedPath1.size()) {
//			    INFO("Deleting path 1");
				temporaryPackedPath1.insert(temporaryPackedPath1.end(), temporaryPackedPath2.begin(), temporaryPackedPath2.end());
				_localUpperbound1 = _localUpperbound2;
			}
			if(0 == temporaryPackedPath2.size()) {
//			    INFO("Deleting path 2");
				temporaryPackedPath2.insert(temporaryPackedPath2.end(), temporaryPackedPath1.begin(), temporaryPackedPath1.end());
				_localUpperbound2 = _localUpperbound1;
			}

			assert(0 < temporaryPackedPath1.size() && 0 < temporaryPackedPath2.size());

			//Plug paths together, s.t. end of packed path is begin of temporary packed path
			if(0 < packedPath1.size() && 0 < packedPath2.size() ) {
//			    INFO("Both paths are non-empty");
				if( *(temporaryPackedPath1.begin()) == *(temporaryPackedPath2.begin())) {
//				    INFO("both paths start with the same node:" << *(temporaryPackedPath1.begin()));
					//both new route segments start with the same node, thus one of the packedPath must go.
					assert( (packedPath1.size() == packedPath2.size() ) || (*(packedPath1.end()-1) != *(packedPath2.end()-1)) );
					if( *(packedPath1.end()-1) == *(temporaryPackedPath1.begin())) {
//					    INFO("Deleting packedPath2 that ends with " << *(packedPath2.end()-1) << ", other ends with " << *(packedPath1.end()-1));
						packedPath2.clear();
						packedPath2.insert(packedPath2.end(), packedPath1.begin(), packedPath1.end());
						distance2 = distance1;
//						INFO("packedPath2 now ends with " <<  *(packedPath2.end()-1));
					} else {
//					    INFO("Deleting path1 that ends with " << *(packedPath1.end()-1) << ", other ends with " << *(packedPath2.end()-1));
					    packedPath1.clear();
						packedPath1.insert(packedPath1.end(), packedPath2.begin(), packedPath2.end());
						distance1 = distance2;
//					    INFO("Path1 now ends with " <<  *(packedPath1.end()-1));
					}
				} else  {
					//packed paths 1 and 2 may need to switch.
					if(*(packedPath1.end()-1) != *(temporaryPackedPath1.begin())) {
//					    INFO("Switching");
						packedPath1.swap(packedPath2);
						std::swap(distance1, distance2);
					}
				}
			}
			packedPath1.insert(packedPath1.end(), temporaryPackedPath1.begin(), temporaryPackedPath1.end());
			packedPath2.insert(packedPath2.end(), temporaryPackedPath2.begin(), temporaryPackedPath2.end());

			distance1 += _localUpperbound1;
			distance2 += _localUpperbound2;
		}

//		INFO("length path1: " << distance1);
//		INFO("length path2: " << distance2);
		if(distance1 <= distance2){
			//remove consecutive duplicates
//			std::cout << "unclean 1: ";
//			for(unsigned i = 0; i < packedPath1.size(); ++i)
//				std::cout << packedPath1[i] << " ";
//			std::cout << std::endl;
			_RemoveConsecutiveDuplicatesFromContainer(packedPath1);
//			std::cout << "cleaned 1: ";
//			for(unsigned i = 0; i < packedPath1.size(); ++i)
//				std::cout << packedPath1[i] << " ";
//			std::cout << std::endl;
			_UnpackPath(packedPath1, unpackedPath);
		} else {
//			std::cout << "unclean 2: ";
//			for(unsigned i = 0; i < packedPath2.size(); ++i)
//				std::cout << packedPath2[i] << " ";
//			std::cout << std::endl;
			_RemoveConsecutiveDuplicatesFromContainer(packedPath2);
//			std::cout << "cleaned 2: ";
//			for(unsigned i = 0; i < packedPath2.size(); ++i)
//				std::cout << packedPath2[i] << " ";
//			std::cout << std::endl;
			_UnpackPath(packedPath2, unpackedPath);
		}
//		INFO("Found via route with distance " << std::min(distance1, distance2));
		return std::min(distance1, distance2);
	}

	int ComputeRoute(PhantomNodes & phantomNodes, std::vector<_PathData> & path) {
		int _upperbound = INT_MAX;
		if(!phantomNodes.AtLeastOnePhantomNodeIsUINTMAX())
			return _upperbound;

		InitializeThreadLocalStorageIfNecessary();
		NodeID middle = ( NodeID ) UINT_MAX;
		//insert start and/or target node of start edge
		_forwardHeap->Insert(phantomNodes.startPhantom.edgeBasedNode, -phantomNodes.startPhantom.weight1, phantomNodes.startPhantom.edgeBasedNode);
		//        INFO("a) forw insert " << phantomNodes.startPhantom.edgeBasedNode << ", weight: " << -phantomNodes.startPhantom.weight1);
		if(phantomNodes.startPhantom.isBidirected() ) {
		    //		    INFO("b) forw insert " << phantomNodes.startPhantom.edgeBasedNode+1 << ", weight: " << -phantomNodes.startPhantom.weight2);
			_forwardHeap->Insert(phantomNodes.startPhantom.edgeBasedNode+1, -phantomNodes.startPhantom.weight2, phantomNodes.startPhantom.edgeBasedNode+1);
		}
		//insert start and/or target node of target edge id
		_backwardHeap->Insert(phantomNodes.targetPhantom.edgeBasedNode, phantomNodes.targetPhantom.weight1, phantomNodes.targetPhantom.edgeBasedNode);
		//		INFO("c) back insert " << phantomNodes.targetPhantom.edgeBasedNode << ", weight: " << phantomNodes.targetPhantom.weight1);
		if(phantomNodes.targetPhantom.isBidirected() ) {
			_backwardHeap->Insert(phantomNodes.targetPhantom.edgeBasedNode+1, phantomNodes.targetPhantom.weight2, phantomNodes.targetPhantom.edgeBasedNode+1);
			//			INFO("d) back insert " << phantomNodes.targetPhantom.edgeBasedNode+1 << ", weight: " << phantomNodes.targetPhantom.weight2);
		}
		int offset = (phantomNodes.startPhantom.isBidirected() ? std::max(phantomNodes.startPhantom.weight1, phantomNodes.startPhantom.weight2) : phantomNodes.startPhantom.weight1) ;
		offset += (phantomNodes.targetPhantom.isBidirected() ? std::max(phantomNodes.targetPhantom.weight1, phantomNodes.targetPhantom.weight2) : phantomNodes.targetPhantom.weight1) ;

		while(_forwardHeap->Size() + _backwardHeap->Size() > 0){
			if(_forwardHeap->Size() > 0){
				_RoutingStep(_forwardHeap, _backwardHeap, true, &middle, &_upperbound, 2*offset);
			}
			if(_backwardHeap->Size() > 0){
				_RoutingStep(_backwardHeap, _forwardHeap, false, &middle, &_upperbound, 2*offset);
			}
		}

//		INFO("dist: " << _upperbound);
		if ( _upperbound == INT_MAX ) {
			return _upperbound;
		}
		std::deque<NodeID> packedPath;
		_RetrievePackedPathFromHeap(_forwardHeap, _backwardHeap, middle, packedPath);


		//Setting weights to correspond with that of the actual chosen path
		if(packedPath[0] == phantomNodes.startPhantom.edgeBasedNode && phantomNodes.startPhantom.isBidirected()) {
//            INFO("Setting weight1=" << phantomNodes.startPhantom.weight1 << " to that of weight2=" << phantomNodes.startPhantom.weight2);
            phantomNodes.startPhantom.weight1 = phantomNodes.startPhantom.weight2;
		} else {
//            INFO("Setting weight2=" << phantomNodes.startPhantom.weight2 << " to that of weight1=" << phantomNodes.startPhantom.weight1);
            phantomNodes.startPhantom.weight2 = phantomNodes.startPhantom.weight1;
		}
//		std::cout << "0: ";
//		for(unsigned i = 0; i < packedPath.size(); ++i)
//			std::cout << packedPath[i] << " ";
//		std::cout << std::endl;

		_UnpackPath(packedPath, path);
		return _upperbound;
	}

	inline void FindRoutingStarts(const _Coordinate & start, const _Coordinate & target, PhantomNodes & routingStarts) const {
		nodeHelpDesk->FindRoutingStarts(start, target, routingStarts);
	}

	inline void FindPhantomNodeForCoordinate(const _Coordinate & location, PhantomNode & result) const {
		nodeHelpDesk->FindPhantomNodeForCoordinate(location, result);
	}

	inline NodeID GetNameIDForOriginDestinationNodeID(NodeID s, NodeID t) const {
		if(s == t)
			return 0;

		EdgeID e = _graph->FindEdge(s, t);
		if(e == UINT_MAX)
			e = _graph->FindEdge( t, s );
		if(UINT_MAX == e) {
			return 0;
		}
		assert(e != UINT_MAX);
		const EdgeData ed = _graph->GetEdgeData(e);
		return ed.via;
	}

	inline std::string GetEscapedNameForNameID(const NodeID nameID) const {
		return ((nameID >= _names->size() || nameID == 0) ? std::string("") : HTMLEntitize(_names->at(nameID)));
	}

	inline std::string GetEscapedNameForEdgeBasedEdgeID(const unsigned edgeID) const {
		const unsigned nameID = _graph->GetEdgeData(edgeID).nameID1;
		return GetEscapedNameForNameID(nameID);
	}
private:
	inline void _RetrievePackedPathFromHeap(HeapPtr & _fHeap, HeapPtr & _bHeap, const NodeID middle, std::deque<NodeID>& packedPath) {
		NodeID pathNode = middle;
		if(_fHeap->GetData(pathNode).parent != middle) {
		    do {
		        pathNode = _fHeap->GetData(pathNode).parent;

		        packedPath.push_front(pathNode);
		    }while(pathNode != _fHeap->GetData(pathNode).parent);
		}
		packedPath.push_back(middle);
		pathNode = middle;
		if(_bHeap->GetData(pathNode).parent != middle) {
		    do{
		        pathNode = _bHeap->GetData(pathNode).parent;
		        packedPath.push_back(pathNode);
		    } while (pathNode != _bHeap->GetData(pathNode).parent);
		}
//		std::cout << "unpacking: ";
//		for(std::deque<NodeID>::iterator it = packedPath.begin(); it != packedPath.end(); ++it)
//			std::cout << *it << " ";
//		std::cout << std::endl;
	}


	inline void _RoutingStep(HeapPtr & _forwardHeap, HeapPtr & _backwardHeap, const bool & forwardDirection, NodeID *middle, int *_upperbound, const int edgeBasedOffset) const {
		const NodeID node = _forwardHeap->DeleteMin();
		const int distance = _forwardHeap->GetKey(node);
		//        INFO((forwardDirection ? "[forw]" : "[back]") << " settled node " << node << " at distance " << distance);
		if(_backwardHeap->WasInserted(node) ){
			//        	INFO((forwardDirection ? "[forw]" : "[back]") << " scanned node " << node << " in both directions");
			const int newDistance = _backwardHeap->GetKey(node) + distance;
			if(newDistance < *_upperbound ){
				if(newDistance>=0 ) {
					//        			INFO((forwardDirection ? "[forw]" : "[back]") << " -> node " << node << " is new middle at total distance " << newDistance);
					*middle = node;
					*_upperbound = newDistance;
				} else {
					//        			INFO((forwardDirection ? "[forw]" : "[back]") << " -> ignored " << node << " as new middle at total distance " << newDistance);
				}
			}
		}

		if(distance-edgeBasedOffset > *_upperbound){
			_forwardHeap->DeleteAll();
			return;
		}

		for ( typename GraphT::EdgeIterator edge = _graph->BeginEdges( node ); edge < _graph->EndEdges(node); edge++ ) {
			const EdgeData & data = _graph->GetEdgeData(edge);
			bool backwardDirectionFlag = (!forwardDirection) ? data.forward : data.backward;
			if(backwardDirectionFlag) {
				const NodeID to = _graph->GetTarget(edge);
				const int edgeWeight = data.distance;

				assert( edgeWeight > 0 );

				//Stalling
				if(_forwardHeap->WasInserted( to )) {
					if(_forwardHeap->GetKey( to ) + edgeWeight < distance) {
						return;
					}
				}
			}
		}

		for ( typename GraphT::EdgeIterator edge = _graph->BeginEdges( node ); edge < _graph->EndEdges(node); edge++ ) {
			const EdgeData & data = _graph->GetEdgeData(edge);
			bool forwardDirectionFlag = (forwardDirection ? data.forward : data.backward );
			if(forwardDirectionFlag) {

				const NodeID to = _graph->GetTarget(edge);
				const int edgeWeight = data.distance;

				assert( edgeWeight > 0 );
				const int toDistance = distance + edgeWeight;

				//New Node discovered -> Add to Heap + Node Info Storage
				if ( !_forwardHeap->WasInserted( to ) ) {
					//                    INFO((forwardDirection ? "[forw]" : "[back]") << " scanning edge (" << node << "," << to << ") with distance " << toDistance << ", edge length: " << data.distance);
					_forwardHeap->Insert( to, toDistance, node );
				}
				//Found a shorter Path -> Update distance
				else if ( toDistance < _forwardHeap->GetKey( to ) ) {
					//                    INFO((forwardDirection ? "[forw]" : "[back]") << " decrease and scanning edge (" << node << "," << to << ") from " << _forwardHeap->GetKey(to) << "to " << toDistance << ", edge length: " << data.distance);
					_forwardHeap->GetData( to ).parent = node;
					_forwardHeap->DecreaseKey( to, toDistance );
					//new parent
				}
			}
		}
	}

	inline void _UnpackPath(std::deque<NodeID> & packedPath, std::vector<_PathData> & unpackedPath) const {
		const unsigned sizeOfPackedPath = packedPath.size();
		SimpleStack<std::pair<NodeID, NodeID> > recursionStack(sizeOfPackedPath);

		//We have to push the path in reverse order onto the stack because it's LIFO.
		for(unsigned i = sizeOfPackedPath-1; i > 0; --i){
			recursionStack.push(std::make_pair(packedPath[i-1], packedPath[i]));
		}

		std::pair<NodeID, NodeID> edge;
		while(!recursionStack.empty()) {
			edge = recursionStack.top();
			recursionStack.pop();
//			INFO("Unpacking edge (" << edge.first << "," << edge.second << ")");

			typename GraphT::EdgeIterator smallestEdge = SPECIAL_EDGEID;
			int smallestWeight = INT_MAX;
			for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(edge.first);eit < _graph->EndEdges(edge.first);++eit){
				const int weight = _graph->GetEdgeData(eit).distance;
				if(_graph->GetTarget(eit) == edge.second && weight < smallestWeight && _graph->GetEdgeData(eit).forward){
//				    INFO("1smallest " << eit << ", " << weight);
					smallestEdge = eit;
					smallestWeight = weight;
				}
			}

			if(smallestEdge == SPECIAL_EDGEID){
				for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(edge.second);eit < _graph->EndEdges(edge.second);++eit){
					const int weight = _graph->GetEdgeData(eit).distance;
					if(_graph->GetTarget(eit) == edge.first && weight < smallestWeight && _graph->GetEdgeData(eit).backward){
//	                    INFO("2smallest " << eit << ", " << weight);
						smallestEdge = eit;
						smallestWeight = weight;
					}
				}
			}
			assert(smallestWeight != INT_MAX);

			const EdgeData& ed = _graph->GetEdgeData(smallestEdge);
			if(ed.shortcut) {//unpack
				const NodeID middle = ed.via;
				//again, we need to this in reversed order
				recursionStack.push(std::make_pair(middle, edge.second));
				recursionStack.push(std::make_pair(edge.first, middle));
			} else {
				assert(!ed.shortcut);
				unpackedPath.push_back(_PathData(ed.via, ed.nameID, ed.turnInstruction, ed.distance) );
			}
		}
	}
};
template<class EdgeData, class GraphT> HeapPtr SearchEngine<EdgeData, GraphT>::_forwardHeap;
template<class EdgeData, class GraphT> HeapPtr SearchEngine<EdgeData, GraphT>::_backwardHeap;

template<class EdgeData, class GraphT> HeapPtr SearchEngine<EdgeData, GraphT>::_forwardHeap2;
template<class EdgeData, class GraphT> HeapPtr SearchEngine<EdgeData, GraphT>::_backwardHeap2;

#endif /* SEARCHENGINE_H_ */
