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


#include "EdgeBasedGraphFactory.h"
#include "../Util/ComputeAngle.h"

#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

#include <fstream>

EdgeBasedGraphFactory::EdgeBasedGraphFactory(
    int number_of_nodes,
    std::vector<ImportEdge> & input_edge_list,
    std::vector<NodeID> & barrier_node_list,
    std::vector<NodeID> & traffic_light_node_list,
    std::vector<TurnRestriction> & input_restrictions_list,
    std::vector<NodeInfo> & node_info_list,
    SpeedProfileProperties speed_profile
) : speed_profile(speed_profile),
    m_turn_restrictions_count(0),
    m_node_info_list(node_info_list)
{
	BOOST_FOREACH(const TurnRestriction & restriction, input_restrictions_list) {
        std::pair<NodeID, NodeID> restriction_source =
            std::make_pair(restriction.fromNode, restriction.viaNode);
        unsigned index;
        RestrictionMap::iterator restriction_iter;
        restriction_iter = m_restriction_map.find(restriction_source);
        if(restriction_iter == m_restriction_map.end()) {
            index = m_restriction_bucket_list.size();
            m_restriction_bucket_list.resize(index+1);
            m_restriction_map.emplace(restriction_source, index);
        } else {
            index = restriction_iter->second;
            //Map already contains an is_only_*-restriction
            if(m_restriction_bucket_list.at(index).begin()->second) {
                continue;
            } else if(restriction.flags.isOnly) {
                //We are going to insert an is_only_*-restriction. There can be only one.
                m_turn_restrictions_count -= m_restriction_bucket_list.at(index).size();
                m_restriction_bucket_list.at(index).clear();
            }
        }
        ++m_turn_restrictions_count;
        m_restriction_bucket_list.at(index).push_back(
            std::make_pair( restriction.toNode, restriction.flags.isOnly)
        );
    }

	m_barrier_nodes.insert(
        barrier_node_list.begin(),
        barrier_node_list.end()
    );

    m_traffic_lights.insert(
        traffic_light_node_list.begin(),
        traffic_light_node_list.end()
    );

    DeallocatingVector< NodeBasedEdge > edges_list;
    NodeBasedEdge edge;
    BOOST_FOREACH(const ImportEdge & import_edge, input_edge_list) {
        if(!import_edge.isForward()) {
            edge.source = import_edge.target();
            edge.target = import_edge.source();
            edge.data.backward = import_edge.isForward();
            edge.data.forward = import_edge.isBackward();
        } else {
            edge.source = import_edge.source();
            edge.target = import_edge.target();
            edge.data.forward = import_edge.isForward();
            edge.data.backward = import_edge.isBackward();
        }
        if(edge.source == edge.target) {
        	continue;
        }

        edge.data.distance = (std::max)((int)import_edge.weight(), 1 );
        BOOST_ASSERT( edge.data.distance > 0 );
        edge.data.shortcut = false;
        edge.data.roundabout = import_edge.isRoundabout();
        edge.data.ignoreInGrid = import_edge.ignoreInGrid();
        edge.data.nameID = import_edge.name();
        edge.data.type = import_edge.type();
        edge.data.isAccessRestricted = import_edge.isAccessRestricted();
        edge.data.contraFlow = import_edge.isContraFlow();
        edges_list.push_back( edge );

        std::swap( edge.source, edge.target );
        edge.data.SwapDirectionFlags();
        edges_list.push_back( edge );
    }

    std::vector<ImportEdge>().swap(input_edge_list);
    std::sort( edges_list.begin(), edges_list.end() );

    m_node_based_graph = boost::make_shared<NodeBasedDynamicGraph>(
        number_of_nodes,
        edges_list
    );
    DeallocatingVector<NodeBasedEdge>().swap(edges_list);
    BOOST_ASSERT(0 == edges_list.size() );
}

