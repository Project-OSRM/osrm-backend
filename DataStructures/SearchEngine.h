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

#include <boost/thread.hpp>

#include "BinaryHeap.h"
#include "PhantomNodes.h"
#include "../Util/StringUtil.h"
#include "../typedefs.h"

struct _HeapData {
	NodeID parent;
	_HeapData( NodeID p ) : parent(p) { }
};

struct _Statistics {
	_Statistics () : insertedNodes(0), stalledNodes(0), meetingNodes(0), deleteMins(0), decreasedNodes(0), oqf(0), eqf(0), df(0), preprocTime(0) {};
	void Reset() {
		insertedNodes = 0;
		stalledNodes = 0;
		meetingNodes = 0;
		deleteMins = 0;
		decreasedNodes = 0;
	}
	unsigned insertedNodes;
	unsigned stalledNodes;
	unsigned meetingNodes;
	unsigned deleteMins;
	unsigned decreasedNodes;
	unsigned oqf;
	unsigned eqf;
	unsigned df;
	double preprocTime;
};

typedef boost::thread_specific_ptr<BinaryHeap< NodeID, NodeID, int, _HeapData > > HeapPtr;

template<class EdgeData, class GraphT>
class SearchEngine {
private:
    const GraphT * _graph;
    NodeInformationHelpDesk * nodeHelpDesk;
	std::vector<string> * _names;
	static HeapPtr _forwardHeap;
	static HeapPtr _backwardHeap;
	inline double absDouble(double input) { if(input < 0) return input*(-1); else return input;}
public:
	SearchEngine(GraphT * g, NodeInformationHelpDesk * nh, vector<string> * n = new vector<string>()) : _graph(g), nodeHelpDesk(nh), _names(n) {}
	~SearchEngine() {}

	inline const void GetCoordinatesForNodeID(NodeID id, _Coordinate& result) const {
		result.lat = nodeHelpDesk->getLatitudeOfNode(id);
		result.lon = nodeHelpDesk->getLongitudeOfNode(id);
	}

    inline void InitializeThreadLocalStorageIfNecessary() {
        if(!_forwardHeap.get())
            _forwardHeap.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData >(nodeHelpDesk->getNumberOfNodes()));
        else
            _forwardHeap->Clear();

