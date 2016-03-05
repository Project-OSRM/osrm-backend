#include "extractor/edge_based_edge.hpp"
#include "extractor/edge_based_graph_factory.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/percent.hpp"
#include "util/integer_range.hpp"
#include "util/lua_util.hpp"
#include "util/simple_logger.hpp"
#include "util/timing_util.hpp"
#include "util/exception.hpp"

#include "extractor/guidance/toolkit.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace osrm
{
namespace extractor
{
// Configuration to find representative candidate for turn angle calculations

EdgeBasedGraphFactory::EdgeBasedGraphFactory(
    std::shared_ptr<util::NodeBasedDynamicGraph> node_based_graph,
    const CompressedEdgeContainer &compressed_edge_container,
    const std::unordered_set<NodeID> &barrier_nodes,
    const std::unordered_set<NodeID> &traffic_lights,
    std::shared_ptr<const RestrictionMap> restriction_map,
    const std::vector<QueryNode> &node_info_list,
    SpeedProfileProperties speed_profile)
    : m_max_edge_id(0), m_node_info_list(node_info_list),
      m_node_based_graph(std::move(node_based_graph)),
      m_restriction_map(std::move(restriction_map)), m_barrier_nodes(barrier_nodes),
      m_traffic_lights(traffic_lights), m_compressed_edge_container(compressed_edge_container),
      speed_profile(std::move(speed_profile))
{
}

void EdgeBasedGraphFactory::GetEdgeBasedEdges(
    util::DeallocatingVector<EdgeBasedEdge> &output_edge_list)
{
    BOOST_ASSERT_MSG(0 == output_edge_list.size(), "Vector is not empty");
    using std::swap; // Koenig swap
    swap(m_edge_based_edge_list, output_edge_list);
}

void EdgeBasedGraphFactory::GetEdgeBasedNodes(std::vector<EdgeBasedNode> &nodes)
{
#ifndef NDEBUG
    for (const EdgeBasedNode &node : m_edge_based_node_list)
    {
        BOOST_ASSERT(
            util::Coordinate(m_node_info_list[node.u].lon, m_node_info_list[node.u].lat).IsValid());
        BOOST_ASSERT(
            util::Coordinate(m_node_info_list[node.v].lon, m_node_info_list[node.v].lat).IsValid());
    }
#endif
    using std::swap; // Koenig swap
    swap(nodes, m_edge_based_node_list);
}

void EdgeBasedGraphFactory::GetStartPointMarkers(std::vector<bool> &node_is_startpoint)
{
    using std::swap; // Koenig swap
    swap(m_edge_based_node_is_startpoint, node_is_startpoint);
}

void EdgeBasedGraphFactory::GetEdgeBasedNodeWeights(std::vector<EdgeWeight> &output_node_weights)
{
    using std::swap; // Koenig swap
    swap(m_edge_based_node_weights, output_node_weights);
}

unsigned EdgeBasedGraphFactory::GetHighestEdgeID() { return m_max_edge_id; }

void EdgeBasedGraphFactory::InsertEdgeBasedNode(const NodeID node_u, const NodeID node_v)
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

    if (forward_data.edge_id == SPECIAL_NODEID && reverse_data.edge_id == SPECIAL_NODEID)
    {
        return;
    }

    if (forward_data.edge_id != SPECIAL_NODEID && reverse_data.edge_id == SPECIAL_NODEID)
        m_edge_based_node_weights[forward_data.edge_id] = INVALID_EDGE_WEIGHT;

    BOOST_ASSERT(m_compressed_edge_container.HasEntryForID(edge_id_1) ==
                 m_compressed_edge_container.HasEntryForID(edge_id_2));
    BOOST_ASSERT(m_compressed_edge_container.HasEntryForID(edge_id_1));
    BOOST_ASSERT(m_compressed_edge_container.HasEntryForID(edge_id_2));
    const auto &forward_geometry = m_compressed_edge_container.GetBucketReference(edge_id_1);
    BOOST_ASSERT(forward_geometry.size() ==
                 m_compressed_edge_container.GetBucketReference(edge_id_2).size());
    const auto geometry_size = forward_geometry.size();

    // There should always be some geometry
    BOOST_ASSERT(0 != geometry_size);

    NodeID current_edge_source_coordinate_id = node_u;

    // traverse arrays from start and end respectively
    for (const auto i : util::irange(0UL, geometry_size))
    {
        BOOST_ASSERT(
            current_edge_source_coordinate_id ==
            m_compressed_edge_container.GetBucketReference(edge_id_2)[geometry_size - 1 - i]
                .node_id);
        const NodeID current_edge_target_coordinate_id = forward_geometry[i].node_id;
        BOOST_ASSERT(current_edge_target_coordinate_id != current_edge_source_coordinate_id);

        // build edges
        m_edge_based_node_list.emplace_back(
            forward_data.edge_id, reverse_data.edge_id, current_edge_source_coordinate_id,
            current_edge_target_coordinate_id, forward_data.name_id,
            m_compressed_edge_container.GetPositionForID(edge_id_1),
            m_compressed_edge_container.GetPositionForID(edge_id_2), false, INVALID_COMPONENTID, i,
            forward_data.travel_mode, reverse_data.travel_mode);

        m_edge_based_node_is_startpoint.push_back(forward_data.startpoint ||
                                                  reverse_data.startpoint);
        current_edge_source_coordinate_id = current_edge_target_coordinate_id;
    }

    BOOST_ASSERT(current_edge_source_coordinate_id == node_v);
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
                                lua_State *lua_state,
                                const std::string &edge_segment_lookup_filename,
                                const std::string &edge_penalty_filename,
                                const bool generate_edge_lookup)
{
    TIMER_START(renumber);
    m_max_edge_id = RenumberEdges() - 1;
    TIMER_STOP(renumber);

    TIMER_START(generate_nodes);
    m_edge_based_node_weights.reserve(m_max_edge_id + 1);
    GenerateEdgeExpandedNodes();
    TIMER_STOP(generate_nodes);

    TIMER_START(generate_edges);
    GenerateEdgeExpandedEdges(original_edge_data_filename, lua_state, edge_segment_lookup_filename,
                              edge_penalty_filename, generate_edge_lookup);

    TIMER_STOP(generate_edges);

    util::SimpleLogger().Write() << "Timing statistics for edge-expanded graph:";
    util::SimpleLogger().Write() << "Renumbering edges: " << TIMER_SEC(renumber) << "s";
    util::SimpleLogger().Write() << "Generating nodes: " << TIMER_SEC(generate_nodes) << "s";
    util::SimpleLogger().Write() << "Generating edges: " << TIMER_SEC(generate_edges) << "s";
}

/// Renumbers all _forward_ edges and sets the edge_id.
/// A specific numbering is not important. Any unique ID will do.
/// Returns the number of edge based nodes.
unsigned EdgeBasedGraphFactory::RenumberEdges()
{
    // renumber edge based node of outgoing edges
    unsigned numbered_edges_count = 0;
    for (const auto current_node : util::irange(0u, m_node_based_graph->GetNumberOfNodes()))
    {
        for (const auto current_edge : m_node_based_graph->GetAdjacentEdgeRange(current_node))
        {
            EdgeData &edge_data = m_node_based_graph->GetEdgeData(current_edge);

            // only number incoming edges
            if (edge_data.reversed)
            {
                continue;
            }

            // oneway streets always require this self-loop. Other streets only if a u-turn plus
            // traversal
            // of the street takes longer than the loop
            m_edge_based_node_weights.push_back(edge_data.distance + speed_profile.u_turn_penalty);

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
    util::Percent progress(m_node_based_graph->GetNumberOfNodes());

    // loop over all edges and generate new set of nodes
    for (const auto node_u : util::irange(0u, m_node_based_graph->GetNumberOfNodes()))
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

            // if we found a non-forward edge reverse and try again
            if (edge_data.edge_id == SPECIAL_NODEID)
            {
                InsertEdgeBasedNode(node_v, node_u);
            }
            else
            {
                InsertEdgeBasedNode(node_u, node_v);
            }
        }
    }

    BOOST_ASSERT(m_edge_based_node_list.size() == m_edge_based_node_is_startpoint.size());
    BOOST_ASSERT(m_max_edge_id + 1 == m_edge_based_node_weights.size());

    util::SimpleLogger().Write() << "Generated " << m_edge_based_node_list.size()
                                 << " nodes in edge-expanded graph";
}

/// Actually it also generates OriginalEdgeData and serializes them...
void EdgeBasedGraphFactory::GenerateEdgeExpandedEdges(
    const std::string &original_edge_data_filename,
    lua_State *lua_state,
    const std::string &edge_segment_lookup_filename,
    const std::string &edge_fixed_penalties_filename,
    const bool generate_edge_lookup)
{
    util::SimpleLogger().Write() << "generating edge-expanded edges";

    std::size_t node_based_edge_counter = 0;
    std::size_t original_edges_counter = 0;
    restricted_turns_counter = 0;
    skipped_uturns_counter = 0;
    skipped_barrier_turns_counter = 0;
    std::size_t compressed = 0;

    std::ofstream edge_data_file(original_edge_data_filename.c_str(), std::ios::binary);
    std::ofstream edge_segment_file;
    std::ofstream edge_penalty_file;

    if (generate_edge_lookup)
    {
        edge_segment_file.open(edge_segment_lookup_filename.c_str(), std::ios::binary);
        edge_penalty_file.open(edge_fixed_penalties_filename.c_str(), std::ios::binary);
    }

    // writes a dummy value that is updated later
    edge_data_file.write((char *)&original_edges_counter, sizeof(unsigned));

    std::vector<OriginalEdgeData> original_edge_data_vector;
    original_edge_data_vector.reserve(1024 * 1024);

    // Loop over all turns and generate new set of edges.
    // Three nested loop look super-linear, but we are dealing with a (kind of)
    // linear number of turns only.
    util::Percent progress(m_node_based_graph->GetNumberOfNodes());
    for (const auto node_u : util::irange(0u, m_node_based_graph->GetNumberOfNodes()))
    {
        // progress.printStatus(node_u);
        for (const EdgeID edge_from_u : m_node_based_graph->GetAdjacentEdgeRange(node_u))
        {
            if (m_node_based_graph->GetEdgeData(edge_from_u).reversed)
            {
                continue;
            }

            ++node_based_edge_counter;
            auto turn_candidates =
                guidance::getTurns(node_u, edge_from_u, *m_node_based_graph, m_node_info_list,
                                   *m_restriction_map, m_barrier_nodes, m_compressed_edge_container);

            const NodeID node_v = m_node_based_graph->GetTarget(edge_from_u);

            for (const auto turn : turn_candidates)
            {
                if (!turn.valid)
                    continue;

                const double turn_angle = turn.angle;

                // only add an edge if turn is not prohibited
                const EdgeData &edge_data1 = m_node_based_graph->GetEdgeData(edge_from_u);
                const EdgeData &edge_data2 = m_node_based_graph->GetEdgeData(turn.eid);

                BOOST_ASSERT(edge_data1.edge_id != edge_data2.edge_id);
                BOOST_ASSERT(!edge_data1.reversed);
                BOOST_ASSERT(!edge_data2.reversed);

                // the following is the core of the loop.
                unsigned distance = edge_data1.distance;
                if (m_traffic_lights.find(node_v) != m_traffic_lights.end())
                {
                    distance += speed_profile.traffic_signal_penalty;
                }

                const int turn_penalty = GetTurnPenalty(turn_angle, lua_state);
                const auto turn_instruction = turn.instruction;

                if (guidance::isUturn(turn_instruction))
                {
                    distance += speed_profile.u_turn_penalty;
                }

                distance += turn_penalty;

                const bool edge_is_compressed =
                    m_compressed_edge_container.HasEntryForID(edge_from_u);

                if (edge_is_compressed)
                {
                    ++compressed;
                }

                original_edge_data_vector.emplace_back(
                    (edge_is_compressed ? m_compressed_edge_container.GetPositionForID(edge_from_u)
                                        : node_v),
                    edge_data1.name_id, turn_instruction, edge_is_compressed,
                    edge_data1.travel_mode);

                ++original_edges_counter;

                if (original_edge_data_vector.size() > 1024 * 1024 * 10)
                {
                    FlushVectorToStream(edge_data_file, original_edge_data_vector);
                }

                BOOST_ASSERT(SPECIAL_NODEID != edge_data1.edge_id);
                BOOST_ASSERT(SPECIAL_NODEID != edge_data2.edge_id);

                // NOTE: potential overflow here if we hit 2^32 routable edges
                BOOST_ASSERT(m_edge_based_edge_list.size() <= std::numeric_limits<NodeID>::max());
                m_edge_based_edge_list.emplace_back(edge_data1.edge_id, edge_data2.edge_id,
                                                    m_edge_based_edge_list.size(), distance, true,
                                                    false);

                // Here is where we write out the mapping between the edge-expanded edges, and
                // the node-based edges that are originally used to calculate the `distance`
                // for the edge-expanded edges.  About 40 lines back, there is:
                //
                //                 unsigned distance = edge_data1.distance;
                //
                // This tells us that the weight for an edge-expanded-edge is based on the weight
                // of the *source* node-based edge.  Therefore, we will look up the individual
                // segments of the source node-based edge, and write out a mapping between
                // those and the edge-based-edge ID.
                // External programs can then use this mapping to quickly perform
                // updates to the edge-expanded-edge based directly on its ID.
                if (generate_edge_lookup)
                {
                    unsigned fixed_penalty = distance - edge_data1.distance;
                    edge_penalty_file.write(reinterpret_cast<const char *>(&fixed_penalty),
                                            sizeof(fixed_penalty));
                    if (edge_is_compressed)
                    {
                        const auto node_based_edges =
                            m_compressed_edge_container.GetBucketReference(edge_from_u);
                        NodeID previous = node_u;

                        const unsigned node_count = node_based_edges.size() + 1;
                        edge_segment_file.write(reinterpret_cast<const char *>(&node_count),
                                                sizeof(node_count));
                        const QueryNode &first_node = m_node_info_list[previous];
                        edge_segment_file.write(reinterpret_cast<const char *>(&first_node.node_id),
                                                sizeof(first_node.node_id));

                        for (auto target_node : node_based_edges)
                        {
                            const QueryNode &from = m_node_info_list[previous];
                            const QueryNode &to = m_node_info_list[target_node.node_id];
                            const double segment_length =
                                util::coordinate_calculation::greatCircleDistance(
                                    util::Coordinate(from.lon, from.lat),
                                    util::Coordinate(to.lon, to.lat));

                            edge_segment_file.write(reinterpret_cast<const char *>(&to.node_id),
                                                    sizeof(to.node_id));
                            edge_segment_file.write(reinterpret_cast<const char *>(&segment_length),
                                                    sizeof(segment_length));
                            edge_segment_file.write(
                                reinterpret_cast<const char *>(&target_node.weight),
                                sizeof(target_node.weight));
                            previous = target_node.node_id;
                        }
                    }
                    else
                    {
                        static const unsigned node_count = 2;
                        const QueryNode from = m_node_info_list[node_u];
                        const QueryNode to = m_node_info_list[node_v];
                        const double segment_length =
                            util::coordinate_calculation::greatCircleDistance(
                                util::Coordinate(from.lon, from.lat),
                                util::Coordinate(to.lon, to.lat));
                        edge_segment_file.write(reinterpret_cast<const char *>(&node_count),
                                                sizeof(node_count));
                        edge_segment_file.write(reinterpret_cast<const char *>(&from.node_id),
                                                sizeof(from.node_id));
                        edge_segment_file.write(reinterpret_cast<const char *>(&to.node_id),
                                                sizeof(to.node_id));
                        edge_segment_file.write(reinterpret_cast<const char *>(&segment_length),
                                                sizeof(segment_length));
                        edge_segment_file.write(
                            reinterpret_cast<const char *>(&edge_data1.distance),
                            sizeof(edge_data1.distance));
                    }
                }
            }
        }
    }

    FlushVectorToStream(edge_data_file, original_edge_data_vector);

    edge_data_file.seekp(std::ios::beg);
    edge_data_file.write((char *)&original_edges_counter, sizeof(unsigned));
    edge_data_file.close();

    util::SimpleLogger().Write() << "Generated " << m_edge_based_node_list.size()
                                 << " edge based nodes";
    util::SimpleLogger().Write() << "Node-based graph contains " << node_based_edge_counter
                                 << " edges";
    util::SimpleLogger().Write() << "Edge-expanded graph ...";
    util::SimpleLogger().Write() << "  contains " << m_edge_based_edge_list.size() << " edges";
    util::SimpleLogger().Write() << "  skips " << restricted_turns_counter << " turns, "
                                                                              "defined by "
                                 << m_restriction_map->size() << " restrictions";
    util::SimpleLogger().Write() << "  skips " << skipped_uturns_counter << " U turns";
    util::SimpleLogger().Write() << "  skips " << skipped_barrier_turns_counter
                                 << " turns over barriers";
}

int EdgeBasedGraphFactory::GetTurnPenalty(double angle, lua_State *lua_state) const
{

    if (speed_profile.has_turn_penalty_function)
    {
        try
        {
            // call lua profile to compute turn penalty
            double penalty =
                luabind::call_function<double>(lua_state, "turn_function", 180. - angle);
            return static_cast<int>(penalty);
        }
        catch (const luabind::error &er)
        {
            util::SimpleLogger().Write(logWARNING) << er.what();
        }
    }
    return 0;
}

} // namespace extractor
} // namespace osrm