void EdgeBasedGraphFactory::FixupArrivingTurnRestriction(
    const NodeID u,
    const NodeID v,
    const NodeID w
) {
    BOOST_ASSERT( u != UINT_MAX );
    BOOST_ASSERT( v != UINT_MAX );
    BOOST_ASSERT( w != UINT_MAX );

    std::vector<NodeID> predecessors;
    for(
        EdgeID current_edge_id = m_node_based_graph->BeginEdges(u);
        current_edge_id < m_node_based_graph->EndEdges(u);
        ++current_edge_id
    ) {
        const EdgeData & edge_data = m_node_based_graph->GetEdgeData(current_edge_id);
        const NodeID target = m_node_based_graph->GetTarget(current_edge_id);
        if( edge_data.backward && ( v != target) ) {
            predecessors.push_back(target);
        }
    }
    BOOST_FOREACH( const NodeID x, predecessors ) {
        const std::pair<NodeID, NodeID> restr_start = std::make_pair(x,u);
        RestrictionMap::const_iterator restriction_iterator;
        restriction_iterator = m_restriction_map.find( restr_start );
        if( restriction_iterator != m_restriction_map.end() ) {
            const unsigned index = restriction_iterator->second;
            BOOST_FOREACH(
                RestrictionTarget & restriction_target,
                m_restriction_bucket_list.at(index)
            ) {
                if( v == restriction_target.first ) {
                    restriction_target.first = w;
                }
            }
        }
    }
}

void EdgeBasedGraphFactory::FixupStartingTurnRestriction(
    const NodeID u,
    const NodeID v,
    const NodeID w
) {
    BOOST_ASSERT( u != UINT_MAX );
    BOOST_ASSERT( v != UINT_MAX );
    BOOST_ASSERT( w != UINT_MAX );

    const std::pair<NodeID, NodeID> old_start = std::make_pair(v,w);
    RestrictionMap::const_iterator restriction_iterator;
    restriction_iterator = m_restriction_map.find( old_start );
    if( restriction_iterator != m_restriction_map.end() ) {
        const unsigned index = restriction_iterator->second;
        // remove old restriction start (v,w)
        m_restriction_map.erase( restriction_iterator );

        // insert new restriction start (u,w) (point to index)
        const std::pair<NodeID, NodeID> new_start = std::make_pair(u,w);
        m_restriction_map.insert( std::make_pair(new_start, index) );
    }
}

void EdgeBasedGraphFactory::GetEdgeBasedEdges(
    DeallocatingVector< EdgeBasedEdge >& output_edge_list
) {
    BOOST_ASSERT_MSG(
        0 == output_edge_list.size(),
        "Vector is not empty"
    );
    m_edge_based_edge_list.swap(output_edge_list);
}

void EdgeBasedGraphFactory::GetEdgeBasedNodes( std::vector<EdgeBasedNode> & nodes) {
#ifndef NDEBUG
    BOOST_FOREACH(const EdgeBasedNode & node, m_edge_based_node_list){
        BOOST_ASSERT(node.lat1 != INT_MAX); BOOST_ASSERT(node.lon1 != INT_MAX);
        BOOST_ASSERT(node.lat2 != INT_MAX); BOOST_ASSERT(node.lon2 != INT_MAX);
    }
#endif
    nodes.swap(m_edge_based_node_list);
}

NodeID EdgeBasedGraphFactory::CheckForEmanatingIsOnlyTurn(
    const NodeID u,
    const NodeID v
) const {
    BOOST_ASSERT( u != UINT_MAX );
    BOOST_ASSERT( v != UINT_MAX );
    const std::pair<NodeID, NodeID> restriction_source = std::make_pair(u, v);
    RestrictionMap::const_iterator restriction_iter;
    restriction_iter = m_restriction_map.find(restriction_source);
    if (restriction_iter != m_restriction_map.end()) {
        const unsigned index = restriction_iter->second;
        BOOST_FOREACH(
            const RestrictionSource & restriction_target,
            m_restriction_bucket_list.at(index)
        ) {
            if(restriction_target.second) {
                return restriction_target.first;
            }
        }
    }
    return UINT_MAX;
}

