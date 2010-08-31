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

#ifndef CONTRACTIONCLEANUP_H_INCLUDED
#define CONTRACTIONCLEANUP_H_INCLUDED

#ifdef _GLIBCXX_PARALLEL
#include <parallel/algorithm>
#else
#include <algorithm>
#endif
#include <sys/time.h>
#include "Contractor.h"

#ifdef _OPENMP
#include <omp.h>
#endif

class ContractionCleanup {
private:

    struct _HeapData {
        NodeID parent;
        _HeapData( NodeID p ) {
            parent = p;
        }
    };
    typedef BinaryHeap< NodeID, NodeID, int, _HeapData > _Heap;

    struct _ThreadData {
        _Heap* _heapForward;
        _Heap* _heapBackward;
        _ThreadData( NodeID nodes ) {
            _heapBackward = new _Heap(nodes);
            _heapForward = new _Heap(nodes);
        }
        ~_ThreadData()
        {
            delete _heapBackward;
            delete _heapForward;
        }
    };

public:

    struct Edge {
        NodeID source;
        NodeID target;
        struct EdgeData {
            int distance : 29;
            bool shortcut : 1;
            bool forward : 1;
            bool backward : 1;
            NodeID middle;
        } data;

        //sorts by source and other attributes
        static bool CompareBySource( const Edge& left, const Edge& right ) {
            if ( left.source != right.source )
                return left.source < right.source;
            int l = ( left.data.forward ? -1 : 0 ) + ( left.data.backward ? -1 : 0 );
            int r = ( right.data.forward ? -1 : 0 ) + ( right.data.backward ? -1 : 0 );
            if ( l != r )
                return l < r;
            if ( left.target != right.target )
                return left.target < right.target;
            return left.data.distance < right.data.distance;
        }

        bool operator== ( const Edge& right ) const {
            return ( source == right.source && target == right.target && data.distance == right.data.distance && data.shortcut == right.data.shortcut && data.forward == right.data.forward && data.backward == right.data.backward && data.middle == right.data.middle );
        }
    };

    ContractionCleanup( int numNodes, const std::vector< Edge >& edges ) {
        _graph = edges;
        _numNodes = numNodes;
    }

    ~ContractionCleanup() {

    }

    void Run() {

        double time = _Timestamp();

        RemoveUselessShortcuts();

        time = _Timestamp() - time;
        cout << "Postprocessing Time: " << time << " s" << endl;
    }

    template< class EdgeT >
    void GetData( std::vector< EdgeT >& edges ) {
        for ( int edge = 0, endEdges = ( int ) _graph.size(); edge != endEdges; ++edge ) {
            EdgeT newEdge;
            newEdge.source = _graph[edge].source;
            newEdge.target = _graph[edge].target;

            newEdge.data.distance = _graph[edge].data.distance;
            newEdge.data.shortcut = _graph[edge].data.shortcut;
            if ( newEdge.data.shortcut )
                newEdge.data.middle = _graph[edge].data.middle;
            newEdge.data.forward = _graph[edge].data.forward;
            newEdge.data.backward = _graph[edge].data.backward;
            edges.push_back( newEdge );
        }
#ifdef _GLIBCXX_PARALLEL
        __gnu_parallel::sort( edges.begin(), edges.end() );
#else
        sort( edges.begin(), edges.end() );
#endif
    }

private:

    class AllowForwardEdge {
    public:
        bool operator()( const Edge& data ) const {
            return data.data.forward;
        }
    };

    class AllowBackwardEdge {
    public:
        bool operator()( const Edge& data ) const {
            return data.data.backward;
        }
    };

    double _Timestamp() {
        struct timeval tp;
        gettimeofday(&tp, NULL);
        return double(tp.tv_sec) + tp.tv_usec / 1000000.;
    }

    void BuildOutgoingGraph() {
        //sort edges by source
#ifdef _GLIBCXX_PARALLEL
        __gnu_parallel::sort( _graph.begin(), _graph.end(), Edge::CompareBySource );
#else
        sort( _graph.begin(), _graph.end(), Edge::CompareBySource );
#endif
        _firstEdge.resize( _numNodes + 1 );
        _firstEdge[0] = 0;
        for ( NodeID i = 0, node = 0; i < ( NodeID ) _graph.size(); i++ ) {
            while ( _graph[i].source != node )
                _firstEdge[++node] = i;
            if ( i == ( NodeID ) _graph.size() - 1 )
                while ( node < _numNodes )
                    _firstEdge[++node] = ( int ) _graph.size();
        }
    }

