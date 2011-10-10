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

#include "../typedefs.h"
#include "../DataStructures/DynamicGraph.h"
#include "../DataStructures/ImportEdge.h"
#include "EdgeBasedGraphFactory.h"

template<>
EdgeBasedGraphFactory::EdgeBasedGraphFactory(int nodes, std::vector<Edge> & inputEdges) {
    std::vector< _ImportEdge > edges;
    edges.reserve( 2 * inputEdges.size() );
    for ( typename std::vector< Edge >::const_iterator i = inputEdges.begin(), e = inputEdges.end(); i != e; ++i ) {
        _ImportEdge edge;
        edge.source = i->source();
        edge.target = i->target();

        edge.data.distance = (std::max)((int)i->weight(), 1 );
        assert( edge.data.distance > 0 );
#ifdef DEBUG
        if ( edge.data.distance > 24 * 60 * 60 * 10 ) {
            cout << "Edge Weight too large -> May lead to invalid CH" << endl;
            continue;
        }
#endif
        edge.data.shortcut = false;
        edge.data.middleName.nameID = i->name();
        edge.data.type = i->type();
        edge.data.forward = i->isForward();
        edge.data.backward = i->isBackward();
        edge.data.originalEdges = 1;
        edges.push_back( edge );
        std::swap( edge.source, edge.target );
        edge.data.forward = i->isBackward();
        edge.data.backward = i->isForward();
        edges.push_back( edge );
    }
    //        std::vector< InputEdgeT >().swap( inputEdges ); //free memory
#ifdef _GLIBCXX_PARALLEL
    __gnu_parallel::sort( edges.begin(), edges.end() );
#else
    sort( edges.begin(), edges.end() );
#endif
    NodeID edge = 0;
    for ( NodeID i = 0; i < edges.size(); ) {
        const NodeID source = edges[i].source;
        const NodeID target = edges[i].target;
        const NodeID middle = edges[i].data.middleName.nameID;
        const short type = edges[i].data.type;
        assert(type >= 0);
        //remove eigenloops
        if ( source == target ) {
            i++;
            continue;
        }
        _ImportEdge forwardEdge;
        _ImportEdge backwardEdge;
        forwardEdge.source = backwardEdge.source = source;
        forwardEdge.target = backwardEdge.target = target;
        forwardEdge.data.forward = backwardEdge.data.backward = true;
        forwardEdge.data.backward = backwardEdge.data.forward = false;
        forwardEdge.data.type = backwardEdge.data.type = type;
        forwardEdge.data.middleName.nameID = backwardEdge.data.middleName.nameID = middle;
        forwardEdge.data.shortcut = backwardEdge.data.shortcut = false;
        forwardEdge.data.originalEdges = backwardEdge.data.originalEdges = 1;
        forwardEdge.data.distance = backwardEdge.data.distance = (std::numeric_limits< int >::max)();
        //remove parallel edges
        while ( i < edges.size() && edges[i].source == source && edges[i].target == target ) {
            if ( edges[i].data.forward )
                forwardEdge.data.distance = (std::min)( edges[i].data.distance, forwardEdge.data.distance );
            if ( edges[i].data.backward )
                backwardEdge.data.distance = (std::min)( edges[i].data.distance, backwardEdge.data.distance );
            i++;
        }
        //merge edges (s,t) and (t,s) into bidirectional edge
        if ( forwardEdge.data.distance == backwardEdge.data.distance ) {
            if ( (int)forwardEdge.data.distance != (std::numeric_limits< int >::max)() ) {
                forwardEdge.data.backward = true;
                edges[edge++] = forwardEdge;
            }
        } else { //insert seperate edges
            if ( (int)forwardEdge.data.distance != (std::numeric_limits< int >::max)() ) {
                edges[edge++] = forwardEdge;
            }
            if ( (int)backwardEdge.data.distance != (std::numeric_limits< int >::max)() ) {
                edges[edge++] = backwardEdge;
            }
        }
    }
    cout << "ok" << endl << "removed " << edges.size() - edge << " edges of " << edges.size() << endl;
    edges.resize( edge );
    _graph = new _NodeBasedDynamicGraph( nodes, edges );
    std::vector< _ImportEdge >().swap( edges );
}

EdgeBasedGraphFactory::~EdgeBasedGraphFactory() {
    DELETE(_graph);
}