bool EdgeBasedGraphFactory::CheckIfTurnIsRestricted(
    const NodeID u,
    const NodeID v,
    const NodeID w
) const {
    BOOST_ASSERT( u != UINT_MAX );
    BOOST_ASSERT( v != UINT_MAX );
    BOOST_ASSERT( w != UINT_MAX );

    const std::pair<NodeID, NodeID> restriction_source = std::make_pair(u, v);
    RestrictionMap::const_iterator restriction_iter;
    restriction_iter = m_restriction_map.find(restriction_source);
    if (restriction_iter != m_restriction_map.end()) {
        const unsigned index = restriction_iter->second;
        BOOST_FOREACH(
            const RestrictionTarget & restriction_target,
            m_restriction_bucket_list.at(index)
        ) {
            if(
                ( w == restriction_target.first ) && // target found
                ( !restriction_target.second    )    // and not an only_-restr.
            ) {
                return true;
            }
        }
    }
    return false;
}

void EdgeBasedGraphFactory::InsertEdgeBasedNode(
    EdgeIterator e1,
    NodeIterator u,
    NodeIterator v,
    bool belongsToTinyComponent
) {
    BOOST_ASSERT( u  != UINT_MAX );
    BOOST_ASSERT( v  != UINT_MAX );
    BOOST_ASSERT( e1 != UINT_MAX );
    EdgeData & data = m_node_based_graph->GetEdgeData(e1);
    EdgeBasedNode currentNode;
    currentNode.nameID = data.nameID;
    currentNode.lat1 = m_node_info_list[u].lat;
    currentNode.lon1 = m_node_info_list[u].lon;
    currentNode.lat2 = m_node_info_list[v].lat;
    currentNode.lon2 = m_node_info_list[v].lon;
    if( m_geometry_compressor.HasEntryForID(e1) ) {
        //reconstruct geometry and put in each individual edge with its offset
    }
    currentNode.belongsToTinyComponent = belongsToTinyComponent;
    currentNode.id = data.edgeBasedNodeID;
    currentNode.ignoreInGrid = data.ignoreInGrid;
    currentNode.weight = data.distance;
    m_edge_based_node_list.push_back(currentNode);
}


void EdgeBasedGraphFactory::FlushVectorToStream(
    std::ofstream & edge_data_file,
    std::vector<OriginalEdgeData> & original_edge_data_vector
) const {
    edge_data_file.write(
        (char*)&(original_edge_data_vector[0]),
        original_edge_data_vector.size()*sizeof(OriginalEdgeData)
    );
    original_edge_data_vector.clear();
}

