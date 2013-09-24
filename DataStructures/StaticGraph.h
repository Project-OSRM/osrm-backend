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

#ifndef STATICGRAPH_H_INCLUDED
#define STATICGRAPH_H_INCLUDED

#include "../DataStructures/Percent.h"
#include "../DataStructures/SharedMemoryVectorWrapper.h"
#include "../Util/SimpleLogger.h"
#include "../typedefs.h"

#include <algorithm>
#include <vector>

template< typename EdgeDataT, bool UseSharedMemory = false>
class StaticGraph {
public:
    typedef NodeID NodeIterator;
    typedef NodeID EdgeIterator;
    typedef EdgeDataT EdgeData;
    class InputEdge {
    public:
        EdgeDataT data;
        NodeIterator source;
        NodeIterator target;
        bool operator<( const InputEdge& right ) const {
            if ( source != right.source ) {
                return source < right.source;
            }
            return target < right.target;
        }
    };

    struct _StrNode {
        //index of the first edge
        EdgeIterator firstEdge;
    };

    struct _StrEdge {
        NodeID target;
        EdgeDataT data;
    };

    StaticGraph( const int nodes, std::vector< InputEdge > &graph ) {
        std::sort( graph.begin(), graph.end() );
        _numNodes = nodes;
        _numEdges = ( EdgeIterator ) graph.size();
        _nodes.resize( _numNodes + 1);
        EdgeIterator edge = 0;
        EdgeIterator position = 0;
        for ( NodeIterator node = 0; node <= _numNodes; ++node ) {
            EdgeIterator lastEdge = edge;
            while ( edge < _numEdges && graph[edge].source == node )
                ++edge;
            _nodes[node].firstEdge = position; //=edge
            position += edge - lastEdge; //remove
        }
        _edges.resize( position ); //(edge)
        edge = 0;
        for ( NodeIterator node = 0; node < _numNodes; ++node ) {
            for ( EdgeIterator i = _nodes[node].firstEdge, e = _nodes[node+1].firstEdge; i != e; ++i ) {
                _edges[i].target = graph[edge].target;
                _edges[i].data = graph[edge].data;
                assert(_edges[i].data.distance > 0);
                edge++;
            }
        }
    }

    StaticGraph(
        typename ShM<_StrNode, UseSharedMemory>::vector & nodes,
        typename ShM<_StrEdge, UseSharedMemory>::vector & edges
    ) {
        _numNodes = nodes.size()-1;
        _numEdges = edges.size();

        _nodes.swap(nodes);
        _edges.swap(edges);

#ifndef NDEBUG
        Percent p(GetNumberOfNodes());
        for(unsigned u = 0; u < GetNumberOfNodes(); ++u) {
            for(unsigned eid = BeginEdges(u); eid < EndEdges(u); ++eid) {
                unsigned v = GetTarget(eid);
                EdgeData & data = GetEdgeData(eid);
                if(data.shortcut) {
                    unsigned eid2 = FindEdgeInEitherDirection(u, data.id);
                    if(eid2 == UINT_MAX) {
                        SimpleLogger().Write(logWARNING) <<
                            "cannot find first segment of edge (" <<
                                u << "," << data.id << "," << v << ")";

                        data.shortcut = false;
                    }
                    eid2 = FindEdgeInEitherDirection(data.id, v);
                    if(eid2 == UINT_MAX) {
                        SimpleLogger().Write(logWARNING) <<
                            "cannot find second segment of edge (" <<
                                u << "," << data.id << "," << v << ")";
                        data.shortcut = false;
                    }
                }
            }
            p.printIncrement();
        }
#endif
    }

    unsigned GetNumberOfNodes() const {
        return _numNodes;
    }

    unsigned GetNumberOfEdges() const {
        return _numEdges;
    }

    unsigned GetOutDegree( const NodeIterator n ) const {
        return BeginEdges(n)-EndEdges(n) - 1;
    }

    inline NodeIterator GetTarget( const EdgeIterator e ) const {
        return NodeIterator( _edges[e].target );
    }

    inline EdgeDataT &GetEdgeData( const EdgeIterator e ) {
        return _edges[e].data;
    }

    const EdgeDataT &GetEdgeData( const EdgeIterator e ) const {
        return _edges[e].data;
    }

    EdgeIterator BeginEdges( const NodeIterator n ) const {
        return EdgeIterator( _nodes[n].firstEdge );
    }

    EdgeIterator EndEdges( const NodeIterator n ) const {
        return EdgeIterator( _nodes[n+1].firstEdge );
    }

    //searches for a specific edge
    EdgeIterator FindEdge( const NodeIterator from, const NodeIterator to ) const {
        EdgeIterator smallestEdge = SPECIAL_EDGEID;
        EdgeWeight smallestWeight = UINT_MAX;
        for ( EdgeIterator edge = BeginEdges( from ); edge < EndEdges(from); edge++ ) {
            const NodeID target = GetTarget(edge);
            const EdgeWeight weight = GetEdgeData(edge).distance;
            if(target == to && weight < smallestWeight) {
                smallestEdge = edge; smallestWeight = weight;
            }
        }
        return smallestEdge;
    }

    EdgeIterator FindEdgeInEitherDirection( const NodeIterator from, const NodeIterator to ) const {
        EdgeIterator tmp =  FindEdge( from, to );
        return (UINT_MAX != tmp ? tmp : FindEdge( to, from ));
    }

    EdgeIterator FindEdgeIndicateIfReverse( const NodeIterator from, const NodeIterator to, bool & result ) const {
        EdgeIterator tmp =  FindEdge( from, to );
        if(UINT_MAX == tmp) {
            tmp =  FindEdge( to, from );
            if(UINT_MAX != tmp) {
                result = true;
            }
        }
        return tmp;
    }

private:

    NodeIterator _numNodes;
    EdgeIterator _numEdges;

    typename ShM< _StrNode, UseSharedMemory >::vector _nodes;
    typename ShM< _StrEdge, UseSharedMemory >::vector _edges;
};

#endif // STATICGRAPH_H_INCLUDED
