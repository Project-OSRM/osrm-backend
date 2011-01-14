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

#include "BinaryHeap.h"
#include "PhantomNodes.h"
#include "../typedefs.h"

struct _HeapData {
    NodeID parent:31;
    bool stalled:1;
    _HeapData( NodeID p ) : parent(p), stalled(false) { }
};

struct _PathData {
    _PathData(NodeID n, bool t) : node(n), turn(t) { }
    NodeID node:31;
    bool turn:1;
};

typedef BinaryHeap< NodeID, int, int, _HeapData, DenseStorage< NodeID, unsigned > > _Heap;

template<typename EdgeData, typename GraphT, typename NodeHelperT = NodeInformationHelpDesk>
class SearchEngine {
private:
    const GraphT * _graph;
    NodeHelperT * nodeHelpDesk;
    std::vector<string> * _names;
    inline double absDouble(double input) { if(input < 0) return input*(-1); else return input;}
public:
    SearchEngine(GraphT * g, NodeHelperT * nh, vector<string> * n = new vector<string>()) : _graph(g), nodeHelpDesk(nh), _names(n) {}
    ~SearchEngine() {}

    inline const void getNodeInfo(NodeID id, _Coordinate& result) const
    {
        result.lat = nodeHelpDesk->getLatitudeOfNode(id);
        result.lon = nodeHelpDesk->getLongitudeOfNode(id);
    }

    unsigned int numberOfNodes() const {
        return nodeHelpDesk->getNumberOfNodes();
    }