    void RemoveUselessShortcuts() {
        int maxThreads = omp_get_max_threads();
        std::vector < _ThreadData* > threadData;
        for ( int threadNum = 0; threadNum < maxThreads; ++threadNum ) {
            threadData.push_back( new _ThreadData( _numNodes ) );
        }

        cout << "Scanning for useless shortcuts" << endl;
        BuildOutgoingGraph();
#pragma omp parallel for
        for ( unsigned i = 0; i < ( unsigned ) _graph.size(); i++ ) {
            for ( unsigned edge = _firstEdge[_graph[i].source]; edge < _firstEdge[_graph[i].source + 1]; ++edge ) {
                if ( edge == i )
                    continue;
                if ( _graph[edge].target != _graph[i].target )
                    continue;
                if ( _graph[edge].data.distance < _graph[i].data.distance )
                    continue;

                _graph[edge].data.forward &= !_graph[i].data.forward;
                _graph[edge].data.backward &= !_graph[i].data.backward;
            }

            if ( !_graph[i].data.forward && !_graph[i].data.backward )
                continue;

            //only remove shortcuts
            if ( !_graph[i].data.shortcut )
                continue;

            if ( _graph[i].data.forward ) {
                int result = _ComputeDistance( _graph[i].source, _graph[i].target, threadData[omp_get_thread_num()] );
                if ( result < _graph[i].data.distance ) {
                    _graph[i].data.forward = false;
                    Contractor::Witness temp;
                    temp.source = _graph[i].source;
                    temp.target = _graph[i].target;
                    temp.middle = _graph[i].data.middle;
                }
            }
            if ( _graph[i].data.backward ) {
                int result = _ComputeDistance( _graph[i].target, _graph[i].source, threadData[omp_get_thread_num()] );

                if ( result < _graph[i].data.distance ) {
                    _graph[i].data.backward = false;
                    Contractor::Witness temp;
                    temp.source = _graph[i].target;
                    temp.target = _graph[i].source;
                    temp.middle = _graph[i].data.middle;
                }
            }
        }

        cout << "Removing edges" << endl;
        int usefull = 0;
        for ( int i = 0; i < ( int ) _graph.size(); i++ ) {
            if ( !_graph[i].data.forward && !_graph[i].data.backward && _graph[i].data.shortcut )
                continue;
            _graph[usefull] = _graph[i];
            usefull++;
        }
        cout << "Removed " << _graph.size() - usefull << " useless shortcuts" << endl;
        _graph.resize( usefull );
        for ( int threadNum = 0; threadNum < maxThreads; ++threadNum ) {
            delete threadData[threadNum];
        }
    }

    template< class EdgeAllowed, class StallEdgeAllowed > void _ComputeStep( _Heap* heapForward, _Heap* heapBackward, const EdgeAllowed& edgeAllowed, const StallEdgeAllowed& stallEdgeAllowed, NodeID* middle, int* targetDistance ) {

        const NodeID node = heapForward->DeleteMin();
        const int distance = heapForward->GetKey( node );

        if ( heapBackward->WasInserted( node ) ) {
            const int newDistance = heapBackward->GetKey( node ) + distance;
            if ( newDistance < *targetDistance ) {
                *middle = node;
                *targetDistance = newDistance;
            }
        }

        if ( distance > *targetDistance ) {
            heapForward->DeleteAll();
            return;
        }
        for ( int edge = _firstEdge[node], endEdges = _firstEdge[node + 1]; edge != endEdges; ++edge ) {
            const NodeID to = _graph[edge].target;
            const int edgeWeight = _graph[edge].data.distance;
            assert( edgeWeight > 0 );
            const int toDistance = distance + edgeWeight;

            if ( edgeAllowed( _graph[edge] ) ) {
                //New Node discovered -> Add to Heap + Node Info Storage
                if ( !heapForward->WasInserted( to ) )
                    heapForward->Insert( to, toDistance, node );

                //Found a shorter Path -> Update distance
                else if ( toDistance < heapForward->GetKey( to ) ) {
                    heapForward->DecreaseKey( to, toDistance );
                    //new parent
                    heapForward->GetData( to ) = node;
                }
            }
        }
    }

    int _ComputeDistance( NodeID source, NodeID target, _ThreadData * data, std::vector< NodeID >* path = NULL ) {
        data->_heapForward->Clear();
        data->_heapBackward->Clear();
        //insert source into heap
        data->_heapForward->Insert( source, 0, source );
        data->_heapBackward->Insert( target, 0, target );

        int targetDistance = std::numeric_limits< int >::max();
        NodeID middle = 0;
        AllowForwardEdge forward;
        AllowBackwardEdge backward;

        while ( data->_heapForward->Size() + data->_heapBackward->Size() > 0 ) {

            if ( data->_heapForward->Size() > 0 ) {
                _ComputeStep( data->_heapForward, data->_heapBackward, forward, backward, &middle, &targetDistance );
            }

            if ( data->_heapBackward->Size() > 0 ) {
                _ComputeStep( data->_heapBackward, data->_heapForward, backward, forward, &middle, &targetDistance );
            }
        }

        if ( targetDistance == std::numeric_limits< int >::max() )
            return std::numeric_limits< unsigned >::max();

        return targetDistance;
    }
    NodeID _numNodes;
    std::vector< Edge > _graph;
    std::vector< unsigned > _firstEdge;
};

#endif // CONTRACTIONCLEANUP_H_INCLUDED