        if(!_backwardHeap.get())
            _backwardHeap.reset(new BinaryHeap< NodeID, NodeID, int, _HeapData >(nodeHelpDesk->getNumberOfNodes()));
        else
            _backwardHeap->Clear();
    }

    int ComputeRoute(PhantomNodes & phantomNodes, vector<_PathData> & path) {
        int _upperbound = INT_MAX;
        if(!phantomNodes.AtLeastOnePhantomNodeIsUINTMAX())
            return _upperbound;

        InitializeThreadLocalStorageIfNecessary();
        NodeID middle = ( NodeID ) UINT_MAX;
        if(phantomNodes.PhantomsAreOnSameNodeBasedEdge()){
            //TODO: Hier behandeln, dass Start und Ziel auf der gleichen Originalkante liegen
            INFO("TODO: Start and target are on same edge")
            return _upperbound;
        }
        //insert start and/or target node of start edge
        _forwardHeap->Insert(phantomNodes.startPhantom.edgeBasedNode, -phantomNodes.startPhantom.weight1, phantomNodes.startPhantom.edgeBasedNode);
//        INFO("[FORW] Inserting node " << phantomNodes.startPhantom.edgeBasedNode << " at distance " << -phantomNodes.startPhantom.weight1);
        if(phantomNodes.startPhantom.isBidirected) {
            _forwardHeap->Insert(phantomNodes.startPhantom.edgeBasedNode+1, -phantomNodes.startPhantom.weight2, phantomNodes.startPhantom.edgeBasedNode+1);
//            INFO("[FORW] Inserting node " << phantomNodes.startPhantom.edgeBasedNode+1 << " at distance " << -phantomNodes.startPhantom.weight2);
        }

        //insert start and/or target node of target edge id
        _backwardHeap->Insert(phantomNodes.targetPhantom.edgeBasedNode, -phantomNodes.targetPhantom.weight1, phantomNodes.targetPhantom.edgeBasedNode);
//        INFO("[BACK] Inserting node " << phantomNodes.targetPhantom.edgeBasedNode << " at distance " << -phantomNodes.targetPhantom.weight1);
        if(phantomNodes.targetPhantom.isBidirected) {
            _backwardHeap->Insert(phantomNodes.targetPhantom.edgeBasedNode+1, -phantomNodes.targetPhantom.weight2, phantomNodes.targetPhantom.edgeBasedNode+1);
//            INFO("[BACK] Inserting node " << phantomNodes.targetPhantom.edgeBasedNode+1 << " at distance " << -phantomNodes.targetPhantom.weight2);
        }

        while(_forwardHeap->Size() + _backwardHeap->Size() > 0){
            if(_forwardHeap->Size() > 0){
                _RoutingStep(_forwardHeap, _backwardHeap, true, &middle, &_upperbound);
            }
            if(_backwardHeap->Size() > 0){
                _RoutingStep(_backwardHeap, _forwardHeap, false, &middle, &_upperbound);
            }
        }
//        INFO("bidirectional search iteration ended: " << _forwardHeap->Size() << "," << _backwardHeap->Size() << ", dist: " << _upperbound);

        if ( _upperbound == INT_MAX ) {
			return _upperbound;
		}
        NodeID pathNode = middle;
        deque<NodeID> packedPath;
        while(phantomNodes.startPhantom.edgeBasedNode != pathNode && (!phantomNodes.startPhantom.isBidirected || phantomNodes.startPhantom.edgeBasedNode+1 != pathNode) ) {
            pathNode = _forwardHeap->GetData(pathNode).parent;
            packedPath.push_front(pathNode);
        }
//        INFO("Finished getting packed forward path: " << packedPath.size());
        packedPath.push_back(middle);
        pathNode = middle;
        while(phantomNodes.targetPhantom.edgeBasedNode != pathNode && (!phantomNodes.targetPhantom.isBidirected || phantomNodes.targetPhantom.edgeBasedNode+1 != pathNode)) {
            pathNode = _backwardHeap->GetData(pathNode).parent;
            packedPath.push_back(pathNode);
        }
//        INFO("Finished getting packed path: " << packedPath.size());
        for(deque<NodeID>::size_type i = 0;i < packedPath.size() - 1;i++){
            _UnpackEdge(packedPath[i], packedPath[i + 1], path);
        }
        return _upperbound;
    }

    unsigned int ComputeDistanceBetweenNodes(NodeID start, NodeID target) {
        InitializeThreadLocalStorageIfNecessary();
        NodeID middle(UINT_MAX);
        int _upperbound = INT_MAX;
        _forwardHeap->Insert(start, 0, start);
        _backwardHeap->Insert(target, 0, target);
        while(_forwardHeap->Size() + _backwardHeap->Size() > 0){
            if(_forwardHeap->Size() > 0){
                _RoutingStep(_forwardHeap, _backwardHeap, true, &middle, &_upperbound);
            }
            if(_backwardHeap->Size() > 0){
                _RoutingStep(_backwardHeap, _forwardHeap, false, &middle, &_upperbound);
            }
        }
        return _upperbound;
    }

    unsigned int ComputeDistanceBetweenNodesWithStats(NodeID start, NodeID target, _Statistics & stats) {
        InitializeThreadLocalStorageIfNecessary();
        NodeID middle(UINT_MAX);
        int _upperbound = INT_MAX;
        _forwardHeap->Insert(start, 0, start);
        _backwardHeap->Insert(target, 0, target);
        stats.insertedNodes += 2;
        while(_forwardHeap->Size() + _backwardHeap->Size() > 0){
            if(_forwardHeap->Size() > 0){
                _RoutingStepWithStats(_forwardHeap, _backwardHeap, true, &middle, &_upperbound, stats);
            }
            if(_backwardHeap->Size() > 0){
                _RoutingStepWithStats(_backwardHeap, _forwardHeap, false, &middle, &_upperbound, stats);
            }
        }

        return _upperbound;
    }

    inline bool FindRoutingStarts(const _Coordinate & start, const _Coordinate & target, PhantomNodes & routingStarts) {
        nodeHelpDesk->FindRoutingStarts(start, target, routingStarts);
        return true;
    }

    inline bool FindPhantomNodeForCoordinate(const _Coordinate & location, PhantomNode & result) {
        return nodeHelpDesk->FindPhantomNodeForCoordinate(location, result);
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
    inline void _RoutingStep(HeapPtr & _forwardHeap, HeapPtr & _backwardHeap, const bool & forwardDirection, NodeID *middle, int *_upperbound) {
        const NodeID node = _forwardHeap->DeleteMin();
        const int distance = _forwardHeap->GetKey(node);
//        INFO((forwardDirection ? "[FORW]" : "[BACK]") << " settling " << node << " with distance " << distance);
        if(_backwardHeap->WasInserted(node)){
            const int newDistance = _backwardHeap->GetKey(node) + distance;
            if(newDistance < *_upperbound){
                *middle = node;
                *_upperbound = newDistance;
            }
        }

        if(distance > *_upperbound){
            _forwardHeap->DeleteAll();
            return;
        }
        for ( typename GraphT::EdgeIterator edge = _graph->BeginEdges( node ); edge < _graph->EndEdges(node); edge++ ) {
			const NodeID to = _graph->GetTarget(edge);
			const int edgeWeight = _graph->GetEdgeData(edge).distance;

			assert( edgeWeight > 0 );

			//Stalling
			bool backwardDirectionFlag = (!forwardDirection) ? _graph->GetEdgeData(edge).forward : _graph->GetEdgeData(edge).backward;
			if(_forwardHeap->WasInserted( to )) {
				if(backwardDirectionFlag) {
					if(_forwardHeap->GetKey( to ) + edgeWeight < distance) {
						return;
					}
				}
			}
		}

        for ( typename GraphT::EdgeIterator edge = _graph->BeginEdges( node ); edge < _graph->EndEdges(node); edge++ ) {
			const NodeID to = _graph->GetTarget(edge);
			const int edgeWeight = _graph->GetEdgeData(edge).distance;

			assert( edgeWeight > 0 );
			bool forwardDirectionFlag = (forwardDirection ? _graph->GetEdgeData(edge).forward : _graph->GetEdgeData(edge).backward );
			if(forwardDirectionFlag) {
	            const int toDistance = distance + edgeWeight;
//	            INFO((forwardDirection ? "[FORW]" : "[BACK]") << " relaxing edge (" << node << "," << to << ") with distance " << toDistance << "=" << distance << "+" << edgeWeight);

				//New Node discovered -> Add to Heap + Node Info Storage
				if ( !_forwardHeap->WasInserted( to ) ) {
//				    INFO((forwardDirection ? "[FORW]" : "[BACK]") << " inserting node " << to << " at distance " << toDistance);
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

    inline void _RoutingStepWithStats(HeapPtr & _forwardHeap, HeapPtr & _backwardHeap, const bool & forwardDirection, NodeID *middle, int *_upperbound, _Statistics & stats) {
        const NodeID node = _forwardHeap->DeleteMin();
        stats.deleteMins++;
        const unsigned int distance = _forwardHeap->GetKey(node);
        if(_backwardHeap->WasInserted(node)){
            const unsigned int newDistance = _backwardHeap->GetKey(node) + distance;
            if(newDistance < *_upperbound){
                *middle = node;
                *_upperbound = newDistance;
            }
        }

        if(distance > *_upperbound) {
            stats.meetingNodes++;
            _forwardHeap->DeleteAll();
            return;
        }
        for ( typename GraphT::EdgeIterator edge = _graph->BeginEdges( node ); edge < _graph->EndEdges(node); edge++ ) {
			const EdgeData& ed = _graph->GetEdgeData(edge);
			const NodeID to = _graph->GetTarget(edge);
			const EdgeWeight edgeWeight = ed.distance;

			assert( edgeWeight > 0 );
			const int toDistance = distance + edgeWeight;

			//Stalling
			if(_forwardHeap->WasInserted( to )) {
				if(!forwardDirection ? ed.forward : ed.backward) {
					if(_forwardHeap->GetKey( to ) + edgeWeight < distance) {
						stats.stalledNodes++;
						return;
					}
				}
			}

			if(forwardDirection ? ed.forward : ed.backward ) {
				//New Node discovered -> Add to Heap + Node Info Storage
				if ( !_forwardHeap->WasInserted( to ) ) {
					_forwardHeap->Insert( to, toDistance, node );
					stats.insertedNodes++;
				}
				//Found a shorter Path -> Update distance
				else if ( toDistance < _forwardHeap->GetKey( to ) ) {
					_forwardHeap->GetData( to ).parent = node;
					_forwardHeap->DecreaseKey( to, toDistance );
					stats.decreasedNodes++;
					//new parent
				}
			}
		}
    }

    inline bool _UnpackEdge(const NodeID source, const NodeID target, std::vector<_PathData> & path) {
        assert(source != target);
        //find edge first.
        typename GraphT::EdgeIterator smallestEdge = SPECIAL_EDGEID;
        int smallestWeight = INT_MAX;
        for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(source);eit < _graph->EndEdges(source);eit++){
            const int weight = _graph->GetEdgeData(eit).distance;
            if(_graph->GetTarget(eit) == target && weight < smallestWeight && _graph->GetEdgeData(eit).forward){
                smallestEdge = eit;
                smallestWeight = weight;
            }
        }

        if(smallestEdge == SPECIAL_EDGEID){
            for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(target);eit < _graph->EndEdges(target);eit++){
                const int weight = _graph->GetEdgeData(eit).distance;
                if(_graph->GetTarget(eit) == source && weight < smallestWeight && _graph->GetEdgeData(eit).backward){
                    smallestEdge = eit;
                    smallestWeight = weight;
                }
            }

        }

        assert(smallestWeight != INT_MAX);

		const EdgeData& ed = _graph->GetEdgeData(smallestEdge);
//		INFO( (ed.shortcut ? "SHRT: " : "ORIG: ") << ed.distance << "," << ed.via);
		if(ed.shortcut) {//unpack
			const NodeID middle = ed.via;
			_UnpackEdge(source, middle, path);
			_UnpackEdge(middle, target, path);
			return false;
		} else {
			assert(!ed.shortcut);
			path.push_back(_PathData(ed.via, ed.nameID1, ed.turnInstruction, ed.distance) );
			return true;
		}
	}
};
template<class EdgeData, class GraphT> HeapPtr SearchEngine<EdgeData, GraphT>::_forwardHeap;
template<class EdgeData, class GraphT> HeapPtr SearchEngine<EdgeData, GraphT>::_backwardHeap;

#endif /* SEARCHENGINE_H_ */