    unsigned int ComputeRoute(PhantomNodes * phantomNodes, vector<_PathData > * path, _Coordinate& startCoord, _Coordinate& targetCoord) {
        bool onSameEdge = false;
        bool onSameEdgeReversed = false;
        bool startReverse = false;
        bool targetReverse = false;

        _Heap * _forwardHeap = new _Heap(nodeHelpDesk->getNumberOfNodes());
        _Heap * _backwardHeap = new _Heap(nodeHelpDesk->getNumberOfNodes());
        NodeID middle = ( NodeID ) 0;
        unsigned int _upperbound = std::numeric_limits<unsigned int>::max();

        if(phantomNodes->startNode1 == UINT_MAX || phantomNodes->startNode2 == UINT_MAX)
            return _upperbound;

        if( (phantomNodes->startNode1 == phantomNodes->targetNode1 && phantomNodes->startNode2 == phantomNodes->targetNode2 ) ||
                (phantomNodes->startNode1 == phantomNodes->targetNode2 && phantomNodes->startNode2 == phantomNodes->targetNode1 ) )
        {
            bool reverse = false;
            EdgeID currentEdge = _graph->FindEdge( phantomNodes->startNode1, phantomNodes->startNode2 );
            if(currentEdge == UINT_MAX){
                currentEdge = _graph->FindEdge( phantomNodes->startNode2, phantomNodes->startNode1 );
                reverse = true;
            }

            if(currentEdge == UINT_MAX){
                delete _forwardHeap;
                delete _backwardHeap;
                return _upperbound;
            }

            if(phantomNodes->startRatio < phantomNodes->targetRatio && _graph->GetEdgeData(currentEdge).forward) {
                onSameEdge = true;
                _upperbound = 10 * ApproximateDistance(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
            } else if(phantomNodes->startRatio > phantomNodes->targetRatio && _graph->GetEdgeData(currentEdge).backward && !reverse)
            {
                onSameEdge = true;
                _upperbound = 10 * ApproximateDistance(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
            } else if(phantomNodes->startRatio < phantomNodes->targetRatio && _graph->GetEdgeData(currentEdge).backward) {
                onSameEdge = true;
                _upperbound = 10 * ApproximateDistance(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
            } else if(phantomNodes->startRatio > phantomNodes->targetRatio && _graph->GetEdgeData(currentEdge).forward && _graph->GetEdgeData(currentEdge).backward) {
                onSameEdge = true;
                _upperbound = 10 * ApproximateDistance(phantomNodes->startCoord.lat, phantomNodes->startCoord.lon, phantomNodes->targetCoord.lat, phantomNodes->targetCoord.lon);
            } else if(phantomNodes->startRatio > phantomNodes->targetRatio) {
                onSameEdgeReversed = true;

                _Coordinate result;
                getNodeInfo(phantomNodes->startNode1, result);
                getNodeInfo(phantomNodes->startNode2, result);

                EdgeWeight w = _graph->GetEdgeData( currentEdge ).distance;
                _forwardHeap->Insert(phantomNodes->startNode2, absDouble(  w*phantomNodes->startRatio), phantomNodes->startNode2);
                _backwardHeap->Insert(phantomNodes->startNode1, absDouble(  w-w*phantomNodes->targetRatio), phantomNodes->startNode1);
            }

        } else if(phantomNodes->startNode1 != UINT_MAX)
        {
            EdgeID edge = _graph->FindEdge( phantomNodes->startNode1, phantomNodes->startNode2);
            if(edge == UINT_MAX){
                edge = _graph->FindEdge( phantomNodes->startNode2, phantomNodes->startNode1 );
                startReverse = true;
            }
            if(edge == UINT_MAX){
                delete _forwardHeap;
                delete _backwardHeap;
                return _upperbound;
            }

            EdgeWeight w = _graph->GetEdgeData( edge ).distance;
            if( (_graph->GetEdgeData( edge ).backward && !startReverse) || (_graph->GetEdgeData( edge ).forward && startReverse) )
                _forwardHeap->Insert(phantomNodes->startNode1, absDouble(  w*phantomNodes->startRatio), phantomNodes->startNode1);
            if( (_graph->GetEdgeData( edge ).backward && startReverse) || (_graph->GetEdgeData( edge ).forward && !startReverse) )
                _forwardHeap->Insert(phantomNodes->startNode2, absDouble(w-w*phantomNodes->startRatio), phantomNodes->startNode2);
        }
        if(phantomNodes->targetNode1 != UINT_MAX && !onSameEdgeReversed)
        {
            EdgeID edge = _graph->FindEdge( phantomNodes->targetNode1, phantomNodes->targetNode2);
            if(edge == UINT_MAX){
                edge = _graph->FindEdge( phantomNodes->targetNode2, phantomNodes->targetNode1 );
                targetReverse = true;
            }
            if(edge == UINT_MAX){
                delete _forwardHeap;
                delete _backwardHeap;
                return _upperbound;
            }

            EdgeWeight w = _graph->GetEdgeData( edge ).distance;
            if( (_graph->GetEdgeData( edge ).backward && !targetReverse) || (_graph->GetEdgeData( edge ).forward && targetReverse) )
                _backwardHeap->Insert(phantomNodes->targetNode2, absDouble(  w*phantomNodes->targetRatio), phantomNodes->targetNode2);
            if( (_graph->GetEdgeData( edge ).backward && targetReverse) || (_graph->GetEdgeData( edge ).forward && !targetReverse) )
                _backwardHeap->Insert(phantomNodes->targetNode1, absDouble(w-w*phantomNodes->startRatio), phantomNodes->targetNode1);
        }

        NodeID sourceHeapNode = 0;
        NodeID targetHeapNode = 0;
        if(onSameEdgeReversed) {
            sourceHeapNode = _forwardHeap->Min();
            targetHeapNode = _backwardHeap->Min();
        }
        while(_forwardHeap->Size() + _backwardHeap->Size() > 0)
        {
            if ( _forwardHeap->Size() > 0 ) {
                _RoutingStep( _forwardHeap, _backwardHeap, true, &middle, &_upperbound );
            }
            if ( _backwardHeap->Size() > 0 ) {
                _RoutingStep( _backwardHeap, _forwardHeap, false, &middle, &_upperbound );
            }
        }

        if ( _upperbound == std::numeric_limits< unsigned int >::max() || onSameEdge )
        {
            delete _forwardHeap;
            delete _backwardHeap;
            return _upperbound;
        }

        NodeID pathNode = middle;
        deque< NodeID > packedPath;

        while ( onSameEdgeReversed ? pathNode != sourceHeapNode : pathNode != phantomNodes->startNode1 && pathNode != phantomNodes->startNode2 ) {
            pathNode = _forwardHeap->GetData( pathNode ).parent;
            packedPath.push_front( pathNode );
        }
//        NodeID realStart = pathNode;
        packedPath.push_back( middle );
        pathNode = middle;

        while ( onSameEdgeReversed ? pathNode != targetHeapNode : pathNode != phantomNodes->targetNode2 && pathNode != phantomNodes->targetNode1 ){
            pathNode = _backwardHeap->GetData( pathNode ).parent;
            packedPath.push_back( pathNode );
        }


        path->push_back( _PathData(packedPath[0], false) );
        {
            for(deque<NodeID>::size_type i = 0; i < packedPath.size()-1; i++)
            {
                _UnpackEdge(packedPath[i], packedPath[i+1], path);
            }
        }

        packedPath.clear();
        delete _forwardHeap;
        delete _backwardHeap;

        return _upperbound/10;
    }

    unsigned int ComputeDistanceBetweenNodes(NodeID start, NodeID target)
    {
        _Heap * _forwardHeap = new _Heap(_graph->GetNumberOfNodes());
        _Heap * _backwardHeap = new _Heap(_graph->GetNumberOfNodes());
        NodeID middle = ( NodeID ) 0;
        unsigned int _upperbound = std::numeric_limits<unsigned int>::max();

        _forwardHeap->Insert(start, 0, start);
        _backwardHeap->Insert(target, 0, target);

        while(_forwardHeap->Size() + _backwardHeap->Size() > 0)
        {
            if ( _forwardHeap->Size() > 0 ) {
                _RoutingStep( _forwardHeap, _backwardHeap, true, &middle, &_upperbound );
            }
            if ( _backwardHeap->Size() > 0 ) {
                _RoutingStep( _backwardHeap, _forwardHeap, false, &middle, &_upperbound );
            }
        }
        delete _forwardHeap;
        delete _backwardHeap;
        return _upperbound;
    }

    inline unsigned int findNearestNodeForLatLon(const _Coordinate& coord, _Coordinate& result) const
    {
        nodeHelpDesk->findNearestNodeCoordForLatLon( coord, result );
        return 0;

    }

    inline bool FindRoutingStarts(const _Coordinate start, const _Coordinate target, PhantomNodes * routingStarts)
    {
        nodeHelpDesk->FindRoutingStarts(start, target, routingStarts);
        return true;
    }

    inline NodeID GetNameIDForOriginDestinationNodeID(NodeID s, NodeID t) const {
        assert(s!=t);
        EdgeID e = _graph->FindEdge( s, t );
        if(e == UINT_MAX)
            e = _graph->FindEdge( t, s );
        assert(e != UINT_MAX);
        const EdgeData ed = _graph->GetEdgeData(e);
        return ed.middleName.nameID;
    }

    inline std::string& GetNameForNameID(const NodeID nameID) const {
        assert(nameID < _names->size());
        return _names->at(nameID);
    }

    inline short GetTypeOfEdgeForOriginDestinationNodeID(NodeID s, NodeID t) const {
        assert(s!=t);
        EdgeID e = _graph->FindEdge( s, t );
        if(e == UINT_MAX)
            e = _graph->FindEdge( t, s );
        assert(e != UINT_MAX);
        const EdgeData ed = _graph->GetEdgeData(e);
        return ed.type;
    }

    inline void RegisterThread(const unsigned k, const unsigned v) {
    	nodeHelpDesk->RegisterThread(k,v);
    }
private:

    void _RoutingStep(_Heap * _forwardHeap, _Heap *_backwardHeap, const bool& forwardDirection, NodeID * middle, unsigned int * _upperbound)
    {
        const NodeID node = _forwardHeap->DeleteMin();
        const unsigned int distance = _forwardHeap->GetKey( node );
        if ( _backwardHeap->WasInserted( node ) ) {
            const unsigned int newDistance = _backwardHeap->GetKey( node ) + distance;
            if ( newDistance < *_upperbound ) {
                *middle = node;
                *_upperbound = newDistance;
            }
        }
        if ( distance > *_upperbound ) {
            _forwardHeap->DeleteAll();
            return;
        }
        for ( typename GraphT::EdgeIterator edge = _graph->BeginEdges( node ); edge < _graph->EndEdges(node); edge++ ) {
            const NodeID to = _graph->GetTarget(edge);
            const int edgeWeight = _graph->GetEdgeData(edge).distance;

            assert( edgeWeight > 0 );
            const int toDistance = distance + edgeWeight;

            if(forwardDirection ? _graph->GetEdgeData(edge).forward : _graph->GetEdgeData(edge).backward )
            {
                //New Node discovered -> Add to Heap + Node Info Storage
                if ( !_forwardHeap->WasInserted( to ) )
                {
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

    bool _UnpackEdge( const NodeID source, const NodeID target, std::vector< _PathData >* path ) {
        assert(source != target);
        //find edge first.
        bool forward = true;
        typename GraphT::EdgeIterator smallestEdge = SPECIAL_EDGEID;
        EdgeWeight smallestWeight = UINT_MAX;
        for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(source); eit < _graph->EndEdges(source); eit++)
        {
            const EdgeWeight weight = _graph->GetEdgeData(eit).distance;
            {
                if(_graph->GetTarget(eit) == target && weight < smallestWeight && _graph->GetEdgeData(eit).forward)
                {
                    smallestEdge = eit; smallestWeight = weight;
                }
            }
        }
        if(smallestEdge == SPECIAL_EDGEID)
        {
            for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(target); eit < _graph->EndEdges(target); eit++)
            {
                const EdgeWeight weight = _graph->GetEdgeData(eit).distance;
                {
                    if(_graph->GetTarget(eit) == source && weight < smallestWeight && _graph->GetEdgeData(eit).backward)
                    {
                        smallestEdge = eit; smallestWeight = weight;
                        forward = false;
                    }
                }
            }
        }

        assert(smallestWeight != SPECIAL_EDGEID); //no edge found. This should not happen at all!

        const EdgeData ed = _graph->GetEdgeData(smallestEdge);
        if(ed.shortcut)
        {//unpack
            const NodeID middle = ed.middleName.middle;
            _UnpackEdge(source, middle, path);
            _UnpackEdge(middle, target, path);
            return false;
        } else {
            assert(!ed.shortcut);
            path->push_back(_PathData(target, (forward ? ed.forwardTurn : ed.backwardTurn) ) );
            return true;
        }
    }
};

#endif /* SEARCHENGINE_H_ */
