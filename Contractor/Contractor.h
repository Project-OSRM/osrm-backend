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
#include <algorithm>
#include <ctime>
#include <limits>
#include <queue>
#include <set>
#include <vector>

#include <stxxl.h>

#include <boost/shared_ptr.hpp>

#include "../DataStructures/BinaryHeap.h"
#include "../DataStructures/DeallocatingVector.h"
#include "../DataStructures/DynamicGraph.h"
#include "../DataStructures/Percent.h"
#include "../DataStructures/XORFastHash.h"
#include "../Util/OpenMPReplacement.h"
#include "../Util/StringUtil.h"

class Contractor {

private:
    struct _ContractorEdgeData {
        _ContractorEdgeData() :
            distance(0), originalEdges(0), id(0), shortcut(0), forward(0), backward(0) {}
        _ContractorEdgeData( unsigned _distance, unsigned _originalEdges, unsigned _id, bool _shortcut, bool _forward, bool _backward) :
            distance(_distance), id(_id), originalEdges(std::min((unsigned)1<<28, _originalEdges) ), shortcut(_shortcut), forward(_forward), backward(_backward), originalViaNodeID(false) {}
        unsigned distance;
        unsigned id;
        unsigned originalEdges:28;
        bool shortcut:1;
        bool forward:1;
        bool backward:1;
        bool originalViaNodeID:1;
    } data;

    struct _HeapData {
        short hop;
        bool target;
        _HeapData() : hop(0), target(false) {}
        _HeapData( short h, bool t ) : hop(h), target(t) {}
    };

    typedef DynamicGraph< _ContractorEdgeData > _DynamicGraph;
    typedef BinaryHeap< NodeID, NodeID, int, _HeapData > _Heap;
    typedef _DynamicGraph::InputEdge _ContractorEdge;

    struct _ThreadData {
        _Heap heap;
        std::vector< _ContractorEdge > insertedEdges;
        std::vector< NodeID > neighbours;
        _ThreadData( NodeID nodes ): heap( nodes ) {
        }
    };

    struct _PriorityData {
        int depth;
        _PriorityData() : depth(0) { }
    };

    struct _ContractionInformation {
        int edgesDeleted;
        int edgesAdded;
        int originalEdgesDeleted;
        int originalEdgesAdded;
        _ContractionInformation() : edgesDeleted(0), edgesAdded(0), originalEdgesDeleted(0), originalEdgesAdded(0) {}
    };

    struct _NodePartitionor {
        bool operator()( std::pair< NodeID, bool > & nodeData ) const {
            return !nodeData.second;
        }
    };

public:

    template<class ContainerT >
    Contractor( int nodes, ContainerT& inputEdges) {
        DeallocatingVector< _ContractorEdge > edges;

        typename ContainerT::deallocation_iterator diter = inputEdges.dbegin();
        typename ContainerT::deallocation_iterator dend  = inputEdges.dend();


        //BOOST_FOREACH(typename ContainerT::value_type & currentEdge, inputEdges) {
        for(; diter!=dend; ++diter) {
            _ContractorEdge edge;
            edge.source = diter->source();
            edge.target = diter->target();
            edge.data = _ContractorEdgeData( (std::max)((int)diter->weight(), 1 ),  1,  diter->id()/*,  currentEdge.getNameIDOfTurnTarget(),  currentEdge.turnInstruction()*/,  false,  diter->isForward(),  diter->isBackward());

            assert( edge.data.distance > 0 );
#ifndef NDEBUG
            if ( edge.data.distance > 24 * 60 * 60 * 10 ) {
                std::cout << "Edge Weight too large -> May lead to invalid CH" << std::endl;
                continue;
            }
#endif
            edges.push_back( edge );
            std::swap( edge.source, edge.target );
            edge.data.forward = diter->isBackward();
            edge.data.backward = diter->isForward();
            edges.push_back( edge );
        }
        //clear input vector and trim the current set of edges with the well-known swap trick
        inputEdges.clear();

        sort( edges.begin(), edges.end() );
        NodeID edge = 0;
        for ( NodeID i = 0; i < edges.size(); ) {
            const NodeID source = edges[i].source;
            const NodeID target = edges[i].target;
            const NodeID id = edges[i].data.id;
//            const short turnType = edges[i].data.turnInstruction;
            //remove eigenloops
            if ( source == target ) {
                i++;
                continue;
            }
            _ContractorEdge forwardEdge;
            _ContractorEdge backwardEdge;
            forwardEdge.source = backwardEdge.source = source;
            forwardEdge.target = backwardEdge.target = target;
            forwardEdge.data.forward = backwardEdge.data.backward = true;
            forwardEdge.data.backward = backwardEdge.data.forward = false;
            forwardEdge.data.shortcut = backwardEdge.data.shortcut = false;
            forwardEdge.data.id = backwardEdge.data.id = id;
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
                if ( ((int)forwardEdge.data.distance) != std::numeric_limits< int >::max() ) {
                    edges[edge++] = forwardEdge;
                }
                if ( (int)backwardEdge.data.distance != std::numeric_limits< int >::max() ) {
                    edges[edge++] = backwardEdge;
                }
            }
        }
        std::cout << "merged " << edges.size() - edge << " edges out of " << edges.size() << std::endl;
//        edges.resize( edge );

