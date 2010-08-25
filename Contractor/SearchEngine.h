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
#include "../typedefs.h"

struct _HeapData {
    NodeID parent;
    _HeapData( NodeID p ) : parent(p) { }
};

typedef BinaryHeap< NodeID, int, int, _HeapData, DenseStorage< NodeID, unsigned > > _Heap;

template<typename EdgeData, typename GraphT, typename KDTST = NodeInformationHelpDesk>
class SearchEngine {
private:
    const GraphT * _graph;
public:
    SearchEngine(GraphT * g, KDTST * k) : _graph(g), nodeHelpDesk(k) {}
    ~SearchEngine() {}

    const void getNodeInfo(NodeID id, NodeInfo * info) const
    {
    	nodeHelpDesk->getExternalNodeInfo(id, info);
    }

    unsigned int numberOfNodes() const
    {
        return nodeHelpDesk->getNumberOfNodes();
    }

    unsigned int ComputeRoute(NodeID start, NodeID target, vector<NodeID> * path)
    {
        _Heap * _forwardHeap = new _Heap(nodeHelpDesk->getNumberOfNodes());
        _Heap * _backwardHeap = new _Heap(nodeHelpDesk->getNumberOfNodes());
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

        if ( _upperbound == std::numeric_limits< unsigned int >::max() )
            return _upperbound;

        NodeID pathNode = middle;
        NodeID unpackEndNode = start;
        deque< NodeID > packedPath;

        while ( pathNode != unpackEndNode ) {
            pathNode = _forwardHeap->GetData( pathNode ).parent;
            packedPath.push_front( pathNode );
        }

        packedPath.push_back( middle );

        pathNode = middle;
        unpackEndNode = target;

        while ( pathNode != unpackEndNode ) {
            pathNode = _backwardHeap->GetData( pathNode ).parent;
            packedPath.push_back( pathNode );
        }

        // push start node explicitely
        path->push_back(packedPath[0]);
        for(deque<NodeID>::size_type i = 0; i < packedPath.size()-1; i++)
        {
            _UnpackEdge(packedPath[i], packedPath[i+1], path);
        }

        packedPath.clear();
        delete _forwardHeap;
        delete _backwardHeap;

        return _upperbound/10;
    }

    unsigned int findNearestNodeForLatLon(const int lat, const int lon, NodeCoords<NodeID> * data) const
    {
        return nodeHelpDesk->findNearestNodeIDForLatLon(  lat,  lon, data);
    }
private:
    KDTST * nodeHelpDesk;

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

    bool _UnpackEdge( const NodeID source, const NodeID target, std::vector< NodeID >* path ) {
        assert(source != target);
        //find edge first.
        typename GraphT::EdgeIterator smallestEdge = SPECIAL_EDGEID;
        EdgeWeight smallestWeight = UINT_MAX;
        for(typename GraphT::EdgeIterator eit = _graph->BeginEdges(source); eit < _graph->EndEdges(source); eit++)
        {
            //const NodeID target = GetTarget(edge);
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
                //const NodeID target = GetTarget(edge);
                const EdgeWeight weight = _graph->GetEdgeData(eit).distance;
                {
                    if(_graph->GetTarget(eit) == source && weight < smallestWeight && _graph->GetEdgeData(eit).backward)
                    {
                        smallestEdge = eit; smallestWeight = weight;
                    }
                }
            }
        }

        assert(smallestWeight != SPECIAL_EDGEID); //no edge found. This should not happen at all!

        const EdgeData ed = _graph->GetEdgeData(smallestEdge);
        if(ed.shortcut)
        {//unpack
            const NodeID middle = ed.middle;
            _UnpackEdge(source, middle, path);
            _UnpackEdge(middle, target, path);
            return false;
        } else {
            assert(!ed.shortcut);
            path->push_back(target);
            return true;
        }
    }
};

#endif /* SEARCHENGINE_H_ */
