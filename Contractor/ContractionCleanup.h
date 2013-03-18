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

#include <algorithm>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include "Contractor.h"
#include "TravelMode.h"

class ContractionCleanup {
private:

    struct _CleanupHeapData {
        NodeID parent;
        _CleanupHeapData( NodeID p ) {
            parent = p;
        }
    };
    typedef BinaryHeap< NodeID, NodeID, int, _CleanupHeapData > _Heap;

    struct _ThreadData {
        _Heap* _heapForward;
        _Heap* _heapBackward;
        _ThreadData( NodeID nodes ) {
            _heapBackward = new _Heap(nodes);
            _heapForward = new _Heap(nodes);
        }
        ~_ThreadData() {
            delete _heapBackward;
            delete _heapForward;
        }
    };

public:

    struct Edge {
        NodeID source;
        NodeID target;
        struct EdgeData {
        	NodeID via;
        	unsigned nameID;
        	int distance;
        	TurnInstruction turnInstruction;
        	bool shortcut:1;
        	bool forward:1;
        	bool backward:1;
            TravelMode mode;
        } data;
        bool operator<( const Edge& right ) const {
            if ( source != right.source )
                return source < right.source;
            return target < right.target;
        }

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
            return ( source == right.source && target == right.target && data.distance == right.data.distance &&
                    data.shortcut == right.data.shortcut && data.forward == right.data.forward && data.backward == right.data.backward
                    && data.via == right.data.via && data.nameID == right.data.nameID
                    );
        }
    };

    ContractionCleanup( int numNodes, const std::vector< Edge >& edges ) {
        _graph = edges;
        _numNodes = numNodes;
    }

    ~ContractionCleanup() {

    }

    void Run() {
        RemoveUselessShortcuts();
    }

    template< class EdgeT >
    void GetData( std::vector< EdgeT >& edges ) {
        for ( int edge = 0, endEdges = ( int ) _graph.size(); edge != endEdges; ++edge ) {
            if(_graph[edge].data.forward || _graph[edge].data.backward) {
                EdgeT newEdge;
                newEdge.source = _graph[edge].source;
                newEdge.target = _graph[edge].target;
                newEdge.data = _graph[edge].data;
                edges.push_back( newEdge );
            }
        }
        sort( edges.begin(), edges.end() );
    }

private:

    double _Timestamp() {
        struct timeval tp;
        gettimeofday(&tp, NULL);
        return double(tp.tv_sec) + tp.tv_usec / 1000000.;
    }

    void BuildOutgoingGraph() {
        //sort edges by source
        sort( _graph.begin(), _graph.end(), Edge::CompareBySource );
        try {
            _firstEdge.resize( _numNodes + 1 );
        } catch(...) {
            ERR("Not enough RAM on machine");
            return;
        }
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

        INFO("Scanning for useless shortcuts");
        BuildOutgoingGraph();
/*
        #pragma omp parallel for
        for ( int i = 0; i < ( int ) _graph.size(); i++ ) {
            //only remove shortcuts
            if ( !_graph[i].data.shortcut )
                continue;

            if ( _graph[i].data.forward ) {
                int result = _ComputeDistance( _graph[i].source, _graph[i].target, threadData[omp_get_thread_num()] );
                if ( result < _graph[i].data.distance ) {
                    _graph[i].data.forward = false;
                }
            }
            if ( _graph[i].data.backward ) {
                int result = _ComputeDistance( _graph[i].target, _graph[i].source, threadData[omp_get_thread_num()] );
                if ( result < _graph[i].data.distance ) {
                    _graph[i].data.backward = false;
                }
            }
        }
*/
        INFO("Removing edges");
        int useful = 0;
        for ( int i = 0; i < ( int ) _graph.size(); i++ ) {
            if ( !_graph[i].data.forward && !_graph[i].data.backward && _graph[i].data.shortcut ) {
                continue;
            }
            _graph[useful] = _graph[i];
            useful++;
        }
        INFO("Removed " << _graph.size() - useful << " useless shortcuts");
        _graph.resize( useful );

        for ( int threadNum = 0; threadNum < maxThreads; ++threadNum ) {
            delete threadData[threadNum];
        }
    }

    void _ComputeStep( _Heap* heapForward, _Heap* heapBackward, bool forwardDirection, NodeID* middle, int* targetDistance ) {

        const NodeID node = heapForward->DeleteMin();
        const int distance = heapForward->GetKey( node );

        if ( distance > *targetDistance ) {
            heapForward->DeleteAll();
            return;
        }

        if ( heapBackward->WasInserted( node ) ) {
            const int newDistance = heapBackward->GetKey( node ) + distance;
            if ( newDistance < *targetDistance ) {
                *middle = node;
                *targetDistance = newDistance;
            }
        }

       for ( int edge = _firstEdge[node], endEdges = _firstEdge[node + 1]; edge != endEdges; ++edge ) {
            const NodeID to = _graph[edge].target;
            const int edgeWeight = _graph[edge].data.distance;
            assert( edgeWeight > 0 );
            const int toDistance = distance + edgeWeight;

            if ( (forwardDirection ? _graph[edge].data.forward : _graph[edge].data.backward ) ) {
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

    int _ComputeDistance( NodeID source, NodeID target, _ThreadData * data ) {
        data->_heapForward->Clear();
        data->_heapBackward->Clear();
        //insert source into heap
        data->_heapForward->Insert( source, 0, source );
        data->_heapBackward->Insert( target, 0, target );

        int targetDistance = std::numeric_limits< int >::max();
        NodeID middle = std::numeric_limits<NodeID>::max();

        while ( data->_heapForward->Size() + data->_heapBackward->Size() > 0 ) {
            if ( data->_heapForward->Size() > 0 ) {
                _ComputeStep( data->_heapForward, data->_heapBackward, true, &middle, &targetDistance );
            }

            if ( data->_heapBackward->Size() > 0 ) {
                _ComputeStep( data->_heapBackward, data->_heapForward, false, &middle, &targetDistance );
            }
        }
        return targetDistance;
    }
    NodeID _numNodes;
    std::vector< Edge > _graph;
    std::vector< unsigned > _firstEdge;
};

#endif // CONTRACTIONCLEANUP_H_INCLUDED
