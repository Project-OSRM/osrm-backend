#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/edge_based_edge.hpp"
#include "util/bearing.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/exception.hpp"
#include "util/guidance/turn_bearing.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/percent.hpp"
#include "util/timing_util.hpp"

#include "extractor/guidance/turn_analysis.hpp"
#include "extractor/guidance/turn_lane_handler.hpp"
#include "extractor/scripting_environment.hpp"
#include "extractor/suffix_table.hpp"

#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>

namespace osrm
{
namespace extractor
{
// Configuration to find representative candidate for turn angle calculations

EdgeBasedGraphFactory::EdgeBasedGraphFactory(
    std::shared_ptr<util::NodeBasedDynamicGraph> node_based_graph,
    CompressedEdgeContainer &compressed_edge_container,
    const std::unordered_set<NodeID> &barrier_nodes,
    const std::unordered_set<NodeID> &traffic_lights,
    std::shared_ptr<const RestrictionMap> restriction_map,
    const std::vector<QueryNode> &node_info_list,
    ProfileProperties profile_properties,
    const util::NameTable &name_table,
    std::vector<std::uint32_t> &turn_lane_offsets,
    std::vector<guidance::TurnLaneType::Mask> &turn_lane_masks,
    guidance::LaneDescriptionMap &lane_description_map)
    : m_max_edge_id(0), m_node_info_list(node_info_list),
      m_node_based_graph(std::move(node_based_graph)),
      m_restriction_map(std::move(restriction_map)), m_barrier_nodes(barrier_nodes),
      m_traffic_lights(traffic_lights), m_compressed_edge_container(compressed_edge_container),
      profile_properties(std::move(profile_properties)), name_table(name_table),
      turn_lane_offsets(turn_lane_offsets), turn_lane_masks(turn_lane_masks),
      lane_description_map(lane_description_map)
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

EdgeID EdgeBasedGraphFactory::GetHighestEdgeID() { return m_max_edge_id; }

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
    const auto segment_count = forward_geometry.size();

    // There should always be some geometry
    BOOST_ASSERT(0 != segment_count);

    const unsigned packed_geometry_id = m_compressed_edge_container.ZipEdges(edge_id_1, edge_id_2);

    NodeID current_edge_source_coordinate_id = node_u;

    const auto edge_id_to_segment_id = [](const NodeID edge_based_node_id) {
        if (edge_based_node_id == SPECIAL_NODEID)
        {
            return SegmentID{SPECIAL_SEGMENTID, false};
        }

        return SegmentID{edge_based_node_id, true};
    };