        _graph.reset( new _DynamicGraph( nodes, edges ) );
        edges.clear();
//        unsigned maxdegree = 0;
//        NodeID highestNode = 0;
//
//        for(unsigned i = 0; i < _graph->GetNumberOfNodes(); ++i) {
//            unsigned degree = _graph->EndEdges(i) - _graph->BeginEdges(i);
//            if(degree > maxdegree) {
//                maxdegree = degree;
//                highestNode = i;
//            }
//        }
//
//        INFO("edges at node with id " << highestNode << " has degree " << maxdegree);
//        for(unsigned i = _graph->BeginEdges(highestNode); i < _graph->EndEdges(highestNode); ++i) {
//            INFO(" ->(" << highestNode << "," << _graph->GetTarget(i) << "); via: " << _graph->GetEdgeData(i).via);
//        }

        //Create temporary file
        
        GetTemporaryFileName(temporaryEdgeStorageFilename);
        std::cout << "contractor finished initalization" << std::endl;
    }

    ~Contractor() {
        //Delete temporary file
        remove(temporaryEdgeStorageFilename.c_str());
    }

    void Run() {
        const NodeID numberOfNodes = _graph->GetNumberOfNodes();
        Percent p (numberOfNodes);

        const unsigned maxThreads = omp_get_max_threads();
        std::vector < _ThreadData* > threadData;
        for ( unsigned threadNum = 0; threadNum < maxThreads; ++threadNum ) {
            threadData.push_back( new _ThreadData( numberOfNodes ) );
        }
        std::cout << "Contractor is using " << maxThreads << " threads" << std::endl;

        NodeID numberOfContractedNodes = 0;
        std::vector< std::pair< NodeID, bool > > remainingNodes( numberOfNodes );
        std::vector< float > nodePriority( numberOfNodes );
        std::vector< _PriorityData > nodeData( numberOfNodes );

        //initialize the variables
#pragma omp parallel for schedule ( guided )
        for ( int x = 0; x < ( int ) numberOfNodes; ++x )
            remainingNodes[x].first = x;

        std::cout << "initializing elimination PQ ..." << std::flush;
#pragma omp parallel
        {
            _ThreadData* data = threadData[omp_get_thread_num()];
#pragma omp parallel for schedule ( guided )
            for ( int x = 0; x < ( int ) numberOfNodes; ++x ) {
                nodePriority[x] = _Evaluate( data, &nodeData[x], x );
            }
        }
        std::cout << "ok" << std::endl << "preprocessing ..." << std::flush;

        bool flushedContractor = false;
        while ( numberOfContractedNodes < numberOfNodes ) {
        	if(!flushedContractor && (numberOfContractedNodes > (numberOfNodes*0.65) ) ){
        	    DeallocatingVector<_ContractorEdge> newSetOfEdges; //this one is not explicitely cleared since it goes out of scope anywa
        		std::cout << " [flush " << numberOfContractedNodes << " nodes] " << std::flush;
        		
        		//Delete old heap data to free memory that we need for the coming operations
                for ( unsigned threadNum = 0; threadNum < maxThreads; threadNum++ ) {
                    delete threadData[threadNum];
                }
                threadData.clear();


        		//Create new priority array
        		std::vector<float> newNodePriority(remainingNodes.size());
        		//this map gives the old IDs from the new ones, necessary to get a consistent graph at the end of contraction
        		oldNodeIDFromNewNodeIDMap.resize(remainingNodes.size());
        		//this map gives the new IDs from the old ones, necessary to remap targets from the remaining graph
        		std::vector<NodeID> newNodeIDFromOldNodeIDMap(numberOfNodes, UINT_MAX);
        		
        		//build forward and backward renumbering map and remap ids in remainingNodes and Priorities.
        		for(unsigned newNodeID = 0; newNodeID < remainingNodes.size(); ++newNodeID) {
        			//create renumbering maps in both directions
        			oldNodeIDFromNewNodeIDMap[newNodeID] = remainingNodes[newNodeID].first;
        			newNodeIDFromOldNodeIDMap[remainingNodes[newNodeID].first] = newNodeID;
        			newNodePriority[newNodeID] = nodePriority[remainingNodes[newNodeID].first];
        			remainingNodes[newNodeID].first = newNodeID;
        		}
        		
        		//Write dummy number of edges to temporary file
        		std::ofstream temporaryEdgeStorage(temporaryEdgeStorageFilename.c_str(), std::ios::binary);
        		initialFilePosition = temporaryEdgeStorage.tellp();
        		unsigned numberOfTemporaryEdges = 0;
        		temporaryEdgeStorage.write((char*)&numberOfTemporaryEdges, sizeof(unsigned));

        		//walk over all nodes
        		for(unsigned i = 0; i < _graph->GetNumberOfNodes(); ++i) {
        		    //INFO("Restructuring node " << i << "|" << _graph->GetNumberOfNodes());
        		    const NodeID start = i;
        		    for(_DynamicGraph::EdgeIterator currentEdge = _graph->BeginEdges(start); currentEdge < _graph->EndEdges(start); ++currentEdge) {
        		        _DynamicGraph::EdgeData & data = _graph->GetEdgeData(currentEdge);
        		        const NodeID target = _graph->GetTarget(currentEdge);
        		        if(UINT_MAX == newNodeIDFromOldNodeIDMap[i] ){
        		            //Save edges of this node w/o renumbering.
        		            temporaryEdgeStorage.write((char*)&start,  sizeof(NodeID));
        		            temporaryEdgeStorage.write((char*)&target, sizeof(NodeID));
        		            temporaryEdgeStorage.write((char*)&data,   sizeof(_DynamicGraph::EdgeData));
        		            ++numberOfTemporaryEdges;
        		        }else {
                            //node is not yet contracted.
                            //add (renumbered) outgoing edges to new DynamicGraph.
        		            _ContractorEdge newEdge;
        		            newEdge.source = newNodeIDFromOldNodeIDMap[start];
        		            newEdge.target = newNodeIDFromOldNodeIDMap[target];
                            newEdge.data = data;
                            newEdge.data.originalViaNodeID = true;
        		            assert(UINT_MAX != newNodeIDFromOldNodeIDMap[start] );
        		            assert(UINT_MAX != newNodeIDFromOldNodeIDMap[target]);
        		            newSetOfEdges.push_back(newEdge);
        		        }
        		    }
        		}
        		//Note the number of temporarily stored edges
        		temporaryEdgeStorage.seekp(initialFilePosition);
        		temporaryEdgeStorage.write((char*)&numberOfTemporaryEdges, sizeof(unsigned));
        		temporaryEdgeStorage.close();
//        		INFO("Flushed " << numberOfTemporaryEdges << " edges to disk");

        		//Delete map from old NodeIDs to new ones.
        		std::vector<NodeID>().swap(newNodeIDFromOldNodeIDMap);

        		//Replace old priorities array by new one
        		nodePriority.swap(newNodePriority);
        		//Delete old nodePriority vector
        		std::vector<float>().swap(newNodePriority);
                //old Graph is removed
                _graph.reset();

                //create new graph
                std::sort(newSetOfEdges.begin(), newSetOfEdges.end());

                //int nodes, const ContainerT &graph
                _graph.reset( new _DynamicGraph(remainingNodes.size(), newSetOfEdges));
                newSetOfEdges.clear();
        		flushedContractor = true;

        		//INFO: MAKE SURE THIS IS THE LAST OPERATION OF THE FLUSH!
        		//reinitialize heaps and ThreadData objects with appropriate size
                for ( unsigned threadNum = 0; threadNum < maxThreads; ++threadNum ) {
                    threadData.push_back( new _ThreadData( _graph->GetNumberOfNodes() ) );
                }
        	}

            const int last = ( int ) remainingNodes.size();
#pragma omp parallel
            {
                //determine independent node set
                _ThreadData* const data = threadData[omp_get_thread_num()];
#pragma omp for schedule ( guided )
                for ( int i = 0; i < last; ++i ) {
                    const NodeID node = remainingNodes[i].first;
                    remainingNodes[i].second = _IsIndependent( nodePriority, nodeData, data, node );
                }
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
                    //nodePriority[x] = -1;
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
                    _ContractorEdge& edgeToInsert = data.insertedEdges[i];
                    const _DynamicGraph::EdgeIterator existingEdgeID = _graph->FindEdge(edgeToInsert.source, edgeToInsert.target);
                    if(existingEdgeID != _graph->EndEdges(edgeToInsert.source)) {
                        _DynamicGraph::EdgeData & existingEdgeData = _graph->GetEdgeData(existingEdgeID);
                        if((edgeToInsert.data.forward == existingEdgeData.forward) && (edgeToInsert.data.backward == existingEdgeData.backward) ){
                            if(existingEdgeData.distance <= edgeToInsert.data.distance) {
                                continue;
                            } else {
                                existingEdgeData.distance = edgeToInsert.data.distance;
                                continue;
                            }
                        } else if(existingEdgeData.forward && existingEdgeData.backward) {
                            //existing edge is bidirectional, new edge is not
                            if(existingEdgeData.distance > edgeToInsert.data.distance) {
                                //split existing edge, because edge to insert is smaller in the one direction it goes than the existing edge
                                existingEdgeData.forward =~ edgeToInsert.data.forward;
                                existingEdgeData.backward =~ edgeToInsert.data.backward;
                            } else {
                                continue;
                            }
                        } else {
                            //new edge is bidirectional, existing one is not
                            if(existingEdgeData.distance > edgeToInsert.data.distance) {
                                existingEdgeData = edgeToInsert.data;
                                continue;
                            } else {
                                //existing edge is lower distance, but unidirectional.
                                //split new edge before insertion, because it is only valid in one direction
//                                edgeToInsert.data.forward =~ existingEdgeData.forward;
//                                edgeToInsert.data.backward =~ existingEdgeData.backward;
                                //INFO("should not happen too often");
                            }
                        }

                    }
                    _graph->InsertEdge( edgeToInsert.source, edgeToInsert.target, edgeToInsert.data );
                }
                data.insertedEdges.clear();
            }
            //update priorities
#pragma omp parallel
            {
                _ThreadData* data = threadData[omp_get_thread_num()];
#pragma omp for schedule ( guided ) nowait
                for ( int position = firstIndependent ; position < last; ++position ) {
                    NodeID x = remainingNodes[position].first;
                    _UpdateNeighbours( nodePriority, nodeData, data, x );
                }
            }
            //remove contracted nodes from the pool
            numberOfContractedNodes += last - firstIndependent;
            remainingNodes.resize( firstIndependent );
            std::vector< std::pair< NodeID, bool > >( remainingNodes ).swap( remainingNodes );
//            unsigned maxdegree = 0;
//            unsigned avgdegree = 0;
//            unsigned mindegree = UINT_MAX;
//            unsigned quaddegree = 0;
//
//            for(unsigned i = 0; i < remainingNodes.size(); ++i) {
//                unsigned degree = _graph->EndEdges(remainingNodes[i].first) - _graph->BeginEdges(remainingNodes[i].first);
//                if(degree > maxdegree)
//                    maxdegree = degree;
//                if(degree < mindegree)
//                    mindegree = degree;
//
//                avgdegree += degree;
//                quaddegree += (degree*degree);
//            }
//
//            avgdegree /= std::max((unsigned)1,(unsigned)remainingNodes.size() );
//            quaddegree /= std::max((unsigned)1,(unsigned)remainingNodes.size() );
//
//            INFO("rest: " << remainingNodes.size() << ", max: " << maxdegree << ", min: " << mindegree << ", avg: " << avgdegree << ", quad: " << quaddegree);

            p.printStatus(numberOfContractedNodes);
        }
        for ( unsigned threadNum = 0; threadNum < maxThreads; threadNum++ ) {
            delete threadData[threadNum];
        }
    }

    template< class Edge >
    void GetEdges( DeallocatingVector< Edge >& edges ) {
        Percent p (_graph->GetNumberOfNodes());
        INFO("Getting edges of minimized graph");
        NodeID numberOfNodes = _graph->GetNumberOfNodes();
        if(oldNodeIDFromNewNodeIDMap.size()) {
        	for ( NodeID node = 0; node < numberOfNodes; ++node ) {
        	    p.printStatus(node);
        		for ( _DynamicGraph::EdgeIterator edge = _graph->BeginEdges( node ), endEdges = _graph->EndEdges( node ); edge < endEdges; ++edge ) {
        			const NodeID target = _graph->GetTarget( edge );
        			const _DynamicGraph::EdgeData& data = _graph->GetEdgeData( edge );
        			Edge newEdge;
        			newEdge.source = oldNodeIDFromNewNodeIDMap[node];
        			newEdge.target = oldNodeIDFromNewNodeIDMap[target];
        			assert(UINT_MAX != newEdge.source);
        			assert(UINT_MAX != newEdge.target);

        			newEdge.data.distance = data.distance;
        			newEdge.data.shortcut = data.shortcut;
        			if(!data.originalViaNodeID)
        				newEdge.data.id = oldNodeIDFromNewNodeIDMap[data.id];
        			else
        				newEdge.data.id = data.id;

        			assert(newEdge.data.id != UINT_MAX);
        			newEdge.data.forward = data.forward;
        			newEdge.data.backward = data.backward;
        			edges.push_back( newEdge );
        		}
        	}
        }
        INFO("Renumbered edges of minimized graph, freeing space");
        _graph.reset();
        std::vector<NodeID>().swap(oldNodeIDFromNewNodeIDMap);
        INFO("Loading temporary edges");

        std::ifstream temporaryEdgeStorage(temporaryEdgeStorageFilename.c_str(), std::ios::binary);
        //Also get the edges from temporary storage
        unsigned numberOfTemporaryEdges = 0;
        temporaryEdgeStorage.read((char*)&numberOfTemporaryEdges, sizeof(unsigned));
        //loads edges of graph before renumbering, no need for further numbering action.
        NodeID start;
        NodeID target;
        //edges.reserve(edges.size()+numberOfTemporaryEdges);
        _DynamicGraph::EdgeData data;
        for(unsigned i = 0; i < numberOfTemporaryEdges; ++i) {
        	temporaryEdgeStorage.read((char*)&start,  sizeof(NodeID));
        	temporaryEdgeStorage.read((char*)&target, sizeof(NodeID));
        	temporaryEdgeStorage.read((char*)&data,   sizeof(_DynamicGraph::EdgeData));
        	Edge newEdge;
        	newEdge.source =  start;
        	newEdge.target = target;
        	newEdge.data.distance = data.distance;
        	newEdge.data.shortcut = data.shortcut;
        	newEdge.data.id = data.id;
        	newEdge.data.forward = data.forward;
        	newEdge.data.backward = data.backward;
        	edges.push_back( newEdge );
        }
        temporaryEdgeStorage.close();
        INFO("CH has " << edges.size() << " edges");
    }

