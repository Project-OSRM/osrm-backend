#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/edge_based_edge.hpp"
#include "extractor/files.hpp"
#include "extractor/guidance/turn_analysis.hpp"
#include "extractor/guidance/turn_lane_handler.hpp"
#include "extractor/scripting_environment.hpp"
#include "extractor/suffix_table.hpp"

#include "storage/io.hpp"

#include "util/bearing.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/exception.hpp"
#include "util/guidance/turn_bearing.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/percent.hpp"
#include "util/timing_util.hpp"

#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <algorithm>
#include <cmath>
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
    const std::vector<util::Coordinate> &coordinates,
    const util::PackedVector<OSMNodeID> &osm_node_ids,
    ProfileProperties profile_properties,
    const util::NameTable &name_table,
    std::vector<std::uint32_t> &turn_lane_offsets,
    std::vector<guidance::TurnLaneType::Mask> &turn_lane_masks,
    guidance::LaneDescriptionMap &lane_description_map)
    : m_max_edge_id(0), m_coordinates(coordinates), m_osm_node_ids(osm_node_ids),
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

NBGToEBG EdgeBasedGraphFactory::InsertEdgeBasedNode(const NodeID node_u, const NodeID node_v)
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

    BOOST_ASSERT(forward_data.edge_id != SPECIAL_NODEID || reverse_data.edge_id != SPECIAL_NODEID);

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

    return NBGToEBG{node_u, node_v, forward_data.edge_id, reverse_data.edge_id};
}

void EdgeBasedGraphFactory::Run(ScriptingEnvironment &scripting_environment,
                                const std::string &turn_data_filename,
                                const std::string &turn_lane_data_filename,
                                const std::string &turn_weight_penalties_filename,
                                const std::string &turn_duration_penalties_filename,
                                const std::string &turn_penalties_index_filename,
                                const std::string &cnbg_ebg_mapping_path)
{
    TIMER_START(renumber);
    m_max_edge_id = RenumberEdges() - 1;
    TIMER_STOP(renumber);

    TIMER_START(generate_nodes);
    m_edge_based_node_weights.reserve(m_max_edge_id + 1);
    {
        auto mapping = GenerateEdgeExpandedNodes();
        files::writeNBGMapping(cnbg_ebg_mapping_path, mapping);
    }
    TIMER_STOP(generate_nodes);

    TIMER_START(generate_edges);
    GenerateEdgeExpandedEdges(scripting_environment,
                              turn_data_filename,
                              turn_lane_data_filename,
                              turn_weight_penalties_filename,
                              turn_duration_penalties_filename,
                              turn_penalties_index_filename);

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

            m_edge_based_node_weights.push_back(edge_data.weight);

            BOOST_ASSERT(numbered_edges_count < m_node_based_graph->GetNumberOfEdges());
            edge_data.edge_id = numbered_edges_count;
            ++numbered_edges_count;

            BOOST_ASSERT(SPECIAL_NODEID != edge_data.edge_id);
        }
    }

    return numbered_edges_count;
}

/// Creates the nodes in the edge expanded graph from edges in the node-based graph.
std::vector<NBGToEBG> EdgeBasedGraphFactory::GenerateEdgeExpandedNodes()
{
    std::vector<NBGToEBG> mapping;

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
                    mapping.push_back(InsertEdgeBasedNode(node_v, node_u));
                }
                else
                {
                    mapping.push_back(InsertEdgeBasedNode(node_u, node_v));
                }
            }
        }
    }

    BOOST_ASSERT(m_edge_based_node_list.size() == m_edge_based_node_is_startpoint.size());
    BOOST_ASSERT(m_max_edge_id + 1 == m_edge_based_node_weights.size());

    util::Log() << "Generated " << m_edge_based_node_list.size() << " nodes in edge-expanded graph";

    return mapping;
}

