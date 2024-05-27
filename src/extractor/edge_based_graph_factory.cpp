#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/conditional_turn_penalty.hpp"
#include "extractor/edge_based_edge.hpp"
#include "extractor/files.hpp"
#include "extractor/intersection/intersection_analysis.hpp"
#include "extractor/scripting_environment.hpp"
#include "extractor/serialization.hpp"
#include "extractor/suffix_table.hpp"

#include "storage/io.hpp"

#include "util/assert.hpp"
#include "util/connectivity_checksum.hpp"
#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/percent.hpp"
#include "util/timing_util.hpp"

#include <boost/assert.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <algorithm>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_pipeline.h>

namespace osrm::extractor
{

// Configuration to find representative candidate for turn angle calculations
EdgeBasedGraphFactory::EdgeBasedGraphFactory(
    const util::NodeBasedDynamicGraph &node_based_graph,
    EdgeBasedNodeDataContainer &node_data_container,
    const CompressedEdgeContainer &compressed_edge_container,
    const std::unordered_set<NodeID> &barrier_nodes,
    const TrafficSignals &traffic_signals,
    const std::vector<util::Coordinate> &coordinates,
    const NameTable &name_table,
    const std::unordered_set<EdgeID> &segregated_edges,
    const extractor::LaneDescriptionMap &lane_description_map)
    : m_edge_based_node_container(node_data_container), m_connectivity_checksum(0),
      m_number_of_edge_based_nodes(0), m_coordinates(coordinates),
      m_node_based_graph(node_based_graph), m_barrier_nodes(barrier_nodes),
      m_traffic_signals(traffic_signals), m_compressed_edge_container(compressed_edge_container),
      name_table(name_table), segregated_edges(segregated_edges),
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

void EdgeBasedGraphFactory::GetEdgeBasedNodeSegments(std::vector<EdgeBasedNodeSegment> &nodes)
{
    using std::swap; // Koenig swap
    swap(nodes, m_edge_based_node_segments);
}

void EdgeBasedGraphFactory::GetEdgeBasedNodeWeights(std::vector<EdgeWeight> &output_node_weights)
{
    using std::swap; // Koenig swap
    swap(m_edge_based_node_weights, output_node_weights);
}

void EdgeBasedGraphFactory::GetEdgeBasedNodeDurations(
    std::vector<EdgeDuration> &output_node_durations)
{
    using std::swap; // Koenig swap
    swap(m_edge_based_node_durations, output_node_durations);
}

void EdgeBasedGraphFactory::GetEdgeBasedNodeDistances(
    std::vector<EdgeDistance> &output_node_distances)
{
    using std::swap; // Koenig swap
    swap(m_edge_based_node_distances, output_node_distances);
}

std::uint32_t EdgeBasedGraphFactory::GetConnectivityChecksum() const
{
    return m_connectivity_checksum;
}

std::uint64_t EdgeBasedGraphFactory::GetNumberOfEdgeBasedNodes() const
{
    return m_number_of_edge_based_nodes;
}

NBGToEBG EdgeBasedGraphFactory::InsertEdgeBasedNode(const NodeID node_u, const NodeID node_v)
{
    // merge edges together into one EdgeBasedNode
    BOOST_ASSERT(node_u != SPECIAL_NODEID);
    BOOST_ASSERT(node_v != SPECIAL_NODEID);

    // find forward edge id and
    const EdgeID edge_id_1 = m_node_based_graph.FindEdge(node_u, node_v);
    BOOST_ASSERT(edge_id_1 != SPECIAL_EDGEID);

    const EdgeData &forward_data = m_node_based_graph.GetEdgeData(edge_id_1);

    // find reverse edge id and
    const EdgeID edge_id_2 = m_node_based_graph.FindEdge(node_v, node_u);
    BOOST_ASSERT(edge_id_2 != SPECIAL_EDGEID);

    const EdgeData &reverse_data = m_node_based_graph.GetEdgeData(edge_id_2);

    BOOST_ASSERT(nbe_to_ebn_mapping[edge_id_1] != SPECIAL_NODEID ||
                 nbe_to_ebn_mapping[edge_id_2] != SPECIAL_NODEID);

    // ⚠ Use the sign bit of node weights to distinguish oneway streets:
    //  * MSB is set - a node corresponds to a one-way street
    //  * MSB is clear - a node corresponds to a bidirectional street
    // Before using node weights data values must be adjusted:
    //  * in contraction if MSB is set the node weight is INVALID_EDGE_WEIGHT.
    //    This adjustment is needed to enforce loop creation for oneways.
    //  * in other cases node weights must be masked with 0x7fffffff to clear MSB
    if (nbe_to_ebn_mapping[edge_id_1] != SPECIAL_NODEID &&
        nbe_to_ebn_mapping[edge_id_2] == SPECIAL_NODEID)
        m_edge_based_node_weights[nbe_to_ebn_mapping[edge_id_1]] |=
            EdgeWeight{static_cast<EdgeWeight::value_type>(0x80000000)};

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

    // const unsigned packed_geometry_id = m_compressed_edge_container.ZipEdges(edge_id_1,
    // edge_id_2);

    NodeID current_edge_source_coordinate_id = node_u;

    const auto edge_id_to_segment_id = [](const NodeID edge_based_node_id)
    {
        if (edge_based_node_id == SPECIAL_NODEID)
        {
            return SegmentID{SPECIAL_SEGMENTID, false};
        }

        return SegmentID{edge_based_node_id, true};
    };

    // Add edge-based node data for forward and reverse nodes indexed by edge_id
    BOOST_ASSERT(nbe_to_ebn_mapping[edge_id_1] != SPECIAL_EDGEID);
    m_edge_based_node_container.nodes[nbe_to_ebn_mapping[edge_id_1]].geometry_id =
        forward_data.geometry_id;
    m_edge_based_node_container.nodes[nbe_to_ebn_mapping[edge_id_1]].annotation_id =
        forward_data.annotation_data;
    m_edge_based_node_container.nodes[nbe_to_ebn_mapping[edge_id_1]].segregated =
        segregated_edges.contains(edge_id_1);

    if (nbe_to_ebn_mapping[edge_id_2] != SPECIAL_EDGEID)
    {
        m_edge_based_node_container.nodes[nbe_to_ebn_mapping[edge_id_2]].geometry_id =
            reverse_data.geometry_id;
        m_edge_based_node_container.nodes[nbe_to_ebn_mapping[edge_id_2]].annotation_id =
            reverse_data.annotation_data;
        m_edge_based_node_container.nodes[nbe_to_ebn_mapping[edge_id_2]].segregated =
            segregated_edges.contains(edge_id_2);
    }

    // Add segments of edge-based nodes
    for (const auto i : util::irange(std::size_t{0}, segment_count))
    {
        BOOST_ASSERT(
            current_edge_source_coordinate_id ==
            m_compressed_edge_container.GetBucketReference(edge_id_2)[segment_count - 1 - i]
                .node_id);
        const NodeID current_edge_target_coordinate_id = forward_geometry[i].node_id;

        // don't add node-segments for penalties
        if (current_edge_target_coordinate_id == current_edge_source_coordinate_id)
            continue;

        BOOST_ASSERT(current_edge_target_coordinate_id != current_edge_source_coordinate_id);

        // build edges
        m_edge_based_node_segments.emplace_back(
            edge_id_to_segment_id(nbe_to_ebn_mapping[edge_id_1]),
            edge_id_to_segment_id(nbe_to_ebn_mapping[edge_id_2]),
            current_edge_source_coordinate_id,
            current_edge_target_coordinate_id,
            i,
            forward_data.flags.startpoint || reverse_data.flags.startpoint);

        current_edge_source_coordinate_id = current_edge_target_coordinate_id;
    }

    BOOST_ASSERT(current_edge_source_coordinate_id == node_v);

    return NBGToEBG{node_u, node_v, nbe_to_ebn_mapping[edge_id_1], nbe_to_ebn_mapping[edge_id_2]};
}

void EdgeBasedGraphFactory::Run(
    ScriptingEnvironment &scripting_environment,
    const std::string &turn_weight_penalties_filename,
    const std::string &turn_duration_penalties_filename,
    const std::string &turn_penalties_index_filename,
    const std::string &cnbg_ebg_mapping_path,
    const std::string &conditional_penalties_filename,
    const std::string &maneuver_overrides_filename,
    const RestrictionMap &unconditional_node_restriction_map,
    const ConditionalRestrictionMap &conditional_node_restriction_map,
    const WayRestrictionMap &way_restriction_map,
    const std::vector<UnresolvedManeuverOverride> &unresolved_maneuver_overrides)
{
    TIMER_START(renumber);
    m_number_of_edge_based_nodes =
        LabelEdgeBasedNodes() + way_restriction_map.NumberOfDuplicatedNodes();
    TIMER_STOP(renumber);

    // Allocate memory for edge-based nodes
    // In addition to the normal edges, allocate enough space for copied edges from
    // via-way-restrictions, see calculation above
    m_edge_based_node_container.nodes.resize(m_number_of_edge_based_nodes);

    TIMER_START(generate_nodes);
    {
        auto mapping = GenerateEdgeExpandedNodes(way_restriction_map);
        files::writeNBGMapping(cnbg_ebg_mapping_path, mapping);
    }
    TIMER_STOP(generate_nodes);

    TIMER_START(generate_edges);
    GenerateEdgeExpandedEdges(scripting_environment,
                              turn_weight_penalties_filename,
                              turn_duration_penalties_filename,
                              turn_penalties_index_filename,
                              conditional_penalties_filename,
                              maneuver_overrides_filename,
                              unconditional_node_restriction_map,
                              conditional_node_restriction_map,
                              way_restriction_map,
                              unresolved_maneuver_overrides);

    TIMER_STOP(generate_edges);

    util::Log() << "Timing statistics for edge-expanded graph:";
    util::Log() << "Renumbering edges: " << TIMER_SEC(renumber) << "s";
    util::Log() << "Generating nodes: " << TIMER_SEC(generate_nodes) << "s";
    util::Log() << "Generating edges: " << TIMER_SEC(generate_edges) << "s";
}

/// Renumbers all _forward_ edges and sets the edge_id.
/// A specific numbering is not important. Any unique ID will do.
/// Returns the number of edge-based nodes.
unsigned EdgeBasedGraphFactory::LabelEdgeBasedNodes()
{
    // heuristic: node-based graph node is a simple intersection with four edges
    // (edge-based nodes)
    constexpr std::size_t ESTIMATED_EDGE_COUNT = 4;
    m_edge_based_node_weights.reserve(ESTIMATED_EDGE_COUNT * m_node_based_graph.GetNumberOfNodes());
    m_edge_based_node_durations.reserve(ESTIMATED_EDGE_COUNT *
                                        m_node_based_graph.GetNumberOfNodes());
    m_edge_based_node_distances.reserve(ESTIMATED_EDGE_COUNT *
                                        m_node_based_graph.GetNumberOfNodes());
    nbe_to_ebn_mapping.resize(m_node_based_graph.GetEdgeCapacity(), SPECIAL_NODEID);

    // renumber edge based node of outgoing edges
    unsigned numbered_edges_count = 0;
    for (const auto current_node : util::irange(0u, m_node_based_graph.GetNumberOfNodes()))
    {
        for (const auto current_edge : m_node_based_graph.GetAdjacentEdgeRange(current_node))
        {
            const EdgeData &edge_data = m_node_based_graph.GetEdgeData(current_edge);
            // only number incoming edges
            if (edge_data.reversed)
            {
                continue;
            }

            m_edge_based_node_weights.push_back(edge_data.weight);
            m_edge_based_node_durations.push_back(edge_data.duration);
            m_edge_based_node_distances.push_back(edge_data.distance);

            BOOST_ASSERT(numbered_edges_count < m_node_based_graph.GetNumberOfEdges());
            nbe_to_ebn_mapping[current_edge] = numbered_edges_count;
            ++numbered_edges_count;
        }
    }

    return numbered_edges_count;
}

// Creates the nodes in the edge expanded graph from edges in the node-based graph.
std::vector<NBGToEBG>
EdgeBasedGraphFactory::GenerateEdgeExpandedNodes(const WayRestrictionMap &way_restriction_map)
{
    std::vector<NBGToEBG> mapping;

    util::Log() << "Generating edge expanded nodes ... ";
    // indicating a normal node within the edge-based graph. This node represents an edge in the
    // node-based graph
    {
        util::UnbufferedLog log;
        util::Percent progress(log, m_node_based_graph.GetNumberOfNodes());

        // m_compressed_edge_container.InitializeBothwayVector();

        // loop over all edges and generate new set of nodes
        for (const auto nbg_node_u : util::irange(0u, m_node_based_graph.GetNumberOfNodes()))
        {
            BOOST_ASSERT(nbg_node_u != SPECIAL_NODEID);
            progress.PrintStatus(nbg_node_u);
            for (EdgeID nbg_edge_id : m_node_based_graph.GetAdjacentEdgeRange(nbg_node_u))
            {
                BOOST_ASSERT(nbg_edge_id != SPECIAL_EDGEID);

                const NodeID nbg_node_v = m_node_based_graph.GetTarget(nbg_edge_id);
                BOOST_ASSERT(nbg_node_v != SPECIAL_NODEID);
                BOOST_ASSERT(nbg_node_u != nbg_node_v);

                // pick only every other edge, since we have every edge as an outgoing and incoming
                // egde
                if (nbg_node_u >= nbg_node_v)
                {
                    continue;
                }

                // if we found a non-forward edge reverse and try again
                if (nbe_to_ebn_mapping[nbg_edge_id] == SPECIAL_NODEID)
                {
                    mapping.push_back(InsertEdgeBasedNode(nbg_node_v, nbg_node_u));
                }
                else
                {
                    mapping.push_back(InsertEdgeBasedNode(nbg_node_u, nbg_node_v));
                }
            }
        }
    }

    util::Log() << "Expanding via-way turn restrictions ... ";
    // Add copies of the nodes
    {
        util::UnbufferedLog log;
        const auto via_edges = way_restriction_map.DuplicatedViaEdges();
        util::Percent progress(log, via_edges.size());

        NodeID edge_based_node_id =
            NodeID(m_number_of_edge_based_nodes - way_restriction_map.NumberOfDuplicatedNodes());
        std::size_t progress_counter = 0;
        // allocate enough space for the mapping
        for (const auto edge : via_edges)
        {
            const auto node_u = edge.from;
            const auto node_v = edge.to;
            // we know that the edge exists as non-reversed edge
            const auto eid = m_node_based_graph.FindEdge(node_u, node_v);

            BOOST_ASSERT(nbe_to_ebn_mapping[eid] != SPECIAL_NODEID);

            // merge edges together into one EdgeBasedNode
            BOOST_ASSERT(node_u != SPECIAL_NODEID);
            BOOST_ASSERT(node_v != SPECIAL_NODEID);

            // find node in the edge based graph, we only require one id:
            const EdgeData &edge_data = m_node_based_graph.GetEdgeData(eid);
            // BOOST_ASSERT(edge_data.edge_id < m_edge_based_node_container.Size());
            m_edge_based_node_container.nodes[edge_based_node_id].geometry_id =
                edge_data.geometry_id;
            m_edge_based_node_container.nodes[edge_based_node_id].annotation_id =
                edge_data.annotation_data;
            m_edge_based_node_container.nodes[edge_based_node_id].segregated =
                segregated_edges.contains(eid);

            const auto ebn_weight = m_edge_based_node_weights[nbe_to_ebn_mapping[eid]];
            BOOST_ASSERT((ebn_weight & EdgeWeight{0x7fffffff}) == edge_data.weight);
            m_edge_based_node_weights.push_back(ebn_weight);
            m_edge_based_node_durations.push_back(
                m_edge_based_node_durations[nbe_to_ebn_mapping[eid]]);
            m_edge_based_node_distances.push_back(
                m_edge_based_node_distances[nbe_to_ebn_mapping[eid]]);

            // Include duplicate nodes in cnbg to ebg mapping. This means a
            // compressed node pair (u,v) can appear multiple times in this list.
            // This is needed by the MLD partition step to ensure duplicate nodes
            // are also assigned to partitions (the MLD partitioner is currently
            // the only consumer of this mapping).
            mapping.push_back(NBGToEBG{node_u, node_v, edge_based_node_id, SPECIAL_NODEID});

            // We also want to include duplicate via edges in the list of segments that
            // an input location can snap to. Without this, it would be possible to not find
            // certain routes that end on a via-way, because they are only routable via the
            // duplicated edge.
            const auto &forward_geometry = m_compressed_edge_container.GetBucketReference(eid);
            const auto segment_count = forward_geometry.size();

            NodeID current_edge_source_coordinate_id = node_u;
            const EdgeData &forward_data = m_node_based_graph.GetEdgeData(eid);

            const auto edge_id_to_segment_id = [](const NodeID edge_based_node_id)
            {
                if (edge_based_node_id == SPECIAL_NODEID)
                {
                    return SegmentID{SPECIAL_SEGMENTID, false};
                }

                return SegmentID{edge_based_node_id, true};
            };

            // Add segments of edge-based nodes
            for (const auto i : util::irange(std::size_t{0}, segment_count))
            {
                const NodeID current_edge_target_coordinate_id = forward_geometry[i].node_id;

                // don't add node-segments for penalties
                if (current_edge_target_coordinate_id == current_edge_source_coordinate_id)
                    continue;

                BOOST_ASSERT(current_edge_target_coordinate_id !=
                             current_edge_source_coordinate_id);

                // build edges
                m_edge_based_node_segments.emplace_back(edge_id_to_segment_id(edge_based_node_id),
                                                        SegmentID{SPECIAL_SEGMENTID, false},
                                                        current_edge_source_coordinate_id,
                                                        current_edge_target_coordinate_id,
                                                        i,
                                                        forward_data.flags.startpoint);

                current_edge_source_coordinate_id = current_edge_target_coordinate_id;
            }

            edge_based_node_id++;
            progress.PrintStatus(progress_counter++);
        }
    }

    BOOST_ASSERT(m_number_of_edge_based_nodes == m_edge_based_node_weights.size());
    BOOST_ASSERT(m_number_of_edge_based_nodes == m_edge_based_node_durations.size());
    BOOST_ASSERT(m_number_of_edge_based_nodes == m_edge_based_node_distances.size());

    util::Log() << "Generated " << m_number_of_edge_based_nodes << " nodes ("
                << way_restriction_map.NumberOfDuplicatedNodes()
                << " of which are duplicates)  and " << m_edge_based_node_segments.size()
                << " segments in edge-expanded graph";

    return mapping;
}

/// Actually it also generates turn data and serializes them...
void EdgeBasedGraphFactory::GenerateEdgeExpandedEdges(
    ScriptingEnvironment &scripting_environment,
    const std::string &turn_weight_penalties_filename,
    const std::string &turn_duration_penalties_filename,
    const std::string &turn_penalties_index_filename,
    const std::string &conditional_penalties_filename,
    const std::string &maneuver_overrides_filename,
    const RestrictionMap &unconditional_node_restriction_map,
    const ConditionalRestrictionMap &conditional_node_restriction_map,
    const WayRestrictionMap &way_restriction_map,
    const std::vector<UnresolvedManeuverOverride> &unresolved_maneuver_overrides)
{
    util::Log() << "Generating edge-expanded edges ";

    // Keep a set of all maneuver turns so we can identify them as
    // we generate the edge-expansion.
    std::unordered_set<NodeBasedTurn> unresolved_turns;
    for (const auto &manuevers : unresolved_maneuver_overrides)
    {
        for (const auto &turn : manuevers.Turns())
        {
            unresolved_turns.insert(turn);
        }
    }

    std::size_t node_based_edge_counter = 0;

    SuffixTable street_name_suffix_table(scripting_environment);
    const auto &turn_lanes_data = transformTurnLaneMapIntoArrays(lane_description_map);
    intersection::MergableRoadDetector mergable_road_detector(m_node_based_graph,
                                                              m_edge_based_node_container,
                                                              m_coordinates,
                                                              m_compressed_edge_container,
                                                              unconditional_node_restriction_map,
                                                              m_barrier_nodes,
                                                              turn_lanes_data,
                                                              name_table,
                                                              street_name_suffix_table);

    // FIXME these need to be tuned in pre-allocated size
    std::vector<TurnPenalty> turn_weight_penalties;
    std::vector<TurnPenalty> turn_duration_penalties;
    std::vector<lookup::TurnIndexBlock> turn_penalties_index;

    // Now, renumber all our maneuver overrides to use edge-based-nodes
    std::vector<StorageManeuverOverride> storage_maneuver_overrides;
    std::vector<NodeID> maneuver_override_sequences;

    const auto weight_multiplier =
        scripting_environment.GetProfileProperties().GetWeightMultiplier();

    // filled in during next stage, kept alive through following scope
    std::vector<Conditional> conditionals;
    // The following block generates the edge-based-edges using a parallel processing pipeline.
    // Sets of intersection IDs are batched in groups of GRAINSIZE (100) `generator_stage`, then
    // those groups are processed in parallel `processor_stage`.  Finally, results are appended to
    // the various buffer vectors by the `output_stage` in the same order that the `generator_stage`
    // created them in (tbb::filter_mode::serial_in_order creates this guarantee).  The order needs
    // to be maintained because we depend on it later in the processing pipeline.
    {
        const NodeID node_count = m_node_based_graph.GetNumberOfNodes();

        // This struct is the buffered output of the `processor_stage`.  This data is
        // appended to the various output arrays/files by the `output_stage`.
        // same as IntersectionData, but grouped with edge to allow sorting after creating.
        struct EdgeWithData
        {
            EdgeBasedEdge edge;
            lookup::TurnIndexBlock turn_index;
            TurnPenalty turn_weight_penalty;
            TurnPenalty turn_duration_penalty;
        };

        auto const transfer_data = [&](const EdgeWithData &edge_with_data)
        {
            m_edge_based_edge_list.push_back(edge_with_data.edge);
            turn_weight_penalties.push_back(edge_with_data.turn_weight_penalty);
            turn_duration_penalties.push_back(edge_with_data.turn_duration_penalty);
            turn_penalties_index.push_back(edge_with_data.turn_index);
        };

        struct EdgesPipelineBuffer
        {
            std::size_t nodes_processed = 0;

            std::vector<EdgeWithData> continuous_data; // may need this
            std::vector<EdgeWithData> delayed_data;    // may need this
            std::vector<Conditional> conditionals;

            std::unordered_multimap<NodeBasedTurn, std::pair<NodeID, NodeID>> turn_to_ebn_map;

            util::ConnectivityChecksum checksum;
        };
        using EdgesPipelineBufferPtr = std::shared_ptr<EdgesPipelineBuffer>;

        m_connectivity_checksum = 0;

        std::unordered_multimap<NodeBasedTurn, std::pair<NodeID, NodeID>> global_turn_to_ebn_map;

        // going over all nodes (which form the center of an intersection), we compute all possible
        // turns along these intersections.
        NodeID current_node = 0;

        // Handle intersections in sets of 100.  The pipeline below has a serial bottleneck during
        // the writing phase, so we want to make the parallel workers do more work to give the
        // serial final stage time to complete its tasks.
        const constexpr unsigned GRAINSIZE = 100;

        // First part of the pipeline generates iterator ranges of IDs in sets of GRAINSIZE
        tbb::filter<void, tbb::blocked_range<NodeID>> generator_stage(
            tbb::filter_mode::serial_in_order,
            [&](tbb::flow_control &fc)
            {
                if (current_node < node_count)
                {
                    auto next_node = std::min(current_node + GRAINSIZE, node_count);
                    auto result = tbb::blocked_range<NodeID>(current_node, next_node);
                    current_node = next_node;
                    return result;
                }
                else
                {
                    fc.stop();
                    return tbb::blocked_range<NodeID>(node_count, node_count);
                }
            });

        // Generate edges for either artificial nodes or the main graph
        const auto generate_edge = [this, &scripting_environment, weight_multiplier](
                                       // what nodes will be used? In most cases this will be the id
                                       // stored in the edge_data. In case of duplicated nodes (e.g.
                                       // due to via-way restrictions), one/both of these might
                                       // refer to a newly added edge based node
                                       const auto edge_based_node_from,
                                       const auto edge_based_node_to,
                                       // the situation of the turn
                                       const auto node_along_road_entering,
                                       const auto node_based_edge_from,
                                       const auto intersection_node,
                                       const auto node_based_edge_to,
                                       const auto &turn_angle,
                                       const auto &road_legs_on_the_right,
                                       const auto &road_legs_on_the_left,
                                       const auto &edge_geometries)
        {
            const auto &edge_data1 = m_node_based_graph.GetEdgeData(node_based_edge_from);
            const auto &edge_data2 = m_node_based_graph.GetEdgeData(node_based_edge_to);

            BOOST_ASSERT(nbe_to_ebn_mapping[node_based_edge_from] !=
                         nbe_to_ebn_mapping[node_based_edge_to]);
            BOOST_ASSERT(!edge_data1.reversed);
            BOOST_ASSERT(!edge_data2.reversed);

            // compute weight and duration penalties
            // In theory we shouldn't get a directed traffic light on a turn, as it indicates that
            // the traffic signal direction was potentially ambiguously annotated on the junction
            // node But we'll check anyway.
            const auto is_traffic_light =
                m_traffic_signals.HasSignal(node_along_road_entering, intersection_node);
            const auto is_uturn =
                guidance::getTurnDirection(turn_angle) == guidance::DirectionModifier::UTurn;

            ExtractionTurn extracted_turn(
                // general info
                turn_angle,
                road_legs_on_the_right.size() + road_legs_on_the_left.size() + 2 - is_uturn,
                is_uturn,
                is_traffic_light,
                m_edge_based_node_container.GetAnnotation(edge_data1.annotation_data)
                    .is_left_hand_driving,
                // source info
                edge_data1.flags.restricted,
                m_edge_based_node_container.GetAnnotation(edge_data1.annotation_data).travel_mode,
                edge_data1.flags.road_classification.IsMotorwayClass(),
                edge_data1.flags.road_classification.IsLinkClass(),
                edge_data1.flags.road_classification.GetNumberOfLanes(),
                edge_data1.flags.highway_turn_classification,
                edge_data1.flags.access_turn_classification,
                ((double)intersection::findEdgeLength(edge_geometries, node_based_edge_from) /
                 from_alias<double>(edge_data1.duration)) *
                    36,
                edge_data1.flags.road_classification.GetPriority(),
                // target info
                edge_data2.flags.restricted,
                m_edge_based_node_container.GetAnnotation(edge_data2.annotation_data).travel_mode,
                edge_data2.flags.road_classification.IsMotorwayClass(),
                edge_data2.flags.road_classification.IsLinkClass(),
                edge_data2.flags.road_classification.GetNumberOfLanes(),
                edge_data2.flags.highway_turn_classification,
                edge_data2.flags.access_turn_classification,
                ((double)intersection::findEdgeLength(edge_geometries, node_based_edge_to) /
                 from_alias<double>(edge_data2.duration)) *
                    36,
                edge_data2.flags.road_classification.GetPriority(),
                // connected roads
                road_legs_on_the_right,
                road_legs_on_the_left);

            scripting_environment.ProcessTurn(extracted_turn);

            // turn penalties are limited to [-2^15, 2^15) which roughly translates to 54 minutes
            // and fits signed 16bit deci-seconds
            auto weight_penalty = TurnPenalty{boost::numeric_cast<TurnPenalty::value_type>(
                extracted_turn.weight * weight_multiplier)};
            auto duration_penalty = TurnPenalty{
                boost::numeric_cast<TurnPenalty::value_type>(extracted_turn.duration * 10.)};

            BOOST_ASSERT(SPECIAL_NODEID != nbe_to_ebn_mapping[node_based_edge_from]);
            BOOST_ASSERT(SPECIAL_NODEID != nbe_to_ebn_mapping[node_based_edge_to]);

            // auto turn_id = m_edge_based_edge_list.size();
            auto weight = edge_data1.weight + alias_cast<EdgeWeight>(weight_penalty);
            auto duration = edge_data1.duration + alias_cast<EdgeDuration>(duration_penalty);
            auto distance = edge_data1.distance;

            EdgeBasedEdge edge_based_edge = {edge_based_node_from,
                                             edge_based_node_to,
                                             SPECIAL_NODEID, // This will be updated once the main
                                                             // loop completes!
                                             weight,
                                             duration,
                                             distance,
                                             true,
                                             false};

            // We write out the mapping between the edge-expanded edges and the original nodes.
            // Since each edge represents a possible maneuver, external programs can use this to
            // quickly perform updates to edge weights in order to penalize certain turns.

            // If this edge is 'trivial' -- where the compressed edge corresponds exactly to an
            // original OSM segment -- we can pull the turn's preceding node ID directly with
            // `node_along_road_entering`;
            // otherwise, we need to look up the node immediately preceding the turn from the
            // compressed edge container.
            const bool isTrivial = m_compressed_edge_container.IsTrivial(node_based_edge_from);

            const auto &from_node =
                isTrivial ? node_along_road_entering
                          : m_compressed_edge_container.GetLastEdgeSourceID(node_based_edge_from);

            const auto &to_node =
                m_compressed_edge_container.GetFirstEdgeTargetID(node_based_edge_to);

            lookup::TurnIndexBlock turn_index_block = {from_node, intersection_node, to_node};

            // insert data into the designated buffer
            return EdgeWithData{
                edge_based_edge, turn_index_block, weight_penalty, duration_penalty};
        };

        //
        // Edge-based-graph stage
        //
        tbb::filter<tbb::blocked_range<NodeID>, EdgesPipelineBufferPtr> processor_stage(
            tbb::filter_mode::parallel,
            [&](const tbb::blocked_range<NodeID> &intersection_node_range)
            {
                auto buffer = std::make_shared<EdgesPipelineBuffer>();
                buffer->nodes_processed = intersection_node_range.size();

                for (auto intersection_node = intersection_node_range.begin(),
                          end = intersection_node_range.end();
                     intersection_node < end;
                     ++intersection_node)
                {
                    // We capture the thread-local work in these objects, then flush them in a
                    // controlled manner at the end of the parallel range
                    const auto &incoming_edges =
                        intersection::getIncomingEdges(m_node_based_graph, intersection_node);
                    const auto &outgoing_edges =
                        intersection::getOutgoingEdges(m_node_based_graph, intersection_node);

                    intersection::IntersectionEdgeGeometries edge_geometries;
                    std::unordered_set<EdgeID> merged_edge_ids;
                    std::tie(edge_geometries, merged_edge_ids) =
                        intersection::getIntersectionGeometries(m_node_based_graph,
                                                                m_compressed_edge_container,
                                                                m_coordinates,
                                                                mergable_road_detector,
                                                                intersection_node);

                    buffer->checksum.process_byte(incoming_edges.size());
                    buffer->checksum.process_byte(outgoing_edges.size());

                    // all nodes in the graph are connected in both directions. We check all
                    // outgoing nodes to find the incoming edge. This is a larger search overhead,
                    // but the cost we need to pay to generate edges here is worth the additional
                    // search overhead.
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
                    // From the flags alone, we cannot determine which nodes are connected to `b` by
                    // an outgoing edge. Therefore, we have to search all connected edges for edges
                    // entering `b`

                    for (const auto &incoming_edge : incoming_edges)
                    {
                        ++node_based_edge_counter;

                        const auto connected_roads =
                            extractor::intersection::getConnectedRoadsForEdgeGeometries(
                                m_node_based_graph,
                                m_edge_based_node_container,
                                unconditional_node_restriction_map,
                                m_barrier_nodes,
                                turn_lanes_data,
                                incoming_edge,
                                edge_geometries,
                                merged_edge_ids);

                        // check if this edge is part of a restriction via-way
                        const auto is_restriction_via_edge =
                            way_restriction_map.IsViaWayEdge(incoming_edge.node, intersection_node);

                        for (const auto &outgoing_edge : outgoing_edges)
                        {
                            auto is_turn_allowed =
                                intersection::isTurnAllowed(m_node_based_graph,
                                                            m_edge_based_node_container,
                                                            unconditional_node_restriction_map,
                                                            m_barrier_nodes,
                                                            edge_geometries,
                                                            turn_lanes_data,
                                                            incoming_edge,
                                                            outgoing_edge);
                            buffer->checksum.process_bit(is_turn_allowed);

                            if (!is_turn_allowed)
                                continue;

                            const auto turn =
                                std::find_if(connected_roads.begin(),
                                             connected_roads.end(),
                                             [edge = outgoing_edge.edge](const auto &road)
                                             { return road.eid == edge; });
                            OSRM_ASSERT(turn != connected_roads.end(),
                                        m_coordinates[intersection_node]);

                            std::vector<ExtractionTurnLeg> road_legs_on_the_right;
                            std::vector<ExtractionTurnLeg> road_legs_on_the_left;

                            auto get_connected_road_info = [&](const auto &connected_edge)
                            {
                                const auto &edge_data =
                                    m_node_based_graph.GetEdgeData(connected_edge.eid);

                                bool is_incoming, is_outgoing;
                                if (edge_data.reversed)
                                {
                                    // If getConnectedRoads adds reversed edge it means
                                    // this edge is incoming-only
                                    is_incoming = true;
                                    is_outgoing = false;
                                }
                                else
                                {
                                    // It does not add incoming edge if there is outgoing so we
                                    // should find it ourselves
                                    is_incoming = false;
                                    auto reversed_edge = m_node_based_graph.FindEdge(
                                        m_node_based_graph.GetTarget(connected_edge.eid),
                                        intersection_node);
                                    if (reversed_edge != SPECIAL_EDGEID)
                                    {
                                        const auto &reversed_edge_data =
                                            m_node_based_graph.GetEdgeData(reversed_edge);

                                        if (!reversed_edge_data.reversed)
                                        {
                                            is_incoming = true;
                                        }
                                    }

                                    is_outgoing = true;
                                }

                                return ExtractionTurnLeg(
                                    edge_data.flags.restricted,
                                    edge_data.flags.road_classification.IsMotorwayClass(),
                                    edge_data.flags.road_classification.IsLinkClass(),
                                    edge_data.flags.road_classification.GetNumberOfLanes(),
                                    edge_data.flags.highway_turn_classification,
                                    edge_data.flags.access_turn_classification,
                                    ((double)intersection::findEdgeLength(edge_geometries,
                                                                          connected_edge.eid) /
                                     from_alias<double>(edge_data.duration)) *
                                        36,
                                    edge_data.flags.road_classification.GetPriority(),
                                    is_incoming,
                                    is_outgoing);
                            };

                            // all connected roads on the right of a u turn
                            const auto is_uturn = guidance::getTurnDirection(turn->angle) ==
                                                  guidance::DirectionModifier::UTurn;
                            if (is_uturn)
                            {
                                if (turn != connected_roads.begin())
                                {
                                    std::transform(connected_roads.begin() + 1,
                                                   turn,
                                                   std::back_inserter(road_legs_on_the_right),
                                                   get_connected_road_info);
                                }

                                std::transform(turn + 1,
                                               connected_roads.end(),
                                               std::back_inserter(road_legs_on_the_right),
                                               get_connected_road_info);
                            }
                            else
                            {
                                if (connected_roads.begin() != turn)
                                {
                                    std::transform(connected_roads.begin() + 1,
                                                   turn,
                                                   std::back_inserter(road_legs_on_the_right),
                                                   get_connected_road_info);
                                }
                                std::transform(turn + 1,
                                               connected_roads.end(),
                                               std::back_inserter(road_legs_on_the_left),
                                               get_connected_road_info);
                            }

                            if (is_uturn && turn != connected_roads.begin())
                            {
                                util::Log(logWARNING)
                                    << "Turn is a u turn but not turning to the first connected "
                                       "edge of the intersection. Node ID: "
                                    << intersection_node << ", OSM link: "
                                    << toOSMLink(m_coordinates[intersection_node]);
                            }
                            else if (turn == connected_roads.begin() && !is_uturn)
                            {
                                util::Log(logWARNING)
                                    << "Turn is a u turn but not classified as a u turn. Node ID: "
                                    << intersection_node << ", OSM link: "
                                    << toOSMLink(m_coordinates[intersection_node]);
                            }

                            // In case a way restriction starts at a given location, add a turn onto
                            // every artificial node emanating here.
                            //
                            //     e - f
                            //     |
                            // a - b
                            //     |
                            //     c - d
                            //
                            // ab via bc to cd
                            // ab via be to ef
                            //
                            // has two artificial nodes (be/bc) with restrictions starting at `ab`.
                            // Since every restriction group (abc | abe) refers to the same
                            // artificial node, we simply have to find a single representative for
                            // the turn. Here we check whether the turn in question is the start of
                            // a via way restriction. If that should be the case, we switch the id
                            // of the edge-based-node for the target to the ID of the duplicated
                            // node associated with the turn. (e.g. ab via bc switches bc to bc_dup)
                            auto const target_id = way_restriction_map.RemapIfRestrictionStart(
                                nbe_to_ebn_mapping[outgoing_edge.edge],
                                incoming_edge.node,
                                outgoing_edge.node,
                                m_node_based_graph.GetTarget(outgoing_edge.edge),
                                m_number_of_edge_based_nodes);

                            /***************************/

                            const auto outgoing_edge_target =
                                m_node_based_graph.GetTarget(outgoing_edge.edge);

                            const auto turn_nodes = NodeBasedTurn{
                                incoming_edge.node, intersection_node, outgoing_edge_target};
                            const auto is_maneuver_turn = unresolved_turns.contains(turn_nodes);

                            if (is_maneuver_turn)
                            {
                                const auto &ebn_from = nbe_to_ebn_mapping[incoming_edge.edge];
                                const auto &ebn_to = target_id;
                                buffer->turn_to_ebn_map.insert(
                                    {turn_nodes, std::make_pair(ebn_from, ebn_to)});
                            }

                            { // scope to forget edge_with_data after
                                const auto edge_with_data =
                                    generate_edge(nbe_to_ebn_mapping[incoming_edge.edge],
                                                  target_id,
                                                  incoming_edge.node,
                                                  incoming_edge.edge,
                                                  outgoing_edge.node,
                                                  outgoing_edge.edge,
                                                  turn->angle,
                                                  road_legs_on_the_right,
                                                  road_legs_on_the_left,
                                                  edge_geometries);

                                buffer->continuous_data.push_back(edge_with_data);

                                // get conditional restrictions that apply to this turn
                                const auto &restrictions =
                                    conditional_node_restriction_map.Restrictions(
                                        incoming_edge.node,
                                        outgoing_edge.node,
                                        outgoing_edge_target);
                                for (const auto &restriction : restrictions)
                                {
                                    buffer->conditionals.push_back(
                                        {nbe_to_ebn_mapping[incoming_edge.edge],
                                         target_id,
                                         {static_cast<std::uint64_t>(-1),
                                          m_coordinates[intersection_node],
                                          restriction->condition}});
                                }
                            }

                            // When on the edge of a via-way turn restriction, we need to not only
                            // handle the normal edges for the way, but also add turns for every
                            // duplicated node. This process is integrated here to avoid doing the
                            // turn analysis multiple times.
                            if (is_restriction_via_edge)
                            {
                                const auto duplicated_nodes = way_restriction_map.DuplicatedNodeIDs(
                                    incoming_edge.node, intersection_node);

                                // next to the normal restrictions tracked in `entry_allowed`, via
                                // ways might introduce additional restrictions. These are handled
                                // here when turning off a via-way
                                for (auto duplicated_node_id : duplicated_nodes)
                                {
                                    const auto from_id =
                                        NodeID(m_number_of_edge_based_nodes -
                                               way_restriction_map.NumberOfDuplicatedNodes() +
                                               duplicated_node_id);

                                    auto const node_at_end_of_turn =
                                        m_node_based_graph.GetTarget(outgoing_edge.edge);

                                    const auto is_restricted = way_restriction_map.IsRestricted(
                                        duplicated_node_id, node_at_end_of_turn);

                                    if (is_restricted)
                                    {
                                        auto const &restrictions =
                                            way_restriction_map.GetRestrictions(
                                                duplicated_node_id, node_at_end_of_turn);

                                        auto const has_unconditional =
                                            std::any_of(restrictions.begin(),
                                                        restrictions.end(),
                                                        [](const auto &restriction)
                                                        { return restriction->IsUnconditional(); });
                                        if (has_unconditional)
                                            continue;

                                        // From this via way, the outgoing edge will either:
                                        // a) take a conditional turn transferring to a via
                                        // path of an overlapping restriction.
                                        // b) take a conditional turn to exit the restriction.
                                        // If a) is applicable here, we change the target to be
                                        // the duplicate restriction node.
                                        auto const via_target_id =
                                            way_restriction_map.RemapIfRestrictionVia(
                                                nbe_to_ebn_mapping[outgoing_edge.edge],
                                                from_id,
                                                m_node_based_graph.GetTarget(outgoing_edge.edge),
                                                m_number_of_edge_based_nodes);

                                        // add into delayed data
                                        auto edge_with_data = generate_edge(from_id,
                                                                            via_target_id,
                                                                            incoming_edge.node,
                                                                            incoming_edge.edge,
                                                                            outgoing_edge.node,
                                                                            outgoing_edge.edge,
                                                                            turn->angle,
                                                                            road_legs_on_the_right,
                                                                            road_legs_on_the_left,
                                                                            edge_geometries);

                                        buffer->delayed_data.push_back(edge_with_data);

                                        // also add the conditions for the way
                                        for (const auto &restriction : restrictions)
                                        {
                                            // add a new conditional for the edge we just
                                            // created
                                            buffer->conditionals.push_back(
                                                {from_id,
                                                 via_target_id,
                                                 {static_cast<std::uint64_t>(-1),
                                                  m_coordinates[intersection_node],
                                                  restriction->condition}});
                                        }

                                        // We also need to track maneuvers that traverse duplicate
                                        // edges
                                        if (is_maneuver_turn)
                                        {
                                            const auto &ebn_from = from_id;
                                            const auto &ebn_to = via_target_id;
                                            buffer->turn_to_ebn_map.insert(
                                                {turn_nodes, std::make_pair(ebn_from, ebn_to)});
                                        }
                                    }
                                    else
                                    {
                                        // From this via way, the outgoing edge will either:
                                        // a) continue along the current via path
                                        // b) transfer to a via path of an overlapping restriction.
                                        // c) exit the restriction
                                        // If a) or b) are applicable here, we change the target to
                                        // be the duplicate restriction node.
                                        auto const via_target_id =
                                            way_restriction_map.RemapIfRestrictionVia(
                                                nbe_to_ebn_mapping[outgoing_edge.edge],
                                                from_id,
                                                m_node_based_graph.GetTarget(outgoing_edge.edge),
                                                m_number_of_edge_based_nodes);

                                        auto edge_with_data = generate_edge(from_id,
                                                                            via_target_id,
                                                                            incoming_edge.node,
                                                                            incoming_edge.edge,
                                                                            outgoing_edge.node,
                                                                            outgoing_edge.edge,
                                                                            turn->angle,
                                                                            road_legs_on_the_right,
                                                                            road_legs_on_the_left,
                                                                            edge_geometries);

                                        buffer->delayed_data.push_back(edge_with_data);

                                        // We also need to track maneuvers that traverse duplicate
                                        // edges
                                        if (is_maneuver_turn)
                                        {
                                            const auto &ebn_from = from_id;
                                            const auto &ebn_to = via_target_id;
                                            buffer->turn_to_ebn_map.insert(
                                                {turn_nodes, std::make_pair(ebn_from, ebn_to)});
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                return buffer;
            });

        // Last part of the pipeline puts all the calculated data into the serial buffers
        util::UnbufferedLog log;
        util::Percent routing_progress(log, node_count);
        std::vector<EdgeWithData> delayed_data;
        tbb::filter<EdgesPipelineBufferPtr, void> output_stage(
            tbb::filter_mode::serial_in_order,
            [&](auto buffer)
            {
                routing_progress.PrintAddition(buffer->nodes_processed);

                m_connectivity_checksum = buffer->checksum.update_checksum(m_connectivity_checksum);

                // Copy data from local buffers into global EBG data
                std::for_each(
                    buffer->continuous_data.begin(), buffer->continuous_data.end(), transfer_data);
                conditionals.insert(
                    conditionals.end(), buffer->conditionals.begin(), buffer->conditionals.end());

                // NOTE: potential overflow here if we hit 2^32 routable edges
                BOOST_ASSERT(m_edge_based_edge_list.size() <= std::numeric_limits<NodeID>::max());

                // Copy via-way restrictions delayed data
                delayed_data.insert(
                    delayed_data.end(), buffer->delayed_data.begin(), buffer->delayed_data.end());

                std::for_each(buffer->turn_to_ebn_map.begin(),
                              buffer->turn_to_ebn_map.end(),
                              [&global_turn_to_ebn_map](const auto &p)
                              {
                                  // TODO: log conflicts here
                                  global_turn_to_ebn_map.insert(p);
                              });
            });

        // Now, execute the pipeline.  The value of "5" here was chosen by experimentation
        // on a 16-CPU machine and seemed to give the best performance.  This value needs
        // to be balanced with the GRAINSIZE above - ideally, the pipeline puts as much work
        // as possible in the `intersection_handler` step so that those parallel workers don't
        // get blocked too much by the slower (io-performing) `buffer_storage`
        tbb::parallel_pipeline(std::thread::hardware_concurrency() * 5,
                               generator_stage & processor_stage & output_stage);

        // NOTE: buffer.delayed_data and buffer.delayed_turn_data have the same index
        std::for_each(delayed_data.begin(), delayed_data.end(), transfer_data);

        // Now, replace node-based-node ID values in the `node_sequence` with
        // the edge-based-node values we found and stored in the `turn_to_ebn_map`
        for (const auto &unresolved_override : unresolved_maneuver_overrides)
        {
            // There can be multiple edge-based-node sequences for a node-based-turn sequence
            // due to duplicate edges in the restriction graph.
            std::vector<std::vector<NodeID>> node_sequences;

            const auto &turns = unresolved_override.Turns();

            BOOST_ASSERT(!turns.empty());
            // Populate the node sequences with the first turn values.
            const auto first_turn_edges = global_turn_to_ebn_map.equal_range(turns[0]);
            std::transform(
                first_turn_edges.first,
                first_turn_edges.second,
                std::back_inserter(node_sequences),
                [](const auto turn_edges) {
                    return std::vector<NodeID>{turn_edges.second.first, turn_edges.second.second};
                });

            std::for_each(std::next(turns.begin()),
                          turns.end(),
                          [&](const auto &turn)
                          {
                              std::vector<std::vector<NodeID>> next_node_sequences;
                              const auto next_turn_edges = global_turn_to_ebn_map.equal_range(turn);
                              for (auto &node_sequence : node_sequences)
                              {
                                  const auto found_it = std::find_if(
                                      next_turn_edges.first,
                                      next_turn_edges.second,
                                      [&](const auto &turn_edges)
                                      {
                                          const auto pre_turn_edge = turn_edges.second.first;
                                          return (node_sequence.back() == pre_turn_edge);
                                      });

                                  if (found_it != next_turn_edges.second)
                                  {
                                      const auto post_turn_edge = found_it->second.second;
                                      node_sequence.push_back(post_turn_edge);
                                      next_node_sequences.push_back(std::move(node_sequence));
                                  }
                              }
                              node_sequences = std::move(next_node_sequences);
                          });

            for (const auto &node_sequence : node_sequences)
            {
                StorageManeuverOverride storage_override;
                storage_override.instruction_node = unresolved_override.instruction_node;
                storage_override.override_type = unresolved_override.override_type;
                storage_override.direction = unresolved_override.direction;

                storage_override.node_sequence_offset_begin = maneuver_override_sequences.size();
                storage_override.node_sequence_offset_end =
                    maneuver_override_sequences.size() + node_sequence.size();

                storage_override.start_node = node_sequence.front();

                maneuver_override_sequences.insert(
                    maneuver_override_sequences.end(), node_sequence.begin(), node_sequence.end());

                storage_maneuver_overrides.push_back(storage_override);
            }
        }
    }
    {
        util::Log() << "Sorting and writing " << storage_maneuver_overrides.size()
                    << " maneuver overrides...";

        // Sort by `from_node`, so that later lookups can be done with a binary search.
        std::sort(storage_maneuver_overrides.begin(),
                  storage_maneuver_overrides.end(),
                  [](const auto &a, const auto &b) { return a.start_node < b.start_node; });

        files::writeManeuverOverrides(
            maneuver_overrides_filename, storage_maneuver_overrides, maneuver_override_sequences);
    }

    util::Log() << "done.";
    util::Log() << "Renumbering turns";
    // Now, update the turn_id property on every EdgeBasedEdge - it will equal the position in the
    // m_edge_based_edge_list array for each object.
    tbb::parallel_for(tbb::blocked_range<NodeID>(0, m_edge_based_edge_list.size()),
                      [this](const tbb::blocked_range<NodeID> &range)
                      {
                          for (auto x = range.begin(), end = range.end(); x != end; ++x)
                          {
                              m_edge_based_edge_list[x].data.turn_id = x;
                          }
                      });

    // re-hash conditionals to connect to their respective edge-based edges. Due to the ordering, we
    // do not really have a choice but to index the conditional penalties and walk over all
    // edge-based-edges to find the ID of the edge
    auto const indexed_conditionals = IndexConditionals(std::move(conditionals));
    util::Log() << "Writing " << indexed_conditionals.size() << " conditional turn penalties...";
    extractor::files::writeConditionalRestrictions(conditional_penalties_filename,
                                                   indexed_conditionals);

    // write weight penalties per turn
    BOOST_ASSERT(turn_weight_penalties.size() == turn_duration_penalties.size() &&
                 turn_weight_penalties.size() == turn_penalties_index.size());
    files::writeTurnWeightPenalty(turn_weight_penalties_filename, turn_weight_penalties);
    files::writeTurnDurationPenalty(turn_duration_penalties_filename, turn_duration_penalties);
    files::writeTurnPenaltiesIndex(turn_penalties_index_filename, turn_penalties_index);

    util::Log() << "Generated " << m_edge_based_node_segments.size() << " edge based node segments";
    util::Log() << "Node-based graph contains " << node_based_edge_counter << " edges";
    util::Log() << "Edge-expanded graph ...";
    util::Log() << "  contains " << m_edge_based_edge_list.size() << " edges";
}

std::vector<ConditionalTurnPenalty>
EdgeBasedGraphFactory::IndexConditionals(std::vector<Conditional> &&conditionals) const
{
    std::unordered_multimap<std::pair<NodeID, NodeID>, ConditionalTurnPenalty *> index;

    // build and index of all conditional restrictions
    for (auto &conditional : conditionals)
        index.insert(std::make_pair(std::make_pair(conditional.from_node, conditional.to_node),
                                    &conditional.penalty));

    std::vector<ConditionalTurnPenalty> indexed_restrictions;

    for (auto const &edge : m_edge_based_edge_list)
    {
        auto const range = index.equal_range(std::make_pair(edge.source, edge.target));
        for (auto itr = range.first; itr != range.second; ++itr)
        {
            itr->second->turn_offset = edge.data.turn_id;
            indexed_restrictions.push_back(*itr->second);
        }
    }

    return indexed_restrictions;
}

} // namespace osrm::extractor
