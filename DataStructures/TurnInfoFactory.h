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

#ifndef TURNINFOFACTORY_H_INCLUDED
#define TURNINFOFACTORY_H_INCLUDED
#ifdef _GLIBCXX_PARALLEL
#include <parallel/algorithm>
#else
#include <algorithm>
#endif
#include "StaticGraph.h"
#include "Percent.h"
#include <ctime>
#include <vector>
#include <queue>
#include <set>
#include <stack>
#include <limits>
#include <omp.h>

typedef StaticGraph<MinimalEdgeData>::InputEdge MinimalEdge;
typedef StaticGraph<MinimalEdgeData> _StaticGraph;
//template <typename InputEdge>
class TurnInfoFactory {
public:

    TurnInfoFactory( int nodes, std::vector< ImportEdge >& inputEdges ) : _inputEdges (inputEdges) {
        std::vector< MinimalEdge > edges;
        edges.reserve( 2 * inputEdges.size() );
        for ( std::vector< ImportEdge >::const_iterator i = inputEdges.begin(), e = inputEdges.end(); i != e; ++i ) {
            MinimalEdge edge;
            edge.source = i->source();
            edge.target = i->target();

            edge.data.distance = std::max((int)i->weight(), 1 );
            assert( edge.data.distance > 0 );
            if ( edge.data.distance > 24 * 60 * 60 * 10 ) {
                cout << "Edge Weight too large -> May lead to invalid CH" << endl;
                continue;
            }
            if ( edge.data.distance <= 0 ) {
                cout <<  "Edge Weight too small -> May lead to invalid CH or Crashes"<< endl;
                continue;
            }
            edge.data.forward = i->isForward();
            edge.data.backward = i->isBackward();
            edges.push_back( edge );
            std::swap( edge.source, edge.target );
            edge.data.forward = i->isBackward();
            edge.data.backward = i->isForward();
            edges.push_back( edge );
        }
#ifdef _GLIBCXX_PARALLEL
        __gnu_parallel::sort( edges.begin(), edges.end() );
#else
        sort( edges.begin(), edges.end() );
#endif
        NodeID edge = 0;
        for ( NodeID i = 0; i < edges.size(); ) {
            const NodeID source = edges[i].source;
            const NodeID target = edges[i].target;
            //remove eigenloops
            if ( source == target ) {
                i++;
                continue;
            }
            MinimalEdge forwardEdge;
            MinimalEdge backwardEdge;
            forwardEdge.source = backwardEdge.source = source;
            forwardEdge.target = backwardEdge.target = target;
            forwardEdge.data.forward = backwardEdge.data.backward = true;
            forwardEdge.data.backward = backwardEdge.data.forward = false;
//            forwardEdge.data.type = backwardEdge.data.type = type;
//            forwardEdge.data.middleName.nameID = backwardEdge.data.middleName.nameID = middle;
//            forwardEdge.data.shortcut = backwardEdge.data.shortcut = false;
//            forwardEdge.data.originalEdges = backwardEdge.data.originalEdges = 1;
            forwardEdge.data.distance = backwardEdge.data.distance = std::numeric_limits< int >::max();
            //remove parallel edges
            while ( i < edges.size() && edges[i].source == source && edges[i].target == target ) {
                if ( edges[i].data.forward )
                    forwardEdge.data.distance = std::min( edges[i].data.distance, forwardEdge.data.distance );
                if ( edges[i].data.backward )
                    backwardEdge.data.distance = std::min( edges[i].data.distance, backwardEdge.data.distance );
                i++;
            }
            //merge edges (s,t) and (t,s) into bidirectional edge
            if ( forwardEdge.data.distance == backwardEdge.data.distance ) {
                if ( forwardEdge.data.distance != std::numeric_limits< int >::max() ) {
                    forwardEdge.data.backward = true;
                    edges[edge++] = forwardEdge;
                }
            } else { //insert seperate edges
                if ( forwardEdge.data.distance != std::numeric_limits< int >::max() ) {
                    edges[edge++] = forwardEdge;
                }
                if ( backwardEdge.data.distance != std::numeric_limits< int >::max() ) {
                    edges[edge++] = backwardEdge;
                }
            }
        }
        cout << "ok" << endl << "removed " << edges.size() - edge << " edges of " << edges.size() << endl;
        edges.resize( edge );
        _graph = new _StaticGraph( nodes, edges );

        std::vector< MinimalEdge >().swap( edges );
    }

    ~TurnInfoFactory() {
        delete _graph;
    }

    /* check if its possible to turn at the end of an edge */
    template< class InputEdge >
    void Run () {
        unsigned count = 0;
        for(unsigned n = 0; n < _inputEdges.size(); n++) {
            NodeID target = _inputEdges[n].target();
            NodeID source = _inputEdges[n].source();

            if(_inputEdges[n].isForward() ) {
                EdgeID begin = _graph->BeginEdges(target);
                EdgeID end = _graph->EndEdges(target);
                if( begin + ( _inputEdges[n].isBackward() ? 2 : 1 ) < end ) {
                    _inputEdges[n].setForwardTurn( true );
                    count++;
                }
            }

            if(_inputEdges[n].isBackward() ) {
                EdgeID begin = _graph->BeginEdges(source);
                EdgeID end = _graph->EndEdges(source);
                if( begin + ( _inputEdges[n].isForward() ? 2 : 1 ) < end ) {
                    _inputEdges[n].setBackwardTurn( true );
                    count ++;
                }
            }
        }
        cout << "allowed turns: " << count << endl;
    }

    /* check if its possible to turn at the end of an edge */
    void Run () {
        unsigned count = 0;
        for(unsigned n = 0; n < _inputEdges.size(); n++) {
            NodeID target = _inputEdges[n].target();
            NodeID source = _inputEdges[n].source();

            if(_inputEdges[n].isForward() ) {
                EdgeID begin = _graph->BeginEdges(target);
                EdgeID end = _graph->EndEdges(target);
                if( begin + ( _inputEdges[n].isBackward() ? 2 : 1 ) < end ) {
                    _inputEdges[n].setForwardTurn( true );
                    count++;
                }
            }

            if(_inputEdges[n].isBackward() ) {
                EdgeID begin = _graph->BeginEdges(source);
                EdgeID end = _graph->EndEdges(source);
                if( begin + ( _inputEdges[n].isForward() ? 2 : 1 ) < end ) {
                    _inputEdges[n].setBackwardTurn( true );
                    count ++;
                }
            }
        }
        cout << "allowed turns: " << count << endl;
    }

private:
    _StaticGraph* _graph;
    std::vector<NodeID> * _components;
    std::vector< ImportEdge >& _inputEdges;
};

#endif // TURNINFOFACTORY_H_INCLUDED