void EdgeBasedGraphFactory::Run(
    const std::string & original_edge_data_filename,
    const std::string & geometry_filename,
    lua_State *lua_state
) {
    SimpleLogger().Write() << "Removing graph geometry while preserving topology";

    const unsigned original_number_of_nodes = m_node_based_graph->GetNumberOfNodes();
    const unsigned original_number_of_edges = m_node_based_graph->GetNumberOfEdges();

    SimpleLogger().Write(logDEBUG) << "Input graph has " << original_number_of_nodes << " nodes and " << original_number_of_edges << " edges";
    Percent p(original_number_of_nodes);
    unsigned removed_node_count = 0;
    for( NodeID v = 0; v < original_number_of_nodes; ++v ) {
        p.printStatus(v);

        // only contract degree 2 vertices
        if( 2 != m_node_based_graph->GetOutDegree(v) ) {
            continue;
        }

        // don't contract barrier node
        if( m_barrier_nodes.end() != m_barrier_nodes.find(v) ) {
            continue;
        }

        const bool reverse_edge_order = !(m_node_based_graph->GetEdgeData(m_node_based_graph->BeginEdges(v)).forward);
        const EdgeIterator forward_e2 = m_node_based_graph->BeginEdges(v) + reverse_edge_order;
        BOOST_ASSERT( UINT_MAX != forward_e2 );
        const EdgeIterator reverse_e2 = m_node_based_graph->BeginEdges(v) + 1 - reverse_edge_order;
        BOOST_ASSERT( UINT_MAX != reverse_e2 );

        const EdgeData & fwd_edge_data2 = m_node_based_graph->GetEdgeData(forward_e2);
        const EdgeData & rev_edge_data2 = m_node_based_graph->GetEdgeData(reverse_e2);

        const NodeIterator w = m_node_based_graph->GetTarget(forward_e2);
        BOOST_ASSERT( UINT_MAX != w );
        BOOST_ASSERT( v != w );
        const NodeIterator u = m_node_based_graph->GetTarget(reverse_e2);
        BOOST_ASSERT( UINT_MAX != u );
        BOOST_ASSERT( u != v );

        const EdgeIterator forward_e1 = m_node_based_graph->FindEdge(u, v);
        BOOST_ASSERT( UINT_MAX != forward_e1 );
        BOOST_ASSERT( v == m_node_based_graph->GetTarget(forward_e1));
        const EdgeIterator reverse_e1 = m_node_based_graph->FindEdge(w, v);
        BOOST_ASSERT( UINT_MAX != reverse_e1 );
        BOOST_ASSERT( v == m_node_based_graph->GetTarget(reverse_e1));

        const EdgeData & fwd_edge_data1 = m_node_based_graph->GetEdgeData(forward_e1);
        const EdgeData & rev_edge_data1 = m_node_based_graph->GetEdgeData(reverse_e1);

        if( m_node_based_graph->FindEdge(u, w) != m_node_based_graph->EndEdges(u) ||
            m_node_based_graph->FindEdge(w, u) != m_node_based_graph->EndEdges(w)
        ) {
            continue;
        }

        if (
            fwd_edge_data1.IsEqualTo(fwd_edge_data2) &&
            rev_edge_data1.IsEqualTo(rev_edge_data2)
        ) {
            // extend e1's to targets of e2's
            m_node_based_graph->SetTarget(forward_e1, w);
            m_node_based_graph->SetTarget(reverse_e1, u);

            // add weight of e2's to e1
            m_node_based_graph->GetEdgeData(forward_e1).distance += fwd_edge_data2.distance;
            m_node_based_graph->GetEdgeData(reverse_e1).distance += rev_edge_data2.distance;

            // remove e2's (if bidir, otherwise only one)
            m_node_based_graph->DeleteEdge(v, forward_e2);
            m_node_based_graph->DeleteEdge(v, reverse_e2);

            // update any involved turn restrictions
            FixupStartingTurnRestriction( u, v, w );
            FixupArrivingTurnRestriction( u, v, w );

            FixupStartingTurnRestriction( w, v, u );
            FixupArrivingTurnRestriction( w, v, u );

            //TODO: store compressed geometry in container
            m_geometry_compressor.CompressEdge( forward_e1, forward_e2, v );
            m_geometry_compressor.CompressEdge( reverse_e1, reverse_e2, v );

            ++removed_node_count;
        }
    }
    SimpleLogger().Write() << "removed " << removed_node_count << " nodes";
    m_geometry_compressor.PrintStatistics();

    unsigned new_node_count = 0;
    unsigned new_edge_count = 0;
    for( unsigned i = 0; i < m_node_based_graph->GetNumberOfNodes(); ++i ) {
        if( m_node_based_graph->GetOutDegree(i) > 0 ) {
            ++new_node_count;
            new_edge_count += (m_node_based_graph->EndEdges(i) - m_node_based_graph->BeginEdges(i));
        }
    }
    SimpleLogger().Write() << "new nodes: " << new_node_count << ", edges " << new_edge_count;
    SimpleLogger().Write() << "Node compression ratio: " << new_node_count/(double)original_number_of_nodes;
    SimpleLogger().Write() << "Edge compression ratio: " << new_edge_count/(double)original_number_of_edges;

    //Extract routing graph
    DeallocatingVector<NodeBasedEdge> edges_list;
    NodeBasedEdge new_edge;
    for(NodeID source = 0; source < m_node_based_graph->GetNumberOfNodes(); ++source) {
        for(
            EdgeID current_edge_id = m_node_based_graph->BeginEdges(source);
            current_edge_id < m_node_based_graph->EndEdges(source);
            ++current_edge_id
        ) {
            const NodeID target = m_node_based_graph->GetTarget(current_edge_id);
            const EdgeData & edge_data = m_node_based_graph->GetEdgeData(current_edge_id);
            if( source > target ) {
                continue;
            }
            if( edge_data.forward) {
                new_edge.source = source;
                new_edge.target = target;
                new_edge.data = edge_data;
                new_edge.data.edgeBasedNodeID = edges_list.size();
                edges_list.push_back(new_edge);
            }

            const EdgeID reverse_edge_id = m_node_based_graph->FindEdge(target, source);
            BOOST_ASSERT( reverse_edge_id != m_node_based_graph->EndEdges(target));

            const EdgeData & reverse_edge_data = m_node_based_graph->GetEdgeData(reverse_edge_id);
            if( reverse_edge_data.forward ) {
                new_edge.source = target;
                new_edge.target = source;
                new_edge.data = reverse_edge_data;
                new_edge.data.edgeBasedNodeID = edges_list.size();
                edges_list.push_back(new_edge);
            }
        }
    }
    m_node_based_graph.reset();

    std::sort( edges_list.begin(), edges_list.end() );

    //Instantiate routing graph
    m_node_based_graph = boost::make_shared<NodeBasedDynamicGraph>(
        original_number_of_nodes,
        edges_list
    );

    DeallocatingVector<NodeBasedEdge>().swap(edges_list);
    BOOST_ASSERT(0 == edges_list.size() );

    SimpleLogger().Write() << "Identifying components of the road network";

    unsigned node_based_edge_counter = 0;
    unsigned original_edges_counter  = 0;

    std::ofstream edge_data_file(
        original_edge_data_filename.c_str(),
        std::ios::binary
    );

    //writes a dummy value that is updated later
    edge_data_file.write(
        (char*)&original_edges_counter,
        sizeof(unsigned)
    );

    //Run a BFS on the undirected graph and identify small components
    std::vector<unsigned> component_index_list;
    std::vector<NodeID> component_index_size;
    BFSCompentExplorer( component_index_list, component_index_size);

    SimpleLogger().Write() <<
        "identified: " << component_index_size.size() << " many components";
    SimpleLogger().Write() <<
        "generating edge-expanded nodes";

    p.reinit(m_node_based_graph->GetNumberOfNodes());
    //loop over all edges and generate new set of nodes
    for(
        NodeIterator u = 0, end = m_node_based_graph->GetNumberOfNodes();
        u < end;
        ++u
     ) {
        p.printIncrement();
        for(
            EdgeIterator e1 = m_node_based_graph->BeginEdges(u),
                last_edge = m_node_based_graph->EndEdges(u);
            e1 < last_edge;
            ++e1
        ) {
            NodeIterator v = m_node_based_graph->GetTarget(e1);

            if(m_node_based_graph->GetEdgeData(e1).type != SHRT_MAX) {
                BOOST_ASSERT_MSG(e1 != UINT_MAX, "edge id invalid");
                BOOST_ASSERT_MSG(u != UINT_MAX,  "souce node invalid");
                BOOST_ASSERT_MSG(v != UINT_MAX,  "target node invalid");
            //Note: edges that end on barrier nodes or on a turn restriction
            //may actually be in two distinct components. We choose the smallest
                const unsigned size_of_component = std::min(
                    component_index_size[component_index_list[u]],
                    component_index_size[component_index_list[v]]
                );

                InsertEdgeBasedNode( e1, u, v, size_of_component < 1000 );
            }
        }
    }

    SimpleLogger().Write()
        << "Generated " << m_edge_based_node_list.size() << " nodes in " <<
        "edge-expanded graph";
    SimpleLogger().Write() << "generating edge-expanded edges";

    std::vector<NodeID>().swap(component_index_size);
    BOOST_ASSERT_MSG(
        0 == component_index_size.capacity(),
        "component size vector not deallocated"
    );
    std::vector<NodeID>().swap(component_index_list);
    BOOST_ASSERT_MSG(
        0 == component_index_list.capacity(),
        "component index vector not deallocated"
    );
    std::vector<OriginalEdgeData> original_edge_data_vector;
    original_edge_data_vector.reserve(10000);

    //Loop over all turns and generate new set of edges.
    //Three nested loop look super-linear, but we are dealing with a (kind of)
    //linear number of turns only.
    unsigned restricted_turns_counter = 0;
    unsigned skipped_uturns_counter = 0;
    unsigned skipped_barrier_turns_counter = 0;
    p.reinit(m_node_based_graph->GetNumberOfNodes());
    for(
        NodeIterator u = 0, end = m_node_based_graph->GetNumberOfNodes();
        u < end;
        ++u
    ) {
        for(
            EdgeIterator e1 = m_node_based_graph->BeginEdges(u),
                last_edge_u = m_node_based_graph->EndEdges(u);
            e1 < last_edge_u;
            ++e1
        ) {
            ++node_based_edge_counter;
            const NodeIterator v = m_node_based_graph->GetTarget(e1);
            const NodeID to_node_of_only_restriction = CheckForEmanatingIsOnlyTurn(u, v);
            const bool is_barrier_node = ( m_barrier_nodes.find(v) != m_barrier_nodes.end() );

            for(
                EdgeIterator e2 = m_node_based_graph->BeginEdges(v),
                    last_edge_v = m_node_based_graph->EndEdges(v);
                    e2 < last_edge_v;
                    ++e2
            ) {
                const NodeIterator w = m_node_based_graph->GetTarget(e2);
                if(
                    to_node_of_only_restriction != UINT_MAX &&
                    w != to_node_of_only_restriction
                ) {
                    //We are at an only_-restriction but not at the right turn.
                    ++restricted_turns_counter;
                    continue;
                }

                if( is_barrier_node) {
                    if(u != w) {
                        ++skipped_barrier_turns_counter;
                        continue;
                    }
                } else {
                    if ( (u == w) && (m_node_based_graph->GetOutDegree(v) > 1) ) {
                        ++skipped_uturns_counter;
                        continue;
                    }
                }

                //only add an edge if turn is not a U-turn except when it is
                //at the end of a dead-end street
                if (
                    CheckIfTurnIsRestricted(u, v, w)          &&
                    (to_node_of_only_restriction == UINT_MAX) &&
                    (w != to_node_of_only_restriction)
                ) {
                    ++restricted_turns_counter;
                    continue;
                }

                //only add an edge if turn is not prohibited
                const EdgeData & edge_data1 = m_node_based_graph->GetEdgeData(e1);
                const EdgeData & edge_data2 = m_node_based_graph->GetEdgeData(e2);

                BOOST_ASSERT(
                    edge_data1.edgeBasedNodeID < m_node_based_graph->GetNumberOfEdges()
                );
                BOOST_ASSERT(
                    edge_data2.edgeBasedNodeID < m_node_based_graph->GetNumberOfEdges()
                );
                BOOST_ASSERT(
                    edge_data1.edgeBasedNodeID != edge_data2.edgeBasedNodeID
                );
                BOOST_ASSERT( edge_data1.forward );
                BOOST_ASSERT( edge_data2.forward );

                // the following is the core of the loop.
                unsigned distance = edge_data1.distance;
                if( m_traffic_lights.find(v) != m_traffic_lights.end() ) {
                    distance += speed_profile.trafficSignalPenalty;
                }
                const int turn_penalty = GetTurnPenalty(u, v, w, lua_state);
                TurnInstruction turnInstruction = AnalyzeTurn(u, v, w);
                if(turnInstruction == TurnInstructions.UTurn){
                    distance += speed_profile.uTurnPenalty;
                }
                distance += turn_penalty;

                const bool edge_is_compressed = m_geometry_compressor.HasEntryForID(e1);
                if(edge_is_compressed) {
                    m_geometry_compressor.AddNodeIDToCompressedEdge(e1, v);
                }

                original_edge_data_vector.push_back(
                    OriginalEdgeData(
                        edge_is_compressed ? m_geometry_compressor.GetPositionForID(e1) : v,
                        edge_data2.nameID,
                        turnInstruction,
                        edge_is_compressed
                    )
                );
                ++original_edges_counter;

                if(original_edge_data_vector.size() > 100000) {
                    FlushVectorToStream(
                        edge_data_file,
                        original_edge_data_vector
                    );
                }

                m_edge_based_edge_list.push_back(
                    EdgeBasedEdge(
                        edge_data1.edgeBasedNodeID,
                        edge_data2.edgeBasedNodeID,
                        m_edge_based_edge_list.size(),
                        distance,
                        true,
                        false
                    )
                );
            }
        }
        p.printIncrement();
    }
    FlushVectorToStream( edge_data_file, original_edge_data_vector );

    edge_data_file.seekp( std::ios::beg );
    edge_data_file.write( (char*)&original_edges_counter, sizeof(unsigned) );
    edge_data_file.close();

    m_geometry_compressor.SerializeInternalVector( geometry_filename );

    SimpleLogger().Write() <<
        "Generated " << m_edge_based_node_list.size() << " edge based nodes";
    SimpleLogger().Write() <<
        "Node-based graph contains " << node_based_edge_counter << " edges";
    SimpleLogger().Write() <<
        "Edge-expanded graph ...";
    SimpleLogger().Write() <<
        "  contains " << m_edge_based_edge_list.size() << " edges";
    SimpleLogger().Write() <<
        "  skips "  << restricted_turns_counter << " turns, "
        "defined by " << m_turn_restrictions_count << " restrictions";
    SimpleLogger().Write() <<
        "  skips "  << skipped_uturns_counter << " U turns";
    SimpleLogger().Write() <<
        "  skips "  <<  skipped_barrier_turns_counter << " turns over barriers";
}