/// Actually it also generates turn data and serializes them...
void EdgeBasedGraphFactory::GenerateEdgeExpandedEdges(
    ScriptingEnvironment &scripting_environment,
    const std::string &turn_data_filename,
    const std::string &turn_lane_data_filename,
    const std::string &turn_weight_penalties_filename,
    const std::string &turn_duration_penalties_filename,
    const std::string &turn_penalties_index_filename)
{
    util::Log() << "Generating edge-expanded edges ";

    std::size_t node_based_edge_counter = 0;
    restricted_turns_counter = 0;
    skipped_uturns_counter = 0;
    skipped_barrier_turns_counter = 0;

    storage::io::FileWriter turn_penalties_index_file(turn_penalties_index_filename,
                                                      storage::io::FileWriter::HasNoFingerprint);

    TurnDataExternalContainer turn_data_container;

    // Loop over all turns and generate new set of edges.
    // Three nested loop look super-linear, but we are dealing with a (kind of)
    // linear number of turns only.
    SuffixTable street_name_suffix_table(scripting_environment);
    guidance::TurnAnalysis turn_analysis(*m_node_based_graph,
                                         m_coordinates,
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

    // FIXME these need to be tuned in pre-allocated size
    std::vector<TurnPenalty> turn_weight_penalties;
    std::vector<TurnPenalty> turn_duration_penalties;

    const auto weight_multiplier =
        scripting_environment.GetProfileProperties().GetWeightMultiplier();

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
                    const bool is_encoded_forwards =
                        m_compressed_edge_container.HasZippedEntryForForwardID(incoming_edge);
                    const bool is_encoded_backwards =
                        m_compressed_edge_container.HasZippedEntryForReverseID(incoming_edge);
                    BOOST_ASSERT(is_encoded_forwards || is_encoded_backwards);
                    if (is_encoded_forwards)
                    {
                        turn_data_container.push_back(
                            GeometryID{m_compressed_edge_container.GetZippedPositionForForwardID(
                                           incoming_edge),
                                       true},
                            edge_data1.name_id,
                            turn.instruction,
                            turn.lane_data_id,
                            entry_class_id,
                            edge_data1.travel_mode,
                            util::guidance::TurnBearing(intersection[0].bearing),
                            util::guidance::TurnBearing(turn.bearing));
                    }
                    else if (is_encoded_backwards)
                    {
                        turn_data_container.push_back(
                            GeometryID{m_compressed_edge_container.GetZippedPositionForReverseID(
                                           incoming_edge),
                                       false},
                            edge_data1.name_id,
                            turn.instruction,
                            turn.lane_data_id,
                            entry_class_id,
                            edge_data1.travel_mode,
                            util::guidance::TurnBearing(intersection[0].bearing),
                            util::guidance::TurnBearing(turn.bearing));
                    }

                    // compute weight and duration penalties
                    auto is_traffic_light = m_traffic_lights.count(node_at_center_of_intersection);
                    ExtractionTurn extracted_turn(turn, is_traffic_light);
                    extracted_turn.source_restricted = edge_data1.restricted;
                    extracted_turn.target_restricted = edge_data2.restricted;
                    scripting_environment.ProcessTurn(extracted_turn);

                    // turn penalties are limited to [-2^15, 2^15) which roughly
                    // translates to 54 minutes and fits signed 16bit deci-seconds
                    auto weight_penalty =
                        boost::numeric_cast<TurnPenalty>(extracted_turn.weight * weight_multiplier);
                    auto duration_penalty =
                        boost::numeric_cast<TurnPenalty>(extracted_turn.duration * 10.);

                    BOOST_ASSERT(SPECIAL_NODEID != edge_data1.edge_id);
                    BOOST_ASSERT(SPECIAL_NODEID != edge_data2.edge_id);

                    // NOTE: potential overflow here if we hit 2^32 routable edges
                    BOOST_ASSERT(m_edge_based_edge_list.size() <=
                                 std::numeric_limits<NodeID>::max());
                    auto turn_id = m_edge_based_edge_list.size();
                    auto weight =
                        boost::numeric_cast<EdgeWeight>(edge_data1.weight + weight_penalty);
                    auto duration =
                        boost::numeric_cast<EdgeWeight>(edge_data1.duration + duration_penalty);
                    m_edge_based_edge_list.emplace_back(edge_data1.edge_id,
                                                        edge_data2.edge_id,
                                                        turn_id,
                                                        weight,
                                                        duration,
                                                        true,
                                                        false);

                    BOOST_ASSERT(turn_weight_penalties.size() == turn_id);
                    turn_weight_penalties.push_back(weight_penalty);
                    BOOST_ASSERT(turn_duration_penalties.size() == turn_id);
                    turn_duration_penalties.push_back(duration_penalty);

                    // We write out the mapping between the edge-expanded edges and the
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
                        isTrivial ? m_osm_node_ids[node_along_road_entering]
                                  : m_osm_node_ids[m_compressed_edge_container.GetLastEdgeSourceID(
                                        incoming_edge)];
                    const auto &via_node =
                        m_osm_node_ids[m_compressed_edge_container.GetLastEdgeTargetID(
                            incoming_edge)];
                    const auto &to_node =
                        m_osm_node_ids[m_compressed_edge_container.GetFirstEdgeTargetID(turn.eid)];

                    lookup::TurnIndexBlock turn_index_block = {from_node, via_node, to_node};

                    turn_penalties_index_file.WriteOne(turn_index_block);
                }
            }
        }
    }

    // write weight penalties per turn
    BOOST_ASSERT(turn_weight_penalties.size() == turn_duration_penalties.size());
    {
        storage::io::FileWriter writer(turn_weight_penalties_filename,
                                       storage::io::FileWriter::HasNoFingerprint);
        storage::serialization::write(writer, turn_weight_penalties);
    }

    {
        storage::io::FileWriter writer(turn_duration_penalties_filename,
                                       storage::io::FileWriter::HasNoFingerprint);
        storage::serialization::write(writer, turn_duration_penalties);
    }

    util::Log() << "Created " << entry_class_hash.size() << " entry classes and "
                << bearing_class_hash.size() << " Bearing Classes";

    util::Log() << "Writing Turn Lane Data to File...";
    {
        storage::io::FileWriter writer(turn_lane_data_filename,
                                       storage::io::FileWriter::HasNoFingerprint);

        std::vector<util::guidance::LaneTupleIdPair> lane_data(lane_data_map.size());
        // extract lane data sorted by ID
        for (auto itr : lane_data_map)
            lane_data[itr.second] = itr.first;

        storage::serialization::write(writer, lane_data);
    }
    util::Log() << "done.";

    files::writeTurnData(turn_data_filename, turn_data_container);

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