    // traverse arrays
    for (const auto i : util::irange(std::size_t{0}, segment_count))
    {
        BOOST_ASSERT(
            current_edge_source_coordinate_id ==
            m_compressed_edge_container.GetBucketReference(edge_id_2)[segment_count - 1 - i]
                .node_id);
        const NodeID current_edge_target_coordinate_id = forward_geometry[i].node_id;
        BOOST_ASSERT(current_edge_target_coordinate_id != current_edge_source_coordinate_id);

        // build edges
        m_edge_based_node_list.emplace_back(edge_id_to_segment_id(forward_data.edge_id),
                                            edge_id_to_segment_id(reverse_data.edge_id),
                                            current_edge_source_coordinate_id,
                                            current_edge_target_coordinate_id,
                                            forward_data.name_id,
                                            packed_geometry_id,
                                            false,
                                            INVALID_COMPONENTID,
                                            i,
                                            forward_data.travel_mode,
                                            reverse_data.travel_mode);

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

void EdgeBasedGraphFactory::Run(ScriptingEnvironment &scripting_environment,
                                const std::string &original_edge_data_filename,
                                const std::string &turn_lane_data_filename,
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
    GenerateEdgeExpandedEdges(scripting_environment,
                              original_edge_data_filename,
                              turn_lane_data_filename,
                              edge_segment_lookup_filename,
                              edge_penalty_filename,
                              generate_edge_lookup);

    TIMER_STOP(generate_edges);

    util::Log() << "Timing statistics for edge-expanded graph:";
    util::Log() << "Renumbering edges: " << TIMER_SEC(renumber) << "s";
    util::Log() << "Generating nodes: " << TIMER_SEC(generate_nodes) << "s";
    util::Log() << "Generating edges: " << TIMER_SEC(generate_edges) << "s";
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
            m_edge_based_node_weights.push_back(edge_data.distance +
                                                profile_properties.u_turn_penalty);

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
    util::Log() << "Generating edge expanded nodes ... ";
    {
        util::UnbufferedLog log;
        util::Percent progress(log, m_node_based_graph->GetNumberOfNodes());

        m_compressed_edge_container.InitializeBothwayVector();

        // loop over all edges and generate new set of nodes
        for (const auto node_u : util::irange(0u, m_node_based_graph->GetNumberOfNodes()))
        {
            BOOST_ASSERT(node_u != SPECIAL_NODEID);
            BOOST_ASSERT(node_u < m_node_based_graph->GetNumberOfNodes());
            progress.PrintStatus(node_u);
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
    }

    BOOST_ASSERT(m_edge_based_node_list.size() == m_edge_based_node_is_startpoint.size());
    BOOST_ASSERT(m_max_edge_id + 1 == m_edge_based_node_weights.size());

    util::Log() << "Generated " << m_edge_based_node_list.size() << " nodes in edge-expanded graph";
}

/// Actually it also generates OriginalEdgeData and serializes them...
void EdgeBasedGraphFactory::GenerateEdgeExpandedEdges(
    ScriptingEnvironment &scripting_environment,
    const std::string &original_edge_data_filename,
    const std::string &turn_lane_data_filename,
    const std::string &edge_segment_lookup_filename,
    const std::string &edge_fixed_penalties_filename,
    const bool generate_edge_lookup)
{
    util::Log() << "Generating edge-expanded edges ";

    std::size_t node_based_edge_counter = 0;
    std::size_t original_edges_counter = 0;
    restricted_turns_counter = 0;
    skipped_uturns_counter = 0;
    skipped_barrier_turns_counter = 0;

    std::ofstream edge_data_file(original_edge_data_filename.c_str(), std::ios::binary);
    std::ofstream edge_segment_file;
    std::ofstream edge_penalty_file;

    if (generate_edge_lookup)
    {
        edge_segment_file.open(edge_segment_lookup_filename.c_str(), std::ios::binary);
        edge_penalty_file.open(edge_fixed_penalties_filename.c_str(), std::ios::binary);
    }

    // Writes a dummy value at the front that is updated later with the total length
    const std::uint64_t length_prefix_empty_space{0};
    edge_data_file.write(reinterpret_cast<const char *>(&length_prefix_empty_space),
                         sizeof(length_prefix_empty_space));

    std::vector<OriginalEdgeData> original_edge_data_vector;
    original_edge_data_vector.reserve(1024 * 1024);

    // Loop over all turns and generate new set of edges.
    // Three nested loop look super-linear, but we are dealing with a (kind of)
    // linear number of turns only.
    SuffixTable street_name_suffix_table(scripting_environment);
    guidance::TurnAnalysis turn_analysis(*m_node_based_graph,
                                         m_node_info_list,
                                         *m_restriction_map,
                                         m_barrier_nodes,
                                         m_compressed_edge_container,
                                         name_table,
                                         street_name_suffix_table,
                                         profile_properties);

    util::guidance::LaneDataIdMap lane_data_map;
    guidance::lanes::TurnLaneHandler turn_lane_handler(*m_node_based_graph,
                                                       turn_lane_offsets,
                                                       turn_lane_masks,
                                                       lane_description_map,
                                                       turn_analysis,
                                                       lane_data_map);

    bearing_class_by_node_based_node.resize(m_node_based_graph->GetNumberOfNodes(),
                                            std::numeric_limits<std::uint32_t>::max());

    {
        util::UnbufferedLog log;

        util::Percent progress(log, m_node_based_graph->GetNumberOfNodes());
        // going over all nodes (which form the center of an intersection), we compute all
        // possible turns along these intersections.
        for (const auto node_at_center_of_intersection :
             util::irange(0u, m_node_based_graph->GetNumberOfNodes()))
        {
            progress.PrintStatus(node_at_center_of_intersection);

            const auto shape_result =
                turn_analysis.ComputeIntersectionShapes(node_at_center_of_intersection);

            // all nodes in the graph are connected in both directions. We check all outgoing nodes
            // to
            // find the incoming edge. This is a larger search overhead, but the cost we need to pay
            // to
            // generate edges here is worth the additional search overhead.
            //
            // a -> b <-> c
            //      |
            //      v
            //      d
            //
            // will have:
            // a: b,rev=0
            // b: a,rev=1 c,rev=0 d,rev=0
            // c: b,rev=0
            //
            // From the flags alone, we cannot determine which nodes are connected to `b` by an
            // outgoing
            // edge. Therefore, we have to search all connected edges for edges entering `b`
            for (const EdgeID outgoing_edge :
                 m_node_based_graph->GetAdjacentEdgeRange(node_at_center_of_intersection))
            {
                const NodeID node_along_road_entering =
                    m_node_based_graph->GetTarget(outgoing_edge);

                const auto incoming_edge = m_node_based_graph->FindEdge(
                    node_along_road_entering, node_at_center_of_intersection);

                if (m_node_based_graph->GetEdgeData(incoming_edge).reversed)
                    continue;

                ++node_based_edge_counter;

                auto intersection_with_flags_and_angles =
                    turn_analysis.GetIntersectionGenerator().TransformIntersectionShapeIntoView(
                        node_along_road_entering,
                        incoming_edge,
                        shape_result.annotated_normalized_shape.normalized_shape,
                        shape_result.intersection_shape,
                        shape_result.annotated_normalized_shape.performed_merges);

                auto intersection = turn_analysis.AssignTurnTypes(
                    node_along_road_entering, incoming_edge, intersection_with_flags_and_angles);

                BOOST_ASSERT(intersection.valid());

                intersection = turn_lane_handler.assignTurnLanes(
                    node_along_road_entering, incoming_edge, std::move(intersection));

                // the entry class depends on the turn, so we have to classify the interesction for
                // every edge
                const auto turn_classification = classifyIntersection(intersection);

                const auto entry_class_id = [&](const util::guidance::EntryClass entry_class) {
                    if (0 == entry_class_hash.count(entry_class))
                    {
                        const auto id = static_cast<std::uint16_t>(entry_class_hash.size());
                        entry_class_hash[entry_class] = id;
                        return id;
                    }
                    else
                    {
                        return entry_class_hash.find(entry_class)->second;
                    }
                }(turn_classification.first);

                const auto bearing_class_id =
                    [&](const util::guidance::BearingClass bearing_class) {
                        if (0 == bearing_class_hash.count(bearing_class))
                        {
                            const auto id = static_cast<std::uint32_t>(bearing_class_hash.size());
                            bearing_class_hash[bearing_class] = id;
                            return id;
                        }
                        else
                        {
                            return bearing_class_hash.find(bearing_class)->second;
                        }
                    }(turn_classification.second);
                bearing_class_by_node_based_node[node_at_center_of_intersection] = bearing_class_id;

                for (const auto &turn : intersection)
                {
                    // only keep valid turns
                    if (!turn.entry_allowed)
                        continue;

                    // only add an edge if turn is not prohibited
                    const EdgeData &edge_data1 = m_node_based_graph->GetEdgeData(incoming_edge);
                    const EdgeData &edge_data2 = m_node_based_graph->GetEdgeData(turn.eid);

                    BOOST_ASSERT(edge_data1.edge_id != edge_data2.edge_id);
                    BOOST_ASSERT(!edge_data1.reversed);
                    BOOST_ASSERT(!edge_data2.reversed);

                    // the following is the core of the loop.
                    unsigned distance = edge_data1.distance;
                    if (m_traffic_lights.find(node_at_center_of_intersection) !=
                        m_traffic_lights.end())
                    {
                        distance += profile_properties.traffic_signal_penalty;
                    }

                    const int32_t turn_penalty =
                        scripting_environment.GetTurnPenalty(180. - turn.angle);

                    const auto turn_instruction = turn.instruction;
                    if (turn_instruction.direction_modifier == guidance::DirectionModifier::UTurn)
                    {
                        distance += profile_properties.u_turn_penalty;
                    }

                    // don't add turn penalty if it is not an actual turn. This heuristic is
                    // necessary
                    // since OSRM cannot handle looping roads/parallel roads
                    if (turn_instruction.type != guidance::TurnType::NoTurn)
                        distance += turn_penalty;

                    const bool is_encoded_forwards =
                        m_compressed_edge_container.HasZippedEntryForForwardID(incoming_edge);
                    const bool is_encoded_backwards =
                        m_compressed_edge_container.HasZippedEntryForReverseID(incoming_edge);
                    BOOST_ASSERT(is_encoded_forwards || is_encoded_backwards);
                    if (is_encoded_forwards)
                    {
                        original_edge_data_vector.emplace_back(
                            GeometryID{m_compressed_edge_container.GetZippedPositionForForwardID(
                                           incoming_edge),
                                       true},
                            edge_data1.name_id,
                            turn.lane_data_id,
                            turn_instruction,
                            entry_class_id,
                            edge_data1.travel_mode,
                            util::guidance::TurnBearing(intersection[0].bearing),
                            util::guidance::TurnBearing(turn.bearing));
                    }
                    else if (is_encoded_backwards)
                    {
                        original_edge_data_vector.emplace_back(
                            GeometryID{m_compressed_edge_container.GetZippedPositionForReverseID(
                                           incoming_edge),
                                       false},
                            edge_data1.name_id,
                            turn.lane_data_id,
                            turn_instruction,
                            entry_class_id,
                            edge_data1.travel_mode,
                            util::guidance::TurnBearing(intersection[0].bearing),
                            util::guidance::TurnBearing(turn.bearing));
                    }

                    ++original_edges_counter;

                    if (original_edge_data_vector.size() > 1024 * 1024 * 10)
                    {
                        FlushVectorToStream(edge_data_file, original_edge_data_vector);
                    }

                    BOOST_ASSERT(SPECIAL_NODEID != edge_data1.edge_id);
                    BOOST_ASSERT(SPECIAL_NODEID != edge_data2.edge_id);

                    // NOTE: potential overflow here if we hit 2^32 routable edges
                    BOOST_ASSERT(m_edge_based_edge_list.size() <=
                                 std::numeric_limits<NodeID>::max());
                    m_edge_based_edge_list.emplace_back(edge_data1.edge_id,
                                                        edge_data2.edge_id,
                                                        m_edge_based_edge_list.size(),
                                                        distance,
                                                        true,
                                                        false);
                    BOOST_ASSERT(original_edges_counter == m_edge_based_edge_list.size());

                    // Here is where we write out the mapping between the edge-expanded edges, and
                    // the node-based edges that are originally used to calculate the `distance`
                    // for the edge-expanded edges.  About 40 lines back, there is:
                    //
                    //                 unsigned distance = edge_data1.distance;
                    //
                    // This tells us that the weight for an edge-expanded-edge is based on the
                    // weight
                    // of the *source* node-based edge.  Therefore, we will look up the individual
                    // segments of the source node-based edge, and write out a mapping between
                    // those and the edge-based-edge ID.
                    // External programs can then use this mapping to quickly perform
                    // updates to the edge-expanded-edge based directly on its ID.
                    if (generate_edge_lookup)
                    {
                        const auto node_based_edges =
                            m_compressed_edge_container.GetBucketReference(incoming_edge);
                        NodeID previous = node_along_road_entering;

                        const unsigned node_count = node_based_edges.size() + 1;
                        const QueryNode &first_node = m_node_info_list[previous];

                        lookup::SegmentHeaderBlock header = {node_count, first_node.node_id};

                        edge_segment_file.write(reinterpret_cast<const char *>(&header),
                                                sizeof(header));

                        for (auto target_node : node_based_edges)
                        {
                            const QueryNode &from = m_node_info_list[previous];
                            const QueryNode &to = m_node_info_list[target_node.node_id];
                            const double segment_length =
                                util::coordinate_calculation::greatCircleDistance(from, to);

                            lookup::SegmentBlock nodeblock = {
                                to.node_id, segment_length, target_node.weight};

                            edge_segment_file.write(reinterpret_cast<const char *>(&nodeblock),
                                                    sizeof(nodeblock));
                            previous = target_node.node_id;
                        }

                        // We also now write out the mapping between the edge-expanded edges and the
                        // original nodes. Since each edge represents a possible maneuver, external
                        // programs can use this to quickly perform updates to edge weights in order
                        // to penalize certain turns.

                        // If this edge is 'trivial' -- where the compressed edge corresponds
                        // exactly to an original OSM segment -- we can pull the turn's preceding
                        // node ID directly with `node_along_road_entering`; otherwise, we need to
                        // look
                        // up the node
                        // immediately preceding the turn from the compressed edge container.
                        const bool isTrivial = m_compressed_edge_container.IsTrivial(incoming_edge);

                        const auto &from_node =
                            isTrivial
                                ? m_node_info_list[node_along_road_entering]
                                : m_node_info_list[m_compressed_edge_container.GetLastEdgeSourceID(
                                      incoming_edge)];
                        const auto &via_node =
                            m_node_info_list[m_compressed_edge_container.GetLastEdgeTargetID(
                                incoming_edge)];
                        const auto &to_node =
                            m_node_info_list[m_compressed_edge_container.GetFirstEdgeTargetID(
                                turn.eid)];

                        const unsigned fixed_penalty = distance - edge_data1.distance;
                        lookup::PenaltyBlock penaltyblock = {
                            fixed_penalty, from_node.node_id, via_node.node_id, to_node.node_id};
                        edge_penalty_file.write(reinterpret_cast<const char *>(&penaltyblock),
                                                sizeof(penaltyblock));
                    }
                }
            }
        }
    }

    util::Log() << "Created " << entry_class_hash.size() << " entry classes and "
                << bearing_class_hash.size() << " Bearing Classes";

    util::Log() << "Writing Turn Lane Data to File...";
    std::ofstream turn_lane_data_file(turn_lane_data_filename.c_str(), std::ios::binary);
    std::vector<util::guidance::LaneTupleIdPair> lane_data(lane_data_map.size());
    // extract lane data sorted by ID
    for (auto itr : lane_data_map)
        lane_data[itr.second] = itr.first;

    std::uint64_t size = lane_data.size();
    turn_lane_data_file.write(reinterpret_cast<const char *>(&size), sizeof(size));

    if (!lane_data.empty())
        turn_lane_data_file.write(reinterpret_cast<const char *>(&lane_data[0]),
                                  sizeof(util::guidance::LaneTupleIdPair) * lane_data.size());

    util::Log() << "done.";

    FlushVectorToStream(edge_data_file, original_edge_data_vector);

    // Finally jump back to the empty space at the beginning and write length prefix
    edge_data_file.seekp(std::ios::beg);

    const auto length_prefix = boost::numeric_cast<std::uint64_t>(original_edges_counter);
    static_assert(sizeof(length_prefix_empty_space) == sizeof(length_prefix), "type mismatch");

    edge_data_file.write(reinterpret_cast<const char *>(&length_prefix), sizeof(length_prefix));

    util::Log() << "Generated " << m_edge_based_node_list.size() << " edge based nodes";
    util::Log() << "Node-based graph contains " << node_based_edge_counter << " edges";
    util::Log() << "Edge-expanded graph ...";
    util::Log() << "  contains " << m_edge_based_edge_list.size() << " edges";
    util::Log() << "  skips " << restricted_turns_counter << " turns, "
                                                             "defined by "
                << m_restriction_map->size() << " restrictions";
    util::Log() << "  skips " << skipped_uturns_counter << " U turns";
    util::Log() << "  skips " << skipped_barrier_turns_counter << " turns over barriers";
}

std::vector<util::guidance::BearingClass> EdgeBasedGraphFactory::GetBearingClasses() const
{
    std::vector<util::guidance::BearingClass> result(bearing_class_hash.size());
    for (const auto &pair : bearing_class_hash)
    {
        BOOST_ASSERT(pair.second < result.size());
        result[pair.second] = pair.first;
    }
    return result;
}

const std::vector<BearingClassID> &EdgeBasedGraphFactory::GetBearingClassIds() const
{
    return bearing_class_by_node_based_node;
}

std::vector<BearingClassID> &EdgeBasedGraphFactory::GetBearingClassIds()
{
    return bearing_class_by_node_based_node;
}

std::vector<util::guidance::EntryClass> EdgeBasedGraphFactory::GetEntryClasses() const
{
    std::vector<util::guidance::EntryClass> result(entry_class_hash.size());
    for (const auto &pair : entry_class_hash)
    {
        BOOST_ASSERT(pair.second < result.size());
        result[pair.second] = pair.first;
    }
    return result;
}

} // namespace extractor
} // namespace osrm
