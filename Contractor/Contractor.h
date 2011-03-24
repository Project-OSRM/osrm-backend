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

#ifndef CONTRACTOR_H_INCLUDED
#define CONTRACTOR_H_INCLUDED
#ifdef _GLIBCXX_PARALLEL
#include <parallel/algorithm>
#else
#include <algorithm>
#endif
#include "../DataStructures/DynamicGraph.h"
#include "../DataStructures/LevelInformation.h"
#include "../DataStructures/Percent.h"
#include "../DataStructures/BinaryHeap.h"
#include <ctime>
#include <vector>
#include <queue>
#include <set>
#include <stack>
#include <limits>
#include <omp.h>

class Contractor {

private:

    union _MiddleName {
        NodeID middle;
        NodeID nameID;
    };

    struct _EdgeData {
        unsigned distance;
        unsigned originalEdges : 29;
        bool shortcut : 1;
        bool forward : 1;
        bool backward : 1;
        short type:6;
        bool forwardTurn:1;
        bool backwardTurn:1;
        _MiddleName middleName;
    } data;

    struct _HeapData {
        bool target;
        _HeapData() : target(false) {}
        _HeapData( bool t ) : target(t) {}
    };

    typedef DynamicGraph< _EdgeData > _DynamicGraph;
    typedef BinaryHeap< NodeID, NodeID, int, _HeapData> _Heap;
    typedef _DynamicGraph::InputEdge _ImportEdge;

    struct _ThreadData {
        _Heap heap;
        std::vector< _ImportEdge > insertedEdges;
        _ThreadData( NodeID nodes ): heap( nodes ) {
        }
    };

    struct _PriorityData {
        int depth;
        NodeID bias;
        _PriorityData() {
            depth = 0;
        }
    };

    struct _ContractionInformation {
        int edgesDeleted;
        int edgesAdded;
        int originalEdgesDeleted;
        int originalEdgesAdded;
        _ContractionInformation() {
            edgesAdded = edgesDeleted = originalEdgesAdded = originalEdgesDeleted = 0;
        }
    };

    struct _NodePartitionor {
        bool operator()( std::pair< NodeID, bool > nodeData ) {
            return !nodeData.second;
        }
    };

public:

