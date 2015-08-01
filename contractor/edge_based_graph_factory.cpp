/*

Copyright (c) 2015, Project OSRM, Dennis Luxen, others
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

#include "edge_based_graph_factory.hpp"
#include "../algorithms/tiny_components.hpp"
#include "../data_structures/percent.hpp"
#include "../util/compute_angle.hpp"
#include "../util/integer_range.hpp"
#include "../util/lua_util.hpp"
#include "../util/simple_logger.hpp"
#include "../util/timing_util.hpp"

#include <boost/assert.hpp>

#include <fstream>
#include <iomanip>
#include <limits>

EdgeBasedGraphFactory::EdgeBasedGraphFactory(std::shared_ptr<NodeBasedDynamicGraph> node_based_graph,
                                             const CompressedEdgeContainer& compressed_edge_container,
                                             const std::unordered_set<NodeID>& barrier_nodes,
                                             const std::unordered_set<NodeID>& traffic_lights,
                                             std::shared_ptr<const RestrictionMap> restriction_map,
                                             const std::vector<QueryNode> &node_info_list,
                                             const SpeedProfileProperties &speed_profile)
    : m_node_info_list(node_info_list),
      m_node_based_graph(node_based_graph),
      m_restriction_map(restriction_map),
      m_barrier_nodes(barrier_nodes),
      m_traffic_lights(traffic_lights),
      m_compressed_edge_container(compressed_edge_container),
      speed_profile(speed_profile)
{
}

void EdgeBasedGraphFactory::GetEdgeBasedEdges(DeallocatingVector<EdgeBasedEdge> &output_edge_list)
{
    BOOST_ASSERT_MSG(0 == output_edge_list.size(), "Vector is not empty");
    m_edge_based_edge_list.swap(output_edge_list);
}

void EdgeBasedGraphFactory::GetEdgeBasedNodes(std::vector<EdgeBasedNode> &nodes)
{
#ifndef NDEBUG
    for (const EdgeBasedNode &node : m_edge_based_node_list)
    {
        BOOST_ASSERT(m_node_info_list.at(node.u).lat != INT_MAX);
        BOOST_ASSERT(m_node_info_list.at(node.u).lon != INT_MAX);
        BOOST_ASSERT(m_node_info_list.at(node.v).lon != INT_MAX);
        BOOST_ASSERT(m_node_info_list.at(node.v).lat != INT_MAX);
    }
#endif
    nodes.swap(m_edge_based_node_list);
}

unsigned EdgeBasedGraphFactory::GetHighestEdgeID()
{
    return m_max_edge_id;
}

void EdgeBasedGraphFactory::InsertEdgeBasedNode(const NodeID node_u,
                                                const NodeID node_v,
                                                const unsigned component_id)
{
    // merge edges together into one EdgeBasedNode
    BOOST_ASSERT(node_u != SPECIAL_NODEID);
    BOOST_ASSERT(node_v != SPECIAL_NODEID);

    // find forward edge id and
    const EdgeID edge_id_1 = m_node_based_graph->FindEdge(node_u, node_v);
    BOOST_ASSERT(edge_id_1 != SPECIAL_EDGEID);

    const EdgeData &forward_data = m_node_based_graph->GetEdgeData(edge_id_1);

    // find reverse edge id and
    const EdgeID edge_id_2 = m_node_based_graph->FindEdge(node_v, node_u);
    BOOST_ASSERT(edge_id_2 != SPECIAL_EDGEID);

    const EdgeData &reverse_data = m_node_based_graph->GetEdgeData(edge_id_2);

    if (forward_data.edge_id == SPECIAL_NODEID &&
        reverse_data.edge_id == SPECIAL_NODEID)
    {
        return;
    }

    BOOST_ASSERT(m_compressed_edge_container.HasEntryForID(edge_id_1) ==
                 m_compressed_edge_container.HasEntryForID(edge_id_2));
    if (m_compressed_edge_container.HasEntryForID(edge_id_1))
    {
        BOOST_ASSERT(m_compressed_edge_container.HasEntryForID(edge_id_2));

        // reconstruct geometry and put in each individual edge with its offset
        const auto& forward_geometry = m_compressed_edge_container.GetBucketReference(edge_id_1);
        const auto& reverse_geometry = m_compressed_edge_container.GetBucketReference(edge_id_2);
        BOOST_ASSERT(forward_geometry.size() == reverse_geometry.size());
        BOOST_ASSERT(0 != forward_geometry.size());
        const unsigned geometry_size = static_cast<unsigned>(forward_geometry.size());
        BOOST_ASSERT(geometry_size > 1);

        // reconstruct bidirectional edge with individual weights and put each into the NN index

        std::vector<int> forward_dist_prefix_sum(forward_geometry.size(), 0);
        std::vector<int> reverse_dist_prefix_sum(reverse_geometry.size(), 0);

        // quick'n'dirty prefix sum as std::partial_sum needs addtional casts
        // TODO: move to lambda function with C++11
        int temp_sum = 0;

        for (const auto i : osrm::irange(0u, geometry_size))
        {
            forward_dist_prefix_sum[i] = temp_sum;
            temp_sum += forward_geometry[i].second;

            BOOST_ASSERT(forward_data.distance >= temp_sum);
        }

        temp_sum = 0;
        for (const auto i : osrm::irange(0u, geometry_size))
        {
            temp_sum += reverse_geometry[reverse_geometry.size() - 1 - i].second;
            reverse_dist_prefix_sum[i] = reverse_data.distance - temp_sum;
            // BOOST_ASSERT(reverse_data.distance >= temp_sum);
        }

        NodeID current_edge_source_coordinate_id = node_u;

        // traverse arrays from start and end respectively
        for (const auto i : osrm::irange(0u, geometry_size))
        {
            BOOST_ASSERT(current_edge_source_coordinate_id ==
                         reverse_geometry[geometry_size - 1 - i].first);
            const NodeID current_edge_target_coordinate_id = forward_geometry[i].first;
            BOOST_ASSERT(current_edge_target_coordinate_id != current_edge_source_coordinate_id);

            // build edges
            m_edge_based_node_list.emplace_back(
                forward_data.edge_id, reverse_data.edge_id,
                current_edge_source_coordinate_id, current_edge_target_coordinate_id,
                forward_data.name_id, forward_geometry[i].second,
                reverse_geometry[geometry_size - 1 - i].second, forward_dist_prefix_sum[i],
                reverse_dist_prefix_sum[i], m_compressed_edge_container.GetPositionForID(edge_id_1),
                component_id, i, forward_data.travel_mode, reverse_data.travel_mode);
            current_edge_source_coordinate_id = current_edge_target_coordinate_id;

            BOOST_ASSERT(m_edge_based_node_list.back().IsCompressed());

            BOOST_ASSERT(node_u != m_edge_based_node_list.back().u ||
                         node_v != m_edge_based_node_list.back().v);

            BOOST_ASSERT(node_u != m_edge_based_node_list.back().v ||
                         node_v != m_edge_based_node_list.back().u);
        }

        BOOST_ASSERT(current_edge_source_coordinate_id == node_v);
        BOOST_ASSERT(m_edge_based_node_list.back().IsCompressed());
    }
    else
    {
        BOOST_ASSERT(!m_compressed_edge_container.HasEntryForID(edge_id_2));

        if (forward_data.edge_id != SPECIAL_NODEID)
        {
            BOOST_ASSERT(!forward_data.reversed);
        }
        else
        {
            BOOST_ASSERT(forward_data.reversed);
        }

        if (reverse_data.edge_id != SPECIAL_NODEID)
        {
            BOOST_ASSERT(!reverse_data.reversed);
        }
        else
        {
            BOOST_ASSERT(reverse_data.reversed);
        }

        BOOST_ASSERT(forward_data.edge_id != SPECIAL_NODEID ||
                     reverse_data.edge_id != SPECIAL_NODEID);

        m_edge_based_node_list.emplace_back(
            forward_data.edge_id, reverse_data.edge_id, node_u, node_v,
            forward_data.name_id, forward_data.distance, reverse_data.distance, 0, 0, SPECIAL_EDGEID,
            component_id, 0, forward_data.travel_mode, reverse_data.travel_mode);
        BOOST_ASSERT(!m_edge_based_node_list.back().IsCompressed());
    }
}

void EdgeBasedGraphFactory::FlushVectorToStream(
    std::ofstream &edge_data_file, std::vector<OriginalEdgeData> &original_edge_data_vector) const
{
    if (original_edge_data_vector.empty())
    {
        return;
    }
    edge_data_file.write((char *)&(original_edge_data_vector[0]),
                         original_edge_data_vector.size() * sizeof(OriginalEdgeData));
    original_edge_data_vector.clear();
}

void EdgeBasedGraphFactory::Run(const std::string &original_edge_data_filename,
                                lua_State *lua_state)
{
    TIMER_START(renumber);
    m_max_edge_id = RenumberEdges() - 1;
    TIMER_STOP(renumber);

    TIMER_START(generate_nodes);
    GenerateEdgeExpandedNodes();
    TIMER_STOP(generate_nodes);

    TIMER_START(generate_edges);
    GenerateEdgeExpandedEdges(original_edge_data_filename, lua_state);
    TIMER_STOP(generate_edges);

    SimpleLogger().Write() << "Timing statistics for edge-expanded graph:";
    SimpleLogger().Write() << "Renumbering edges: " << TIMER_SEC(renumber) << "s";
    SimpleLogger().Write() << "Generating nodes: " << TIMER_SEC(generate_nodes) << "s";
    SimpleLogger().Write() << "Generating edges: " << TIMER_SEC(generate_edges) << "s";
}


/// Renumbers all _forward_ edges and sets the edge_id.
/// A specific numbering is not important. Any unique ID will do.
/// Returns the number of edge based nodes.
unsigned EdgeBasedGraphFactory::RenumberEdges()
{
    // renumber edge based node of outgoing edges
    unsigned numbered_edges_count = 0;
    for (const auto current_node : osrm::irange(0u, m_node_based_graph->GetNumberOfNodes()))
    {
        for (const auto current_edge : m_node_based_graph->GetAdjacentEdgeRange(current_node))
        {
            EdgeData &edge_data = m_node_based_graph->GetEdgeData(current_edge);

            // only number incoming edges
            if (edge_data.reversed)
            {
                continue;
            }

            BOOST_ASSERT(numbered_edges_count < m_node_based_graph->GetNumberOfEdges());
            edge_data.edge_id = numbered_edges_count;
            ++numbered_edges_count;

            BOOST_ASSERT(SPECIAL_NODEID != edge_data.edge_id);
        }
    }

    return numbered_edges_count;
}

/// Creates the nodes in the edge expanded graph from edges in the node-based graph.
void EdgeBasedGraphFactory::GenerateEdgeExpandedNodes()
{
    SimpleLogger().Write() << "Identifying components of the (compressed) road network";

    // Run a BFS on the undirected graph and identify small components
    TarjanSCC<NodeBasedDynamicGraph> component_explorer(m_node_based_graph, *m_restriction_map,
                                                        m_barrier_nodes);

    component_explorer.run();

    SimpleLogger().Write() << "identified: "
                           << component_explorer.get_number_of_components()
                           << " (compressed) components";
    SimpleLogger().Write() << "identified "
                           << component_explorer.get_size_one_count()
                           << " (compressed) SCCs of size 1";
    SimpleLogger().Write() << "generating edge-expanded nodes";

    Percent progress(m_node_based_graph->GetNumberOfNodes());

    // loop over all edges and generate new set of nodes
    for (const auto node_u : osrm::irange(0u, m_node_based_graph->GetNumberOfNodes()))
    {
        BOOST_ASSERT(node_u != SPECIAL_NODEID);
        BOOST_ASSERT(node_u < m_node_based_graph->GetNumberOfNodes());
        progress.printStatus(node_u);
        for (EdgeID e1 : m_node_based_graph->GetAdjacentEdgeRange(node_u))
        {
            const EdgeData &edge_data = m_node_based_graph->GetEdgeData(e1);
            BOOST_ASSERT(e1 != SPECIAL_EDGEID);
            const NodeID node_v = m_node_based_graph->GetTarget(e1);

            BOOST_ASSERT(SPECIAL_NODEID != node_v);
            // pick only every other edge, since we have every edge as an outgoing
            // and incoming egde
            if (node_u > node_v)
            {
                continue;
            }

            BOOST_ASSERT(node_u < node_v);

            // Note: edges that end on barrier nodes or on a turn restriction
            // may actually be in two distinct components. We choose the smallest
            const unsigned size_of_component =
                std::min(component_explorer.get_component_size(node_u),
                         component_explorer.get_component_size(node_v));

            const unsigned id_of_smaller_component = [node_u, node_v, &component_explorer]
            {
                if (component_explorer.get_component_size(node_u) <
                    component_explorer.get_component_size(node_v))
                {
                    return component_explorer.get_component_id(node_u);
                }
                return component_explorer.get_component_id(node_v);
            }();

            const bool component_is_tiny = size_of_component < 1000;

            // we only set edge_id for forward edges
            if (edge_data.edge_id == SPECIAL_NODEID)
            {
                InsertEdgeBasedNode(node_v, node_u,
                                    (component_is_tiny ? id_of_smaller_component + 1 : 0));
            }
            else
            {
                InsertEdgeBasedNode(node_u, node_v,
                                    (component_is_tiny ? id_of_smaller_component + 1 : 0));
            }
        }
    }

    SimpleLogger().Write() << "Generated " << m_edge_based_node_list.size()
                           << " nodes in edge-expanded graph";
}

/// Actually it also generates OriginalEdgeData and serializes them...
void EdgeBasedGraphFactory::GenerateEdgeExpandedEdges(
    const std::string &original_edge_data_filename, lua_State *lua_state)
{
    SimpleLogger().Write() << "generating edge-expanded edges";

    unsigned node_based_edge_counter = 0;
    unsigned original_edges_counter = 0;

    std::ofstream edge_data_file(original_edge_data_filename.c_str(), std::ios::binary);

    // writes a dummy value that is updated later
    edge_data_file.write((char *)&original_edges_counter, sizeof(unsigned));

    std::vector<OriginalEdgeData> original_edge_data_vector;
    original_edge_data_vector.reserve(1024 * 1024);

    // Loop over all turns and generate new set of edges.
    // Three nested loop look super-linear, but we are dealing with a (kind of)
    // linear number of turns only.
    unsigned restricted_turns_counter = 0;
    unsigned skipped_uturns_counter = 0;
    unsigned skipped_barrier_turns_counter = 0;
    unsigned compressed = 0;

    Percent progress(m_node_based_graph->GetNumberOfNodes());

    for (const auto node_u : osrm::irange(0u, m_node_based_graph->GetNumberOfNodes()))
    {
        progress.printStatus(node_u);
        for (const EdgeID e1 : m_node_based_graph->GetAdjacentEdgeRange(node_u))
        {
            if (m_node_based_graph->GetEdgeData(e1).reversed)
            {
                continue;
            }

            ++node_based_edge_counter;
            const NodeID node_v = m_node_based_graph->GetTarget(e1);
            const NodeID only_restriction_to_node =
                m_restriction_map->CheckForEmanatingIsOnlyTurn(node_u, node_v);
            const bool is_barrier_node = m_barrier_nodes.find(node_v) != m_barrier_nodes.end();

            for (const EdgeID e2 : m_node_based_graph->GetAdjacentEdgeRange(node_v))
            {
                if (m_node_based_graph->GetEdgeData(e2).reversed)
                {
                    continue;
                }
                const NodeID node_w = m_node_based_graph->GetTarget(e2);

                if ((only_restriction_to_node != SPECIAL_NODEID) &&
                    (node_w != only_restriction_to_node))
                {
                    // We are at an only_-restriction but not at the right turn.
                    ++restricted_turns_counter;
                    continue;
                }

                if (is_barrier_node)
                {
                    if (node_u != node_w)
                    {
                        ++skipped_barrier_turns_counter;
                        continue;
                    }
                }
                else
                {
                    if ((node_u == node_w) && (m_node_based_graph->GetOutDegree(node_v) > 1))
                    {
                        ++skipped_uturns_counter;
                        continue;
                    }
                }

                // only add an edge if turn is not a U-turn except when it is
                // at the end of a dead-end street
                if (m_restriction_map->CheckIfTurnIsRestricted(node_u, node_v, node_w) &&
                    (only_restriction_to_node == SPECIAL_NODEID) &&
                    (node_w != only_restriction_to_node))
                {
                    // We are at an only_-restriction but not at the right turn.
                    ++restricted_turns_counter;
                    continue;
                }

                // only add an edge if turn is not prohibited
                const EdgeData &edge_data1 = m_node_based_graph->GetEdgeData(e1);
                const EdgeData &edge_data2 = m_node_based_graph->GetEdgeData(e2);

                BOOST_ASSERT(edge_data1.edge_id != edge_data2.edge_id);
                BOOST_ASSERT(!edge_data1.reversed);
                BOOST_ASSERT(!edge_data2.reversed);

                // the following is the core of the loop.
                unsigned distance = edge_data1.distance;
                if (m_traffic_lights.find(node_v) != m_traffic_lights.end())
                {
                    distance += speed_profile.traffic_signal_penalty;
                }

                // unpack last node of first segment if packed
                const auto first_coordinate =
                    m_node_info_list[(m_compressed_edge_container.HasEntryForID(e1)
                                          ? m_compressed_edge_container.GetLastEdgeSourceID(e1)
                                          : node_u)];

                // unpack first node of second segment if packed
                const auto third_coordinate =
                    m_node_info_list[(m_compressed_edge_container.HasEntryForID(e2)
                                          ? m_compressed_edge_container.GetFirstEdgeTargetID(e2)
                                          : node_w)];

                const double turn_angle = ComputeAngle::OfThreeFixedPointCoordinates(
                    first_coordinate, m_node_info_list[node_v], third_coordinate);

                const int turn_penalty = GetTurnPenalty(turn_angle, lua_state);
                TurnInstruction turn_instruction = AnalyzeTurn(node_u, node_v, node_w, turn_angle);
                if (turn_instruction == TurnInstruction::UTurn)
                {
                    distance += speed_profile.u_turn_penalty;
                }
                distance += turn_penalty;

                const bool edge_is_compressed = m_compressed_edge_container.HasEntryForID(e1);

                if (edge_is_compressed)
                {
                    ++compressed;
                }

                original_edge_data_vector.emplace_back(
                    (edge_is_compressed ? m_compressed_edge_container.GetPositionForID(e1) : node_v),
                    edge_data1.name_id, turn_instruction, edge_is_compressed,
                    edge_data2.travel_mode);

                ++original_edges_counter;

                if (original_edge_data_vector.size() > 1024 * 1024 * 10)
                {
                    FlushVectorToStream(edge_data_file, original_edge_data_vector);
                }

                BOOST_ASSERT(SPECIAL_NODEID != edge_data1.edge_id);
                BOOST_ASSERT(SPECIAL_NODEID != edge_data2.edge_id);

                m_edge_based_edge_list.emplace_back(edge_data1.edge_id, edge_data2.edge_id,
                                  m_edge_based_edge_list.size(), distance, true, false);
            }
        }
    }
    FlushVectorToStream(edge_data_file, original_edge_data_vector);

    edge_data_file.seekp(std::ios::beg);
    edge_data_file.write((char *)&original_edges_counter, sizeof(unsigned));
    edge_data_file.close();

    SimpleLogger().Write() << "Generated " << m_edge_based_node_list.size() << " edge based nodes";
    SimpleLogger().Write() << "Node-based graph contains " << node_based_edge_counter << " edges";
    SimpleLogger().Write() << "Edge-expanded graph ...";
    SimpleLogger().Write() << "  contains " << m_edge_based_edge_list.size() << " edges";
    SimpleLogger().Write() << "  skips " << restricted_turns_counter << " turns, "
                                                                        "defined by "
                           << m_restriction_map->size() << " restrictions";
    SimpleLogger().Write() << "  skips " << skipped_uturns_counter << " U turns";
    SimpleLogger().Write() << "  skips " << skipped_barrier_turns_counter << " turns over barriers";
}

int EdgeBasedGraphFactory::GetTurnPenalty(double angle, lua_State *lua_state) const
{

    if (speed_profile.has_turn_penalty_function)
    {
        try
        {
            // call lua profile to compute turn penalty
            double penalty = luabind::call_function<double>(lua_state, "turn_function", 180. - angle);
            return static_cast<int>(penalty);
        }
        catch (const luabind::error &er)
        {
            SimpleLogger().Write(logWARNING) << er.what();
        }
    }
    return 0;
}

TurnInstruction EdgeBasedGraphFactory::AnalyzeTurn(const NodeID node_u,
                                                   const NodeID node_v,
                                                   const NodeID node_w,
                                                   const double angle) const
{
    if (node_u == node_w)
    {
        return TurnInstruction::UTurn;
    }

    const EdgeID edge1 = m_node_based_graph->FindEdge(node_u, node_v);
    const EdgeID edge2 = m_node_based_graph->FindEdge(node_v, node_w);

    const EdgeData &data1 = m_node_based_graph->GetEdgeData(edge1);
    const EdgeData &data2 = m_node_based_graph->GetEdgeData(edge2);

    // roundabouts need to be handled explicitely
    if (data1.roundabout && data2.roundabout)
    {
        // Is a turn possible? If yes, we stay on the roundabout!
        if (1 == m_node_based_graph->GetDirectedOutDegree(node_v))
        {
            // No turn possible.
            return TurnInstruction::NoTurn;
        }
        return TurnInstruction::StayOnRoundAbout;
    }
    // Does turn start or end on roundabout?
    if (data1.roundabout || data2.roundabout)
    {
        // We are entering the roundabout
        if ((!data1.roundabout) && data2.roundabout)
        {
            return TurnInstruction::EnterRoundAbout;
        }
        // We are leaving the roundabout
        if (data1.roundabout && (!data2.roundabout))
        {
            return TurnInstruction::LeaveRoundAbout;
        }
    }

    // If street names stay the same and if we are certain that it is not a
    // a segment of a roundabout, we skip it.
    if (data1.name_id == data2.name_id)
    {
        // TODO: Here we should also do a small graph exploration to check for
        //      more complex situations
        if (0 != data1.name_id || m_node_based_graph->GetOutDegree(node_v) <= 2)
        {
            return TurnInstruction::NoTurn;
        }
    }

    return TurnInstructionsClass::GetTurnDirectionOfInstruction(angle);
}