private:
    inline void _Dijkstra( const int maxDistance, const unsigned numTargets, const int maxNodes, const int hopLimit, _ThreadData* const data ){

        _Heap& heap = data->heap;

        int nodes = 0;
        unsigned targetsFound = 0;
        while ( heap.Size() > 0 ) {
            const NodeID node = heap.DeleteMin();
            const int distance = heap.GetKey( node );
            const short currentHop = heap.GetData( node ).hop+1;

            if ( ++nodes > maxNodes )
                return;
            //Destination settled?
            if ( distance > maxDistance )
                return;

            if ( heap.GetData( node ).target ) {
                ++targetsFound;
                if ( targetsFound >= numTargets )
                    return;
            }

            if(currentHop >= hopLimit)
                continue;

            //iterate over all edges of node
            for ( _DynamicGraph::EdgeIterator edge = _graph->BeginEdges( node ), endEdges = _graph->EndEdges( node ); edge != endEdges; ++edge ) {
                const _ContractorEdgeData& data = _graph->GetEdgeData( edge );
                if ( !data.forward )
                    continue;
                const NodeID to = _graph->GetTarget( edge );
                const int toDistance = distance + data.distance;

                //New Node discovered -> Add to Heap + Node Info Storage
                if ( !heap.WasInserted( to ) )
                    heap.Insert( to, toDistance, _HeapData(currentHop, false) );

                //Found a shorter Path -> Update distance
                else if ( toDistance < heap.GetKey( to ) ) {
                    heap.DecreaseKey( to, toDistance );
                    heap.GetData( to ).hop = currentHop;
                }
            }
        }
    }

    float _Evaluate( _ThreadData* const data, _PriorityData* const nodeData, NodeID node){
        _ContractionInformation stats;

        //perform simulated contraction
        _Contract< true> ( data, node, &stats );

        // Result will contain the priority
        float result;
        if ( stats.edgesDeleted == 0 || stats.originalEdgesDeleted == 0 )
                result = 1 * nodeData->depth;
        else
                result =  2 * ((( float ) stats.edgesAdded ) / stats.edgesDeleted ) + 4 * ((( float ) stats.originalEdgesAdded ) / stats.originalEdgesDeleted ) + 1 * nodeData->depth;
        assert( result >= 0 );
        return result;
    }

    template< bool Simulate >
    bool _Contract( _ThreadData* data, NodeID node, _ContractionInformation* stats = NULL ) {
        _Heap& heap = data->heap;
        int insertedEdgesSize = data->insertedEdges.size();
        std::vector< _ContractorEdge >& insertedEdges = data->insertedEdges;

        for ( _DynamicGraph::EdgeIterator inEdge = _graph->BeginEdges( node ), endInEdges = _graph->EndEdges( node ); inEdge != endInEdges; ++inEdge ) {
            const _ContractorEdgeData& inData = _graph->GetEdgeData( inEdge );
            const NodeID source = _graph->GetTarget( inEdge );
            if ( Simulate ) {
                assert( stats != NULL );
                ++stats->edgesDeleted;
                stats->originalEdgesDeleted += inData.originalEdges;
            }
            if ( !inData.backward )
                continue;

            heap.Clear();
            heap.Insert( source, 0, _HeapData() );
//            if ( node != source )
//                heap.Insert( node, inData.distance, _HeapData() );
            int maxDistance = 0;
            unsigned numTargets = 0;

            for ( _DynamicGraph::EdgeIterator outEdge = _graph->BeginEdges( node ), endOutEdges = _graph->EndEdges( node ); outEdge != endOutEdges; ++outEdge ) {
                const _ContractorEdgeData& outData = _graph->GetEdgeData( outEdge );
                if ( !outData.forward )
                    continue;
                const NodeID target = _graph->GetTarget( outEdge );
                const int pathDistance = inData.distance + outData.distance;
                maxDistance = std::max( maxDistance, pathDistance );
                if ( !heap.WasInserted( target ) ) {
                    heap.Insert( target, pathDistance, _HeapData( 0, true ) );
                    ++numTargets;
                } else if ( pathDistance < heap.GetKey( target ) ) {
                    heap.DecreaseKey( target, pathDistance );
                }
            }

            if( Simulate )
                _Dijkstra( maxDistance, numTargets, 1000, (true ? INT_MAX : 5), data );
            else
                _Dijkstra( maxDistance, numTargets, 2000, (true ? INT_MAX : 7), data );

            for ( _DynamicGraph::EdgeIterator outEdge = _graph->BeginEdges( node ), endOutEdges = _graph->EndEdges( node ); outEdge != endOutEdges; ++outEdge ) {
                const _ContractorEdgeData& outData = _graph->GetEdgeData( outEdge );
                if ( !outData.forward )
                    continue;
                const NodeID target = _graph->GetTarget( outEdge );
                const int pathDistance = inData.distance + outData.distance;
                const int distance = heap.GetKey( target );
                if ( pathDistance <= distance ) {
                    if ( Simulate ) {
                        assert( stats != NULL );
                        stats->edgesAdded+=2;
                        stats->originalEdgesAdded += 2* ( outData.originalEdges + inData.originalEdges );
                    } else {
                        _ContractorEdge newEdge;
                        newEdge.source = source;
                        newEdge.target = target;
                        newEdge.data = _ContractorEdgeData( pathDistance, outData.originalEdges + inData.originalEdges, node/*, 0, inData.turnInstruction*/, true, true, false);;
                        insertedEdges.push_back( newEdge );
                        std::swap( newEdge.source, newEdge.target );
                        newEdge.data.forward = false;
                        newEdge.data.backward = true;
                        insertedEdges.push_back( newEdge );
                    }
                }
            }
        }
        if ( !Simulate ) {
            for ( int i = insertedEdgesSize, iend = insertedEdges.size(); i < iend; ++i ) {
                bool found = false;
                for ( int other = i + 1 ; other < iend ; ++other ) {
                    if ( insertedEdges[other].source != insertedEdges[i].source )
                        continue;
                    if ( insertedEdges[other].target != insertedEdges[i].target )
                        continue;
                    if ( insertedEdges[other].data.distance != insertedEdges[i].data.distance )
                        continue;
                    if ( insertedEdges[other].data.shortcut != insertedEdges[i].data.shortcut )
                        continue;
                    insertedEdges[other].data.forward |= insertedEdges[i].data.forward;
                    insertedEdges[other].data.backward |= insertedEdges[i].data.backward;
                    found = true;
                    break;
                }
                if ( !found )
                    insertedEdges[insertedEdgesSize++] = insertedEdges[i];
            }
            insertedEdges.resize( insertedEdgesSize );
        }
        return true;
    }

    void _DeleteIncomingEdges( _ThreadData* data, NodeID node ) {
        std::vector< NodeID >& neighbours = data->neighbours;
        neighbours.clear();

        //find all neighbours
        for ( _DynamicGraph::EdgeIterator e = _graph->BeginEdges( node ) ; e < _graph->EndEdges( node ) ; ++e ) {
            const NodeID u = _graph->GetTarget( e );
            if ( u != node )
                neighbours.push_back( u );
        }
        //eliminate duplicate entries ( forward + backward edges )
        std::sort( neighbours.begin(), neighbours.end() );
        neighbours.resize( std::unique( neighbours.begin(), neighbours.end() ) - neighbours.begin() );

        for ( int i = 0, e = ( int ) neighbours.size(); i < e; ++i ) {
            //			const NodeID u = neighbours[i];
            _graph->DeleteEdgesTo( neighbours[i], node );
        }
    }

    bool _UpdateNeighbours( std::vector< float > & priorities, std::vector< _PriorityData > & nodeData, _ThreadData* const data, NodeID node) {
        std::vector< NodeID >& neighbours = data->neighbours;
        neighbours.clear();

        //find all neighbours
        for ( _DynamicGraph::EdgeIterator e = _graph->BeginEdges( node ) ; e < _graph->EndEdges( node ) ; ++e ) {
            const NodeID u = _graph->GetTarget( e );
            if ( u == node )
                continue;
            neighbours.push_back( u );
            nodeData[u].depth = (std::max)(nodeData[node].depth + 1, nodeData[u].depth );
        }
        //eliminate duplicate entries ( forward + backward edges )
        std::sort( neighbours.begin(), neighbours.end() );
        neighbours.resize( std::unique( neighbours.begin(), neighbours.end() ) - neighbours.begin() );

        int neighbourSize = ( int ) neighbours.size();
        for ( int i = 0, e = neighbourSize; i < e; ++i ) {
            const NodeID u = neighbours[i];
            priorities[u] = _Evaluate( data, &( nodeData )[u], u );
        }

        return true;
    }

    bool _IsIndependent( const std::vector< float >& priorities, const std::vector< _PriorityData >& nodeData, _ThreadData* const data, NodeID node ) {
        const double priority = priorities[node];

        std::vector< NodeID >& neighbours = data->neighbours;
        neighbours.clear();

        for ( _DynamicGraph::EdgeIterator e = _graph->BeginEdges( node ) ; e < _graph->EndEdges( node ) ; ++e ) {
            const NodeID target = _graph->GetTarget( e );
            if(node==target)
                continue;
            const double targetPriority = priorities[target];
            assert( targetPriority >= 0 );
            //found a neighbour with lower priority?
            if ( priority > targetPriority )
                return false;
            //tie breaking
              if ( priority == targetPriority && bias(node, target) ) {
                return false;
            }
            neighbours.push_back( target );
        }

        std::sort( neighbours.begin(), neighbours.end() );
        neighbours.resize( std::unique( neighbours.begin(), neighbours.end() ) - neighbours.begin() );

        //examine all neighbours that are at most 2 hops away
        for ( std::vector< NodeID >::const_iterator i = neighbours.begin(), lastNode = neighbours.end(); i != lastNode; ++i ) {
            const NodeID u = *i;

            for ( _DynamicGraph::EdgeIterator e = _graph->BeginEdges( u ) ; e < _graph->EndEdges( u ) ; ++e ) {
                const NodeID target = _graph->GetTarget( e );
                if(node==target)
                    continue;

                const double targetPriority = priorities[target];
                assert( targetPriority >= 0 );
                //found a neighbour with lower priority?
                if ( priority > targetPriority)
                    return false;
                //tie breaking
                if ( priority == targetPriority && bias(node, target) ) {
                    return false;
                }
            }
        }
        return true;
    }


    /**
     * This bias function takes up 22 assembly instructions in total on X86
     */
    inline bool bias(const NodeID a, const NodeID b) const {
        unsigned short hasha = fastHash(a);
        unsigned short hashb = fastHash(b);

        //The compiler optimizes that to conditional register flags but without branching statements!
        if(hasha != hashb)
            return hasha < hashb;
        return a < b;
    }

    boost::shared_ptr<_DynamicGraph> _graph;
    std::vector<_DynamicGraph::InputEdge> contractedEdges;
    std::string temporaryEdgeStorageFilename;
    std::vector<NodeID> oldNodeIDFromNewNodeIDMap;
    long initialFilePosition;

    XORFastHash fastHash;
};

#endif // CONTRACTOR_H_INCLUDED