int EdgeBasedGraphFactory::GetTurnPenalty(
    const NodeID u,
    const NodeID v,
    const NodeID w,
    lua_State *lua_state
) const {
    const double angle = GetAngleBetweenThreeFixedPointCoordinates (
        m_node_info_list[u],
        m_node_info_list[v],
        m_node_info_list[w]
    );

    if( speed_profile.has_turn_penalty_function ) {
        try {
            //call lua profile to compute turn penalty
            return luabind::call_function<int>(
                lua_state,
                "turn_function",
                180.-angle
            );
        } catch (const luabind::error &er) {
            SimpleLogger().Write(logWARNING) << er.what();
        }
    }
    return 0;
}

TurnInstruction EdgeBasedGraphFactory::AnalyzeTurn(
    const NodeID u,
    const NodeID v,
    const NodeID w
) const {
    if(u == w) {
        return TurnInstructions.UTurn;
    }

    const EdgeIterator edge1 = m_node_based_graph->FindEdge(u, v);
    const EdgeIterator edge2 = m_node_based_graph->FindEdge(v, w);

    const EdgeData & data1 = m_node_based_graph->GetEdgeData(edge1);
    const EdgeData & data2 = m_node_based_graph->GetEdgeData(edge2);

    if(!data1.contraFlow && data2.contraFlow) {
    	return TurnInstructions.EnterAgainstAllowedDirection;
    }
    if(data1.contraFlow && !data2.contraFlow) {
    	return TurnInstructions.LeaveAgainstAllowedDirection;
    }

    //roundabouts need to be handled explicitely
    if(data1.roundabout && data2.roundabout) {
        //Is a turn possible? If yes, we stay on the roundabout!
        if( 1 == m_node_based_graph->GetOutDegree(v) ) {
            //No turn possible.
            return TurnInstructions.NoTurn;
        }
        return TurnInstructions.StayOnRoundAbout;
    }
    //Does turn start or end on roundabout?
    if(data1.roundabout || data2.roundabout) {
        //We are entering the roundabout
        if( (!data1.roundabout) && data2.roundabout) {
            return TurnInstructions.EnterRoundAbout;
        }
        //We are leaving the roundabout
        if(data1.roundabout && (!data2.roundabout) ) {
            return TurnInstructions.LeaveRoundAbout;
        }
    }

    //If street names stay the same and if we are certain that it is not a
    //a segment of a roundabout, we skip it.
    if( data1.nameID == data2.nameID ) {
        //TODO: Here we should also do a small graph exploration to check for
        //      more complex situations
        if( 0 != data1.nameID ) {
            return TurnInstructions.NoTurn;
        } else if (m_node_based_graph->GetOutDegree(v) <= 2) {
            return TurnInstructions.NoTurn;
        }
    }

    const double angle = GetAngleBetweenThreeFixedPointCoordinates (
        m_node_info_list[u],
        m_node_info_list[v],
        m_node_info_list[w]
    );
    return TurnInstructions.GetTurnDirectionOfInstruction(angle);
}

