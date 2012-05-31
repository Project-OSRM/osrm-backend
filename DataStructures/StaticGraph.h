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

#ifndef STATICGRAPH_H_INCLUDED
#define STATICGRAPH_H_INCLUDED

#include <vector>
#include <algorithm>

#include "../typedefs.h"
#include "ImportEdge.h"

template< typename EdgeDataT>
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
            if ( source != right.source )
                return source < right.source;
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

    StaticGraph( std::vector<_StrNode> & nodes, std::vector<_StrEdge> & edges) {
        _numNodes = nodes.size();
        _numEdges = edges.size();

        _nodes.swap(nodes);
        _edges.swap(edges);

        //Add dummy node to end of _nodes array;
        _nodes.push_back(_nodes.back());

#ifndef NDEBUG
        Percent p(GetNumberOfNodes());
        for(unsigned u = 0; u < GetNumberOfNodes(); ++u) {
            for(unsigned eid = BeginEdges(u); eid < EndEdges(u); ++eid) {
                unsigned v = GetTarget(eid);
                EdgeData & data = GetEdgeData(eid);
                if(data.shortcut) {
                    unsigned eid2 = FindEdgeInEitherDirection(u, data.id);
                    if(eid2 == UINT_MAX) {
                        DEBUG("cannot find first segment of edge (" << u << "," << data.id << "," << v << ")");
                        data.shortcut = false;
                    }
                    eid2 = FindEdgeInEitherDirection(data.id, v);
                    if(eid2 == UINT_MAX) {
                        DEBUG("cannot find second segment of edge (" << u << "," << data.id << "," << v << ")");
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

    unsigned GetOutDegree( const NodeIterator &n ) const {
        return BeginEdges(n)-EndEdges(n) - 1;
    }

    inline NodeIterator GetTarget( const EdgeIterator &e ) const {
        return NodeIterator( _edges[e].target );
    }

    inline EdgeDataT &GetEdgeData( const EdgeIterator &e ) {
        return _edges[e].data;
    }

    const EdgeDataT &GetEdgeData( const EdgeIterator &e ) const {
        return _edges[e].data;
    }

    EdgeIterator BeginEdges( const NodeIterator &n ) const {
        return EdgeIterator( _nodes[n].firstEdge );
    }

    EdgeIterator EndEdges( const NodeIterator &n ) const {
        return EdgeIterator( _nodes[n+1].firstEdge );
    }

    //searches for a specific edge
    EdgeIterator FindEdge( const NodeIterator &from, const NodeIterator &to ) const {
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

    EdgeIterator FindEdgeInEitherDirection( const NodeIterator &from, const NodeIterator &to ) const {
        EdgeIterator tmp =  FindEdge( from, to );
        return (UINT_MAX != tmp ? tmp : FindEdge( to, from ));
    }

    EdgeIterator FindEdgeIndicateIfReverse( const NodeIterator &from, const NodeIterator &to, bool & result ) const {
        EdgeIterator tmp =  FindEdge( from, to );
        if(UINT_MAX == tmp) {
            tmp =  FindEdge( to, from );
            if(UINT_MAX != tmp)
                result = true;
        }
        return tmp;
    }

private:

    NodeIterator _numNodes;
    EdgeIterator _numEdges;

    std::vector< _StrNode > _nodes;
    std::vector< _StrEdge > _edges;
};

#endif // STATICGRAPH_H_INCLUDED