    template< class InputEdge >
    Contractor( int nodes, std::vector< InputEdge >& inputEdges, unsigned eqf = 2, unsigned oqf = 1, unsigned df = 1) : edgeQuotionFactor(eqf), originalQuotientFactor(oqf), depthFactor(df), maxDepth(0) {

        std::vector< _ImportEdge > edges;
        edges.reserve( 2 * inputEdges.size() );
        for ( typename std::vector< InputEdge >::const_iterator i = inputEdges.begin(), e = inputEdges.end(); i != e; ++i ) {
            _ImportEdge edge;
            edge.source = i->source();
            edge.target = i->target();

            edge.data.distance = std::max((int)i->weight(), 1 );
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
            edge.data.forwardTurn = i->isForwardTurn();
            edge.data.backwardTurn = i->isBackwardTurn();
            edges.push_back( edge );
        }
//        std::vector< InputEdge >().swap( inputEdges ); //free memory
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
                if ( (int)forwardEdge.data.distance != std::numeric_limits< int >::max() ) {
                    forwardEdge.data.backward = true;
                    edges[edge++] = forwardEdge;
                }
            } else { //insert seperate edges
                if ( (int)forwardEdge.data.distance != std::numeric_limits< int >::max() ) {
                    edges[edge++] = forwardEdge;
                }
                if ( (int)backwardEdge.data.distance != std::numeric_limits< int >::max() ) {
                    edges[edge++] = backwardEdge;
                }
            }
        }
        cout << "ok" << endl << "removed " << edges.size() - edge << " edges of " << edges.size() << endl;
        edges.resize( edge );
        _graph = new _DynamicGraph( nodes, edges );
        std::vector< _ImportEdge >().swap( edges );
        _levelInformation = new LevelInformation();
    }

    ~Contractor() {
        delete _graph;
        delete _levelInformation;
    }

    int GetNumberOfLevels() const { return maxDepth; }

    template< class InputEdge >
    void CheckForAllOrigEdges(std::vector< InputEdge >& inputEdges) {
        for(unsigned int i = 0; i < inputEdges.size(); i++) {
            bool found = false;
            _DynamicGraph::EdgeIterator eit = _graph->BeginEdges(inputEdges[i].source());
            for(;eit<_graph->EndEdges(inputEdges[i].source()); eit++) {
                if(_graph->GetEdgeData(eit).distance == inputEdges[i].weight())
                    found = true;
            }
            eit = _graph->BeginEdges(inputEdges[i].target());
            for(;eit<_graph->EndEdges(inputEdges[i].target()); eit++) {
                if(_graph->GetEdgeData(eit).distance == inputEdges[i].weight())
                    found = true;
            }
            assert(found);
        }
    }

    void Run() {
        const NodeID numberOfNodes = _graph->GetNumberOfNodes();
        Percent p (numberOfNodes);

        unsigned maxThreads = omp_get_max_threads();
        std::vector < _ThreadData* > threadData;
        for ( unsigned threadNum = 0; threadNum < maxThreads; ++threadNum ) {
            threadData.push_back( new _ThreadData( numberOfNodes ) );
        }
        cout << "Contractor is using " << maxThreads << " threads" << endl;

        NodeID levelID = 0;
        std::vector< std::pair< NodeID, bool > > remainingNodes( numberOfNodes );
        std::vector< double > nodePriority( numberOfNodes );
        std::vector< _PriorityData > nodeData( numberOfNodes );

        //initialize the variables
#pragma omp parallel for schedule ( guided )
        for ( int x = 0; x < ( int ) numberOfNodes; ++x )
            remainingNodes[x].first = x;
        std::random_shuffle( remainingNodes.begin(), remainingNodes.end() );
        for ( int x = 0; x < ( int ) numberOfNodes; ++x )
            nodeData[remainingNodes[x].first].bias = x;

        cout << "initializing elimination PQ ..." << flush;
#pragma omp parallel
        {
            _ThreadData* data = threadData[omp_get_thread_num()];
#pragma omp for schedule ( guided )
            for ( int x = 0; x < ( int ) numberOfNodes; ++x ) {
                nodePriority[x] = _Evaluate( data, &nodeData[x], x );
            }
        }
        cout << "ok" << endl << "preprocessing ..." << flush;

        while ( levelID < numberOfNodes ) {
            const int last = ( int ) remainingNodes.size();

            //determine independent node set
#pragma omp parallel for schedule ( guided )
            for ( int i = 0; i < last; ++i ) {
                const NodeID node = remainingNodes[i].first;
                remainingNodes[i].second = _IsIndependent( _graph, nodePriority, nodeData, node );
            }
            _NodePartitionor functor;
            const std::vector < std::pair < NodeID, bool > >::const_iterator first = stable_partition( remainingNodes.begin(), remainingNodes.end(), functor );
            const int firstIndependent = first - remainingNodes.begin();

            //contract independent nodes
#pragma omp parallel
            {
                _ThreadData* data = threadData[omp_get_thread_num()];
#pragma omp for schedule ( guided ) nowait
                for ( int position = firstIndependent ; position < last; ++position ) {
                    NodeID x = remainingNodes[position].first;
                    _Contract< false > ( data, x );
                    nodePriority[x] = -1;
                }
                std::sort( data->insertedEdges.begin(), data->insertedEdges.end() );
            }

#pragma omp parallel
            {
                _ThreadData* data = threadData[omp_get_thread_num()];
#pragma omp for schedule ( guided ) nowait
                for ( int position = firstIndependent ; position < last; ++position ) {
                    NodeID x = remainingNodes[position].first;
                    _DeleteIncomingEdges( data, x );
                }
            }

            //insert new edges
            for ( unsigned threadNum = 0; threadNum < maxThreads; ++threadNum ) {
                _ThreadData& data = *threadData[threadNum];
                for ( int i = 0; i < ( int ) data.insertedEdges.size(); ++i ) {
                    const _ImportEdge& edge = data.insertedEdges[i];
                    bool found = false;
                    for ( _DynamicGraph::EdgeIterator e = _graph->BeginEdges( edge.source ) ; e < _graph->EndEdges( edge.source ) ; ++e ) {
                        const NodeID target = _graph->GetTarget( e );
                        if ( target != edge.target )
                            continue;
                        _EdgeData& data = _graph->GetEdgeData( e );
                        if ( data.distance != edge.data.distance )
                            continue;
                        if ( data.shortcut != edge.data.shortcut )
                            continue;
                        if ( data.middleName.middle != edge.data.middleName.middle )
                            continue;
                        data.forward |= edge.data.forward;
                        data.backward |= edge.data.backward;
                        found = true;
                        break;
                    }
                    if ( !found )
                        _graph->InsertEdge( edge.source, edge.target, edge.data );
                }
                std::vector< _ImportEdge >().swap( data.insertedEdges );
            }

            //update priorities
#pragma omp parallel
            {
                _ThreadData* data = threadData[omp_get_thread_num()];
#pragma omp for schedule ( guided ) nowait
                for ( int position = firstIndependent ; position < last; ++position ) {
                    NodeID x = remainingNodes[position].first;
                    _UpdateNeighbours( &nodePriority, &nodeData, data, x );
                }
            }

            //remove contracted nodes from the pool
            levelID += last - firstIndependent;
            remainingNodes.resize( firstIndependent );
            std::vector< std::pair< NodeID, bool > >( remainingNodes ).swap( remainingNodes );
            p.printStatus(levelID);
        }

        for ( _DynamicGraph::NodeIterator n = 0; n < _graph->GetNumberOfNodes(); n++ ) {
            _levelInformation->Add(nodeData[n].depth, n);
        }

        for ( unsigned threadNum = 0; threadNum < maxThreads; threadNum++ ) {
            delete threadData[threadNum];
        }

        cout << "[contractor] checking sanity of generated data ..." << flush;
        _CheckCH<_EdgeData>();
        cout << "ok" << endl;
        std::cout << "[contractor] max level: " << maxDepth << std::endl;
    }

    template< class Edge >
    void GetEdges( std::vector< Edge >& edges ) {
        NodeID numberOfNodes = _graph->GetNumberOfNodes();
        for ( NodeID node = 0; node < numberOfNodes; ++node ) {
            for ( _DynamicGraph::EdgeIterator edge = _graph->BeginEdges( node ), endEdges = _graph->EndEdges( node ); edge < endEdges; edge++ ) {
                const NodeID target = _graph->GetTarget( edge );
                const _EdgeData& data = _graph->GetEdgeData( edge );
                Edge newEdge;
                newEdge.source = node;
                newEdge.target = target;
                newEdge.data.distance = data.distance;
                newEdge.data.shortcut = data.shortcut;
                if(data.shortcut) {
                    newEdge.data.middleName.middle = data.middleName.middle;
                    newEdge.data.type = -1;
                } else {
                    newEdge.data.middleName.nameID = data.middleName.nameID;
                    newEdge.data.type = data.type;
                    assert(newEdge.data.type >= 0);
                }

                newEdge.data.forward = data.forward;
                newEdge.data.forwardTurn = data.forwardTurn;
                newEdge.data.backwardTurn = data.backwardTurn;
                newEdge.data.backward = data.backward;
                edges.push_back( newEdge );
            }
        }
    }

    LevelInformation * GetLevelInformation() {
        return _levelInformation;
    }

