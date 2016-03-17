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

// configuration of turn classification
const bool constexpr INVERT = true;
const bool constexpr RESOLVE_TO_RIGHT = true;
const bool constexpr RESOLVE_TO_LEFT = false;

// what angle is interpreted as going straight
const double constexpr STRAIGHT_ANGLE = 180.;
// if a turn deviates this much from going straight, it will be kept straight
const double constexpr MAXIMAL_ALLOWED_NO_TURN_DEVIATION = 2.;
// angle that lies between two nearly indistinguishable roads
const double constexpr NARROW_TURN_ANGLE = 25.;
// angle difference that can be classified as straight, if its the only narrow turn
const double constexpr FUZZY_STRAIGHT_ANGLE = 15.;
const double constexpr DISTINCTION_RATIO = 2;

// Configuration to find representative candidate for turn angle calculations
const double constexpr MINIMAL_SEGMENT_LENGTH = 1.;
const double constexpr DESIRED_SEGMENT_LENGTH = 10.;

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
        BOOST_ASSERT(m_node_info_list.at(node.u).lat != INT_MAX);
        BOOST_ASSERT(m_node_info_list.at(node.u).lon != INT_MAX);
        BOOST_ASSERT(m_node_info_list.at(node.v).lon != INT_MAX);
        BOOST_ASSERT(m_node_info_list.at(node.v).lat != INT_MAX);
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
    edge_data_file.write(reinterpret_cast<const char *>(&original_edges_counter),
                         sizeof(original_edges_counter));

    std::vector<OriginalEdgeData> original_edge_data_vector;
    original_edge_data_vector.reserve(1024 * 1024);

    // Loop over all turns and generate new set of edges.
    // Three nested loop look super-linear, but we are dealing with a (kind of)
    // linear number of turns only.
    util::Percent progress(m_node_based_graph->GetNumberOfNodes());

    for (const auto node_u : util::irange(0u, m_node_based_graph->GetNumberOfNodes()))
    {
        // progress.printStatus(node_u);
        for (const EdgeID edge_form_u : m_node_based_graph->GetAdjacentEdgeRange(node_u))
        {
            if (m_node_based_graph->GetEdgeData(edge_form_u).reversed)
            {
                continue;
            }

            ++node_based_edge_counter;
            auto turn_candidates = getTurnCandidates(node_u, edge_form_u);
            turn_candidates = optimizeCandidates(edge_form_u, turn_candidates);
            turn_candidates = suppressTurns(edge_form_u, turn_candidates);

            const NodeID node_v = m_node_based_graph->GetTarget(edge_form_u);

            for (const auto turn : turn_candidates)
            {
                if (!turn.valid)
                    continue;

                const double turn_angle = turn.angle;

                // only add an edge if turn is not prohibited
                const EdgeData &edge_data1 = m_node_based_graph->GetEdgeData(edge_form_u);
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
                const TurnInstruction turn_instruction = turn.instruction;

                if (turn_instruction == TurnInstruction::UTurn)
                {
                    distance += speed_profile.u_turn_penalty;
                }

                distance += turn_penalty;

                const bool edge_is_compressed =
                    m_compressed_edge_container.HasEntryForID(edge_form_u);

                if (edge_is_compressed)
                {
                    ++compressed;
                }

                original_edge_data_vector.emplace_back(
                    (edge_is_compressed ? m_compressed_edge_container.GetPositionForID(edge_form_u)
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
                            m_compressed_edge_container.GetBucketReference(edge_form_u);
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
                                    from.lat, from.lon, to.lat, to.lon);

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
                            util::coordinate_calculation::greatCircleDistance(from.lat, from.lon,
                                                                              to.lat, to.lon);
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
    edge_data_file.write(reinterpret_cast<char *>(&original_edges_counter),
                         sizeof(original_edges_counter));

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

// requires sorted candidates
std::vector<EdgeBasedGraphFactory::TurnCandidate>
EdgeBasedGraphFactory::optimizeCandidates(NodeID via_eid,
                                          std::vector<TurnCandidate> turn_candidates) const
{
    BOOST_ASSERT_MSG(std::is_sorted(turn_candidates.begin(), turn_candidates.end(),
                                    [](const TurnCandidate &left, const TurnCandidate &right)
                                    {
                                        return left.angle < right.angle;
                                    }),
                     "Turn Candidates not sorted by angle.");
    if (turn_candidates.size() <= 1)
        return turn_candidates;

    const auto getLeft = [&turn_candidates](std::size_t index)
    {
        return (index + 1) % turn_candidates.size();
    };
    const auto getRight = [&turn_candidates](std::size_t index)
    {
        return (index + turn_candidates.size() - 1) % turn_candidates.size();
    };

    // handle availability of multiple u-turns (e.g. street with separated small parking roads)
    if (turn_candidates[0].instruction == TurnInstruction::UTurn && turn_candidates[0].angle == 0)
    {
        if (turn_candidates[getLeft(0)].instruction == TurnInstruction::UTurn)
            turn_candidates[getLeft(0)].instruction = TurnInstruction::TurnSharpLeft;
        if (turn_candidates[getRight(0)].instruction == TurnInstruction::UTurn)
            turn_candidates[getRight(0)].instruction = TurnInstruction::TurnSharpRight;
    }

    const auto keepStraight = [](double angle)
    {
        return std::abs(angle - 180) < 5;
    };

    for (std::size_t turn_index = 0; turn_index < turn_candidates.size(); ++turn_index)
    {
        auto &turn = turn_candidates[turn_index];
        if (turn.instruction > TurnInstruction::TurnSlightLeft ||
            turn.instruction == TurnInstruction::UTurn)
            continue;
        auto &left = turn_candidates[getLeft(turn_index)];
        if (turn.angle == left.angle)
        {
            util::SimpleLogger().Write(logDEBUG)
                << "[warning] conflicting turn angles, identical road duplicated? "
                << m_node_info_list[m_node_based_graph->GetTarget(via_eid)].lat << " "
                << m_node_info_list[m_node_based_graph->GetTarget(via_eid)].lon << std::endl;
        }
        if (isConflict(turn.instruction, left.instruction))
        {
            // begin of a conflicting region
            std::size_t conflict_begin = turn_index;
            std::size_t conflict_end = getLeft(turn_index);
            std::size_t conflict_size = 2;
            while (
                isConflict(turn_candidates[getLeft(conflict_end)].instruction, turn.instruction) &&
                conflict_size < turn_candidates.size())
            {
                conflict_end = getLeft(conflict_end);
                ++conflict_size;
            }

            turn_index = (conflict_end < conflict_begin) ? turn_candidates.size() : conflict_end;

            if (conflict_size > 3)
            {
                // check if some turns are invalid to find out about good handling
            }

            auto &instruction_left_of_end = turn_candidates[getLeft(conflict_end)].instruction;
            auto &instruction_right_of_begin =
                turn_candidates[getRight(conflict_begin)].instruction;
            auto &candidate_at_end = turn_candidates[conflict_end];
            auto &candidate_at_begin = turn_candidates[conflict_begin];
            if (conflict_size == 2)
            {
                if (turn.instruction == TurnInstruction::GoStraight)
                {
                    if (instruction_left_of_end != TurnInstruction::TurnSlightLeft &&
                        instruction_right_of_begin != TurnInstruction::TurnSlightRight)
                    {
                        std::int32_t resolved_count = 0;
                        // uses side-effects in resolve
                        if (!keepStraight(candidate_at_end.angle) &&
                            !resolve(candidate_at_end.instruction, instruction_left_of_end,
                                     RESOLVE_TO_LEFT))
                            util::SimpleLogger().Write(logDEBUG)
                                << "[warning] failed to resolve conflict";
                        else
                            ++resolved_count;
                        // uses side-effects in resolve
                        if (!keepStraight(candidate_at_begin.angle) &&
                            !resolve(candidate_at_begin.instruction, instruction_right_of_begin,
                                     RESOLVE_TO_RIGHT))
                            util::SimpleLogger().Write(logDEBUG)
                                << "[warning] failed to resolve conflict";
                        else
                            ++resolved_count;
                        if (resolved_count >= 1 &&
                            (!keepStraight(candidate_at_begin.angle) ||
                             !keepStraight(candidate_at_end.angle))) // should always be the
                                                                     // case, theoretically
                            continue;
                    }
                }
                if (candidate_at_begin.confidence < candidate_at_end.confidence)
                { // if right shift is cheaper, or only option
                    if (resolve(candidate_at_begin.instruction, instruction_right_of_begin,
                                RESOLVE_TO_RIGHT))
                        continue;
                    else if (resolve(candidate_at_end.instruction, instruction_left_of_end,
                                     RESOLVE_TO_LEFT))
                        continue;
                }
                else
                {
                    if (resolve(candidate_at_end.instruction, instruction_left_of_end,
                                RESOLVE_TO_LEFT))
                        continue;
                    else if (resolve(candidate_at_begin.instruction, instruction_right_of_begin,
                                     RESOLVE_TO_RIGHT))
                        continue;
                }
                if (isSlightTurn(turn.instruction) || isSharpTurn(turn.instruction))
                {
                    auto resolve_direction =
                        (turn.instruction == TurnInstruction::TurnSlightRight ||
                         turn.instruction == TurnInstruction::TurnSharpLeft)
                            ? RESOLVE_TO_RIGHT
                            : RESOLVE_TO_LEFT;
                    if (resolve_direction == RESOLVE_TO_RIGHT &&
                        resolveTransitive(
                            candidate_at_begin.instruction, instruction_right_of_begin,
                            turn_candidates[getRight(getRight(conflict_begin))].instruction,
                            RESOLVE_TO_RIGHT))
                        continue;
                    else if (resolve_direction == RESOLVE_TO_LEFT &&
                             resolveTransitive(
                                 candidate_at_end.instruction, instruction_left_of_end,
                                 turn_candidates[getLeft(getLeft(conflict_end))].instruction,
                                 RESOLVE_TO_LEFT))
                        continue;
                }
            }
            else if (conflict_size >= 3)
            {
                // a conflict of size larger than three cannot be handled with the current
                // model.
                // Handle it as best as possible and keep the rest of the conflicting turns
                if (conflict_size > 3)
                {
                    NodeID conflict_location = m_node_based_graph->GetTarget(via_eid);
                    util::SimpleLogger().Write(logDEBUG)
                        << "[warning] found conflict larget than size three at "
                        << m_node_info_list[conflict_location].lat << ", "
                        << m_node_info_list[conflict_location].lon;
                }

                if (!resolve(candidate_at_begin.instruction, instruction_right_of_begin,
                             RESOLVE_TO_RIGHT))
                {
                    if (isSlightTurn(turn.instruction))
                        resolveTransitive(
                            candidate_at_begin.instruction, instruction_right_of_begin,
                            turn_candidates[getRight(getRight(conflict_begin))].instruction,
                            RESOLVE_TO_RIGHT);
                    else if (isSharpTurn(turn.instruction))
                        resolveTransitive(
                            candidate_at_end.instruction, instruction_left_of_end,
                            turn_candidates[getLeft(getLeft(conflict_end))].instruction,
                            RESOLVE_TO_LEFT);
                }
                if (!resolve(candidate_at_end.instruction, instruction_left_of_end,
                             RESOLVE_TO_LEFT))
                {
                    if (isSlightTurn(turn.instruction))
                        resolveTransitive(
                            candidate_at_end.instruction, instruction_left_of_end,
                            turn_candidates[getLeft(getLeft(conflict_end))].instruction,
                            RESOLVE_TO_LEFT);
                    else if (isSharpTurn(turn.instruction))
                        resolveTransitive(
                            candidate_at_begin.instruction, instruction_right_of_begin,
                            turn_candidates[getRight(getRight(conflict_begin))].instruction,
                            RESOLVE_TO_RIGHT);
                }
            }
        }
    }
    return turn_candidates;
}

bool EdgeBasedGraphFactory::isObviousChoice(EdgeID via_eid,
                                            std::size_t turn_index,
                                            const std::vector<TurnCandidate> &turn_candidates) const
{
    const auto getLeft = [&turn_candidates](std::size_t index)
    {
        return (index + 1) % turn_candidates.size();
    };
    const auto getRight = [&turn_candidates](std::size_t index)
    {
        return (index + turn_candidates.size() - 1) % turn_candidates.size();
    };
    const auto &candidate = turn_candidates[turn_index];
    const EdgeData &in_data = m_node_based_graph->GetEdgeData(via_eid);
    const EdgeData &out_data = m_node_based_graph->GetEdgeData(candidate.eid);
    const auto &candidate_to_the_left = turn_candidates[getLeft(turn_index)];

    const auto &candidate_to_the_right = turn_candidates[getRight(turn_index)];

    const auto hasValidRatio = [](const TurnCandidate &left, const TurnCandidate &center,
                                  const TurnCandidate &right)
    {
        auto angle_left = (left.angle > 180) ? angularDeviation(left.angle, STRAIGHT_ANGLE) : 180;
        auto angle_right =
            (right.angle < 180) ? angularDeviation(right.angle, STRAIGHT_ANGLE) : 180;
        auto self_angle = angularDeviation(center.angle, STRAIGHT_ANGLE);
        return angularDeviation(center.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE &&
               ((center.angle < STRAIGHT_ANGLE)
                    ? (angle_right > self_angle && angle_left / self_angle > DISTINCTION_RATIO)
                    : (angle_left > self_angle && angle_right / self_angle > DISTINCTION_RATIO));
    };
    // only valid turn

    return turn_candidates.size() == 1 ||
           // only non u-turn
           (turn_candidates.size() == 2 &&
            candidate_to_the_left.instruction == TurnInstruction::UTurn) || // nearly straight turn
           angularDeviation(candidate.angle, STRAIGHT_ANGLE) < MAXIMAL_ALLOWED_NO_TURN_DEVIATION ||
           hasValidRatio(candidate_to_the_left, candidate, candidate_to_the_right) ||
           (in_data.name_id != 0 && in_data.name_id == out_data.name_id &&
            angularDeviation(candidate.angle, STRAIGHT_ANGLE) < NARROW_TURN_ANGLE / 2);
}

std::vector<EdgeBasedGraphFactory::TurnCandidate>
EdgeBasedGraphFactory::suppressTurns(EdgeID via_eid,
                                     std::vector<TurnCandidate> turn_candidates) const
{
    // remove invalid candidates
    BOOST_ASSERT_MSG(std::is_sorted(turn_candidates.begin(), turn_candidates.end(),
                                    [](const TurnCandidate &left, const TurnCandidate &right)
                                    {
                                        return left.angle < right.angle;
                                    }),
                     "Turn Candidates not sorted by angle.");
    const auto end_valid = std::remove_if(turn_candidates.begin(), turn_candidates.end(),
                                          [](const TurnCandidate &candidate)
                                          {
                                              return !candidate.valid;
                                          });
    turn_candidates.erase(end_valid, turn_candidates.end());

    const auto getLeft = [&turn_candidates](std::size_t index)
    {
        return (index + 1) % turn_candidates.size();
    };
    const auto getRight = [&turn_candidates](std::size_t index)
    {
        return (index + turn_candidates.size() - 1) % turn_candidates.size();
    };

    const EdgeData &in_data = m_node_based_graph->GetEdgeData(via_eid);

    bool has_obvious_with_same_name = false;
    double obvious_with_same_name_angle = 0;
    for (std::size_t turn_index = 0; turn_index < turn_candidates.size(); ++turn_index)
    {
        if (m_node_based_graph->GetEdgeData(turn_candidates[turn_index].eid).name_id ==
                in_data.name_id &&
            isObviousChoice(via_eid, turn_index, turn_candidates))
        {
            has_obvious_with_same_name = true;
            obvious_with_same_name_angle = turn_candidates[turn_index].angle;
            break;
        }
    }

    for (std::size_t turn_index = 0; turn_index < turn_candidates.size(); ++turn_index)
    {
        auto &candidate = turn_candidates[turn_index];
        const EdgeData &out_data = m_node_based_graph->GetEdgeData(candidate.eid);
        if (candidate.valid && candidate.instruction != TurnInstruction::UTurn)
        {
            // TODO road category would be useful to indicate obviousness of turn
            // check if turn can be omitted or at least changed
            const auto &left = turn_candidates[getLeft(turn_index)];
            const auto &right = turn_candidates[getRight(turn_index)];

            // make very slight instructions straight, if they are the only valid choice going with
            // at most a slight turn
            if (candidate.instruction < TurnInstruction::ReachViaLocation &&
                (!isSlightTurn(getTurnDirection(left.angle)) || !left.valid) &&
                (!isSlightTurn(getTurnDirection(right.angle)) || !right.valid) &&
                angularDeviation(candidate.angle, STRAIGHT_ANGLE) < FUZZY_STRAIGHT_ANGLE)
                candidate.instruction = TurnInstruction::GoStraight;

            // TODO this smaller comparison for turns is DANGEROUS, has to be revised if turn
            // instructions change
            if (candidate.instruction < TurnInstruction::ReachViaLocation)
            {
                if (in_data.travel_mode ==
                    out_data.travel_mode) // make sure to always announce mode changes
                {
                    if (isObviousChoice(via_eid, turn_index, turn_candidates))
                    {

                        if (in_data.name_id == out_data.name_id) // same road
                        {
                            candidate.instruction = TurnInstruction::NoTurn;
                        }

                        else if (!has_obvious_with_same_name)
                        {
                            // TODO discuss, we might want to keep the current name of the turn. But
                            // this would mean emitting a turn when you just keep on a road
                            candidate.instruction = TurnInstruction::NameChanges;
                        }
                        else if (candidate.angle < obvious_with_same_name_angle)
                            candidate.instruction = TurnInstruction::TurnSlightRight;
                        else
                            candidate.instruction = TurnInstruction::TurnSlightLeft;
                    }
                    else if (candidate.instruction == TurnInstruction::GoStraight &&
                             has_obvious_with_same_name)
                    {
                        if (candidate.angle < obvious_with_same_name_angle)
                            candidate.instruction = TurnInstruction::TurnSlightRight;
                        else
                            candidate.instruction = TurnInstruction::TurnSlightLeft;
                    }
                }
            }
        }
    }
    return turn_candidates;
}

std::vector<EdgeBasedGraphFactory::TurnCandidate>
EdgeBasedGraphFactory::getTurnCandidates(NodeID from_node, EdgeID via_eid)
{
    std::vector<TurnCandidate> turn_candidates;
    const NodeID turn_node = m_node_based_graph->GetTarget(via_eid);
    const NodeID only_restriction_to_node =
        m_restriction_map->CheckForEmanatingIsOnlyTurn(from_node, turn_node);
    const bool is_barrier_node = m_barrier_nodes.find(turn_node) != m_barrier_nodes.end();

    for (const EdgeID onto_edge : m_node_based_graph->GetAdjacentEdgeRange(turn_node))
    {
        bool turn_is_valid = true;
        if (m_node_based_graph->GetEdgeData(onto_edge).reversed)
        {
            turn_is_valid = false;
        }
        const NodeID to_node = m_node_based_graph->GetTarget(onto_edge);

        if (turn_is_valid && (only_restriction_to_node != SPECIAL_NODEID) &&
            (to_node != only_restriction_to_node))
        {
            // We are at an only_-restriction but not at the right turn.
            ++restricted_turns_counter;
            turn_is_valid = false;
        }

        if (turn_is_valid)
        {
            if (is_barrier_node)
            {
                if (from_node != to_node)
                {
                    ++skipped_barrier_turns_counter;
                    turn_is_valid = false;
                }
            }
            else
            {
                if (from_node == to_node && m_node_based_graph->GetOutDegree(turn_node) > 1)
                {
                    auto number_of_emmiting_bidirectional_edges = 0;
                    for (auto edge : m_node_based_graph->GetAdjacentEdgeRange(turn_node))
                    {
                        auto target = m_node_based_graph->GetTarget(edge);
                        auto reverse_edge = m_node_based_graph->FindEdge(target, turn_node);
                        if (!m_node_based_graph->GetEdgeData(reverse_edge).reversed)
                        {
                            ++number_of_emmiting_bidirectional_edges;
                        }
                    }
                    if (number_of_emmiting_bidirectional_edges > 1)
                    {
                        ++skipped_uturns_counter;
                        turn_is_valid = false;
                    }
                }
            }
        }

        // only add an edge if turn is not a U-turn except when it is
        // at the end of a dead-end street
        if (m_restriction_map->CheckIfTurnIsRestricted(from_node, turn_node, to_node) &&
            (only_restriction_to_node == SPECIAL_NODEID) && (to_node != only_restriction_to_node))
        {
            // We are at an only_-restriction but not at the right turn.
            ++restricted_turns_counter;
            turn_is_valid = false;
        }

        // unpack first node of second segment if packed

        const auto first_coordinate =
            getRepresentativeCoordinate(from_node, turn_node, via_eid, INVERT);
        const auto third_coordinate =
            getRepresentativeCoordinate(turn_node, to_node, onto_edge, !INVERT);

        const auto angle = util::coordinate_calculation::computeAngle(
            first_coordinate, m_node_info_list[turn_node], third_coordinate);

        const auto turn = AnalyzeTurn(from_node, via_eid, turn_node, onto_edge, to_node, angle);

        auto confidence = getTurnConfidence(angle, turn);
        if (!turn_is_valid)
            confidence *= 0.8; // makes invalid turns more likely to be resolved in conflicts

        turn_candidates.push_back({onto_edge, turn_is_valid, angle, turn, confidence});
    }

    const auto ByAngle = [](const TurnCandidate &first, const TurnCandidate second)
    {
        return first.angle < second.angle;
    };
    std::sort(std::begin(turn_candidates), std::end(turn_candidates), ByAngle);

    const auto getLeft = [&](std::size_t index)
    {
        return (index + 1) % turn_candidates.size();
    };

    const auto getRight = [&](std::size_t index)
    {
        return (index + turn_candidates.size() - 1) % turn_candidates.size();
    };

    const auto isInvalidEquivalent = [&](std::size_t this_turn, std::size_t valid_turn)
    {
        if (!turn_candidates[valid_turn].valid || turn_candidates[this_turn].valid)
            return false;

        return angularDeviation(turn_candidates[this_turn].angle,
                                turn_candidates[valid_turn].angle) < NARROW_TURN_ANGLE;
    };

    for (std::size_t index = 0; index < turn_candidates.size(); ++index)
    {
        if (isInvalidEquivalent(index, getRight(index)) ||
            isInvalidEquivalent(index, getLeft(index)))
        {
            turn_candidates.erase(turn_candidates.begin() + index);
            --index;
        }
    }
    return turn_candidates;
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

// node_u -- (edge_1) --> node_v -- (edge_2) --> node_w
TurnInstruction EdgeBasedGraphFactory::AnalyzeTurn(const NodeID node_u,
                                                   const EdgeID edge1,
                                                   const NodeID node_v,
                                                   const EdgeID edge2,
                                                   const NodeID node_w,
                                                   const double angle) const
{

    const EdgeData &data1 = m_node_based_graph->GetEdgeData(edge1);
    const EdgeData &data2 = m_node_based_graph->GetEdgeData(edge2);
    if (node_u == node_w)
    {
        return TurnInstruction::UTurn;
    }

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

    // assign a designated turn angle instruction purely based on the angle
    return getTurnDirection(angle);
}

QueryNode EdgeBasedGraphFactory::getRepresentativeCoordinate(const NodeID src,
                                                             const NodeID tgt,
                                                             const EdgeID via_eid,
                                                             bool INVERTED) const
{
    if (m_compressed_edge_container.HasEntryForID(via_eid))
    {
        util::FixedPointCoordinate prev = util::FixedPointCoordinate(
                                       m_node_info_list[INVERTED ? tgt : src].lat,
                                       m_node_info_list[INVERTED ? tgt : src].lon),
                                   cur;
        // walk along the edge for the first 5 meters
        const auto &geometry = m_compressed_edge_container.GetBucketReference(via_eid);
        double dist = 0;
        double this_dist = 0;
        NodeID prev_id = INVERTED ? tgt : src;

        const auto selectBestCandidate = [this](const NodeID current, const double current_distance,
                                                const NodeID previous,
                                                const double previous_distance)
        {
            if (current_distance < DESIRED_SEGMENT_LENGTH ||
                current_distance - DESIRED_SEGMENT_LENGTH <
                    DESIRED_SEGMENT_LENGTH - previous_distance ||
                previous_distance < MINIMAL_SEGMENT_LENGTH)
            {
                return m_node_info_list[current];
            }
            else
            {
                return m_node_info_list[previous];
            }
        };

        if (INVERTED)
        {
            for (auto itr = geometry.rbegin(), end = geometry.rend(); itr != end; ++itr)
            {
                const auto compressed_node = *itr;
                cur = util::FixedPointCoordinate(m_node_info_list[compressed_node.node_id].lat,
                                                 m_node_info_list[compressed_node.node_id].lon);
                this_dist = util::coordinate_calculation::haversineDistance(prev, cur);
                if (dist + this_dist > DESIRED_SEGMENT_LENGTH)
                {
                    return selectBestCandidate(compressed_node.node_id, dist + this_dist, prev_id,
                                               dist);
                }
                dist += this_dist;
                prev = cur;
                prev_id = compressed_node.node_id;
            }
            cur = util::FixedPointCoordinate(m_node_info_list[src].lat, m_node_info_list[src].lon);
            this_dist = util::coordinate_calculation::haversineDistance(prev, cur);
            return selectBestCandidate(src, dist + this_dist, prev_id, dist);
        }
        else
        {
            for (auto itr = geometry.begin(), end = geometry.end(); itr != end; ++itr)
            {
                const auto compressed_node = *itr;
                cur = util::FixedPointCoordinate(m_node_info_list[compressed_node.node_id].lat,
                                                 m_node_info_list[compressed_node.node_id].lon);
                this_dist = util::coordinate_calculation::haversineDistance(prev, cur);
                if (dist + this_dist > DESIRED_SEGMENT_LENGTH)
                {
                    return selectBestCandidate(compressed_node.node_id, dist + this_dist, prev_id,
                                               dist);
                }
                dist += this_dist;
                prev = cur;
                prev_id = compressed_node.node_id;
            }
            cur = util::FixedPointCoordinate(m_node_info_list[tgt].lat, m_node_info_list[tgt].lon);
            this_dist = util::coordinate_calculation::haversineDistance(prev, cur);
            return selectBestCandidate(tgt, dist + this_dist, prev_id, dist);
        }
    }
    // default: If the edge is very short, or we do not have a compressed geometry
    return m_node_info_list[INVERTED ? src : tgt];
}
} // namespace extractor
} // namespace osrm