unsigned EdgeBasedGraphFactory::GetNumberOfNodes() const {
    return m_node_based_graph->GetNumberOfEdges();
}

void EdgeBasedGraphFactory::BFSCompentExplorer(
    std::vector<unsigned> & component_index_list,
    std::vector<unsigned> & component_index_size
) const {
    std::queue<std::pair<NodeID, NodeID> > bfs_queue;
    Percent p( m_node_based_graph->GetNumberOfNodes() );
    unsigned current_component, current_component_size;
    current_component = current_component_size = 0;

    BOOST_ASSERT( component_index_list.empty() );
    BOOST_ASSERT( component_index_size.empty() );

    component_index_list.resize(
        m_node_based_graph->GetNumberOfNodes(),
        UINT_MAX
    );

    //put unexplorered node with parent pointer into queue
    for( NodeID node = 0, end = m_node_based_graph->GetNumberOfNodes(); node < end; ++node) {
        if(UINT_MAX == component_index_list[node]) {
            bfs_queue.push(std::make_pair(node, node));
            //mark node as read
            component_index_list[node] = current_component;
            p.printIncrement();
            while(!bfs_queue.empty()) {
                //fetch element from BFS queue
                std::pair<NodeID, NodeID> current_queue_item = bfs_queue.front();
                bfs_queue.pop();

                const NodeID v = current_queue_item.first;  //current node
                const NodeID u = current_queue_item.second; //parent
                //increment size counter of current component
                ++current_component_size;
                const bool is_barrier_node = (m_barrier_nodes.find(v) != m_barrier_nodes.end());
                if(!is_barrier_node) {
                    const NodeID to_node_of_only_restriction = CheckForEmanatingIsOnlyTurn(u, v);

                    for(
                        EdgeIterator e2 = m_node_based_graph->BeginEdges(v);
                        e2 < m_node_based_graph->EndEdges(v);
                        ++e2
                    ) {
                        NodeIterator w = m_node_based_graph->GetTarget(e2);

                        if(
                            to_node_of_only_restriction != UINT_MAX &&
                            w != to_node_of_only_restriction
                        ) {
                            // At an only_-restriction but not at the right turn
                            continue;
                        }
                        if( u != w ) {
                            //only add an edge if turn is not a U-turn except
                            //when it is at the end of a dead-end street.
                            if (!CheckIfTurnIsRestricted(u, v, w) ) {
                                //only add an edge if turn is not prohibited
                                if(UINT_MAX == component_index_list[w]) {
                                    //insert next (node, parent) only if w has
                                    //not yet been explored
                                    //mark node as read
                                    component_index_list[w] = current_component;
                                    bfs_queue.push(std::make_pair(w,v));
                                    p.printIncrement();
                                }
                            }
                        }
                    }
                }
            }
            //push size into vector
            component_index_size.push_back(current_component_size);
            //reset counters;
            current_component_size = 0;
            ++current_component;
        }
    }
}