private:
    bool _ConstructCH( _DynamicGraph* _graph );

    void _Dijkstra( NodeID source, const int maxDistance, const unsigned numTargets, _ThreadData* data ){

        _Heap& heap = data->heap;

        unsigned nodes = 0;
        while ( heap.Size() > 0 ) {
            const NodeID node = heap.DeleteMin();
            const int distance = heap.GetKey( node );
            if ( nodes++ > numTargets )
                return;
            //Destination settled?
            if ( distance > maxDistance )
                return;

            //iterate over all edges of node
            for ( _DynamicGraph::EdgeIterator edge = _graph->BeginEdges( node ), endEdges = _graph->EndEdges( node ); edge != endEdges; ++edge ) {
                const _EdgeData& data = _graph->GetEdgeData( edge );
                if ( !data.forward )
                    continue;
                const NodeID to = _graph->GetTarget( edge );
                const int toDistance = distance + data.distance;

                //New Node discovered -> Add to Heap + Node Info Storage
                if ( !heap.WasInserted( to ) )
                    heap.Insert( to, toDistance, _HeapData() );

                //Found a shorter Path -> Update distance
                else if ( toDistance < heap.GetKey( to ) ) {
                    heap.DecreaseKey( to, toDistance );
                    //heap.GetData( to ).hops = hops + 1;
                }
            }
        }
    }

    double _Evaluate( _ThreadData* data, _PriorityData* nodeData, NodeID node ){
        _ContractionInformation stats;

        //perform simulated contraction
        _Contract< true > ( data, node, &stats );
        if(nodeData->depth > maxDepth) {
#pragma omp critical
            maxDepth = nodeData->depth;
        }
        // Result will contain the priority
        if ( stats.edgesDeleted == 0 || stats.originalEdgesDeleted == 0 )
            return depthFactor * nodeData->depth;
        return edgeQuotionFactor * ((( double ) stats.edgesAdded ) / stats.edgesDeleted ) + originalQuotientFactor * ((( double ) stats.originalEdgesAdded ) / stats.originalEdgesDeleted ) + depthFactor * nodeData->depth;
    }

    template< class Edge >
    bool _CheckCH()
    {
        NodeID numberOfNodes = _graph->GetNumberOfNodes();
        for ( NodeID node = 0; node < numberOfNodes; ++node ) {
            for ( _DynamicGraph::EdgeIterator edge = _graph->BeginEdges( node ), endEdges = _graph->EndEdges( node ); edge != endEdges; ++edge ) {
                const NodeID start = node;
                const NodeID target = _graph->GetTarget( edge );
                const _EdgeData& data = _graph->GetEdgeData( edge );
                const NodeID middle = data.middleName.middle;
                assert(start != target);
                if(data.shortcut)
                {
                    if(_graph->FindEdge(start, middle) == SPECIAL_EDGEID && _graph->FindEdge(middle, start) == SPECIAL_EDGEID)
                    {
                        assert(false);
                        return false;
                    }
                    if(_graph->FindEdge(middle, target) == SPECIAL_EDGEID && _graph->FindEdge(target, middle) == SPECIAL_EDGEID)
                    {
                        assert(false);
                        return false;
                    }
                }
            }
        }
        return true;
    }

    template< bool Simulate > bool _Contract( _ThreadData* data, NodeID node, _ContractionInformation* stats = NULL ) {
        _Heap& heap = data->heap;

        for ( _DynamicGraph::EdgeIterator inEdge = _graph->BeginEdges( node ), endInEdges = _graph->EndEdges( node ); inEdge != endInEdges; ++inEdge ) {
            const _EdgeData& inData = _graph->GetEdgeData( inEdge );
            const NodeID source = _graph->GetTarget( inEdge );
            if ( Simulate ) {
                assert( stats != NULL );
                unsigned factor = (inData.forward && inData.backward ? 2 : 1 );
                stats->edgesDeleted+=factor;
                stats->originalEdgesDeleted += factor*inData.originalEdges;
            }
            if ( !inData.backward )
                continue;

            heap.Clear();
            heap.Insert( source, 0, _HeapData() );
            if ( node != source )
                heap.Insert( node, inData.distance, _HeapData() );
            int maxDistance = 0;
            //unsigned numTargets = 0;

            for ( _DynamicGraph::EdgeIterator outEdge = _graph->BeginEdges( node ), endOutEdges = _graph->EndEdges( node ); outEdge != endOutEdges; ++outEdge ) {
                const _EdgeData& outData = _graph->GetEdgeData( outEdge );
                if ( !outData.forward )
                    continue;
                const NodeID target = _graph->GetTarget( outEdge );
                const int pathDistance = inData.distance + outData.distance;
                maxDistance = std::max( maxDistance, pathDistance );
                if ( !heap.WasInserted( target ) )
                    heap.Insert( target, pathDistance, _HeapData(true) );
                else if ( pathDistance < heap.GetKey( target ) )
                    heap.DecreaseKey( target, pathDistance );
            }

            if( Simulate )
                _Dijkstra( source, maxDistance, 500, data );
            else
                _Dijkstra( source, maxDistance, 1000, data );

            for ( _DynamicGraph::EdgeIterator outEdge = _graph->BeginEdges( node ), endOutEdges = _graph->EndEdges( node ); outEdge != endOutEdges; ++outEdge ) {
                const _EdgeData& outData = _graph->GetEdgeData( outEdge );
                if ( !outData.forward )
                    continue;
                const NodeID target = _graph->GetTarget( outEdge );
                const int pathDistance = inData.distance + outData.distance;
                const int distance = heap.GetKey( target );

                if ( pathDistance <= distance ) {
                    if ( Simulate ) {
                        assert( stats != NULL );
                        stats->edgesAdded++;
                        stats->originalEdgesAdded += ( outData.originalEdges + inData.originalEdges );
                    } else {
                        _ImportEdge newEdge;
                        newEdge.source = source;
                        newEdge.target = target;
                        newEdge.data.distance = pathDistance;
                        newEdge.data.forward = true;
                        newEdge.data.backward = false;
                        newEdge.data.middleName.middle = node;
                        newEdge.data.shortcut = true;
                        newEdge.data.originalEdges = outData.originalEdges + inData.originalEdges;
                        data->insertedEdges.push_back( newEdge );
                        std::swap( newEdge.source, newEdge.target );
                        newEdge.data.forward = false;
                        newEdge.data.backward = true;
                        data->insertedEdges.push_back( newEdge );
                    }
                }
            }
        }
        return true;
    }

    bool _DeleteIncomingEdges( _ThreadData* data, NodeID node ) {
        std::vector < NodeID > neighbours;

        //find all neighbours
        for ( _DynamicGraph::EdgeIterator e = _graph->BeginEdges( node ) ; e < _graph->EndEdges( node ) ; ++e ) {
            const NodeID u = _graph->GetTarget( e );
            if ( u == node )
                continue;
            neighbours.push_back( u );
        }
        //eliminate duplicate entries ( forward + backward edges )
        std::sort( neighbours.begin(), neighbours.end() );
        neighbours.resize( std::unique( neighbours.begin(), neighbours.end() ) - neighbours.begin() );

        for ( int i = 0, e = ( int ) neighbours.size(); i < e; ++i ) {
            const NodeID u = neighbours[i];
            _graph->DeleteEdgesTo( u, node );
        }

        return true;
    }

    bool _UpdateNeighbours( std::vector< double >* priorities, std::vector< _PriorityData >* nodeData, _ThreadData* data, NodeID node ) {
        std::vector < NodeID > neighbours;

        //find all neighbours
        for ( _DynamicGraph::EdgeIterator e = _graph->BeginEdges( node ) ; e < _graph->EndEdges( node ) ; ++e ) {
            const NodeID u = _graph->GetTarget( e );
            if ( u == node )
                continue;
            neighbours.push_back( u );
            ( *nodeData )[u].depth = std::max(( *nodeData )[node].depth + 1, ( *nodeData )[u].depth );
        }
        //eliminate duplicate entries ( forward + backward edges )
        std::sort( neighbours.begin(), neighbours.end() );
        neighbours.resize( std::unique( neighbours.begin(), neighbours.end() ) - neighbours.begin() );

        for ( int i = 0, e = ( int ) neighbours.size(); i < e; ++i ) {
            const NodeID u = neighbours[i];
            ( *priorities )[u] = _Evaluate( data, &( *nodeData )[u], u );
        }

        return true;
    }

    bool _IsIndependent( const _DynamicGraph* _graph, const std::vector< double >& priorities, const std::vector< _PriorityData >& nodeData, NodeID node ) {
        const double priority = priorities[node];

        std::vector< NodeID > neighbours;

        for ( _DynamicGraph::EdgeIterator e = _graph->BeginEdges( node ) ; e < _graph->EndEdges( node ) ; ++e ) {
            const NodeID target = _graph->GetTarget( e );
            const double targetPriority = priorities[target];
            assert( targetPriority >= 0 );
            //found a neighbour with lower priority?
            if ( priority > targetPriority )
                return false;
            //tie breaking
            if ( priority == targetPriority && nodeData[node].bias < nodeData[target].bias )
                return false;
            neighbours.push_back( target );
        }

        std::sort( neighbours.begin(), neighbours.end() );
        neighbours.resize( std::unique( neighbours.begin(), neighbours.end() ) - neighbours.begin() );

        //examine all neighbours that are at most 2 hops away
        for ( std::vector< NodeID >::const_iterator i = neighbours.begin(), lastNode = neighbours.end(); i != lastNode; ++i ) {
            const NodeID u = *i;

            for ( _DynamicGraph::EdgeIterator e = _graph->BeginEdges( u ) ; e < _graph->EndEdges( u ) ; ++e ) {
                const NodeID target = _graph->GetTarget( e );

                const double targetPriority = priorities[target];
                assert( targetPriority >= 0 );
                //found a neighbour with lower priority?
                if ( priority > targetPriority )
                    return false;
                //tie breaking
                if ( priority == targetPriority && nodeData[node].bias < nodeData[target].bias )
                    return false;
            }
        }

        return true;
    }

    LevelInformation * _levelInformation;
    _DynamicGraph* _graph;
    std::vector<NodeID> * _components;
    unsigned edgeQuotionFactor;
    unsigned originalQuotientFactor;
    unsigned depthFactor;
    int maxDepth;
};

#endif // CONTRACTOR_H_INCLUDED
