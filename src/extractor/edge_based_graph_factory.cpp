#include "extractor/edge_based_graph_factory.hpp"
#include "extractor/edge_based_edge.hpp"
#include "extractor/files.hpp"
#include "extractor/guidance/turn_analysis.hpp"
#include "extractor/guidance/turn_lane_handler.hpp"
#include "extractor/scripting_environment.hpp"
#include "extractor/suffix_table.hpp"

#include "storage/io.hpp"

#include "util/assert.hpp"
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

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/pipeline.h>
#include <tbb/task_scheduler_init.h>

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
    const extractor::PackedOSMIDs &osm_node_ids,
    ProfileProperties profile_properties,
    const util::NameTable &name_table,
    guidance::LaneDescriptionMap &lane_description_map)
    : m_max_edge_id(0), m_coordinates(coordinates), m_osm_node_ids(osm_node_ids),
      m_node_based_graph(std::move(node_based_graph)),
      m_restriction_map(std::move(restriction_map)), m_barrier_nodes(barrier_nodes),
      m_traffic_lights(traffic_lights), m_compressed_edge_container(compressed_edge_container),
      profile_properties(std::move(profile_properties)), name_table(name_table),
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

void EdgeBasedGraphFactory::GetEdgeBasedNodes(EdgeBasedNodeDataContainer &data_container)
{
    using std::swap; // Koenig swap
    swap(data_container, m_edge_based_node_container);
}

void EdgeBasedGraphFactory::GetEdgeBasedNodeSegments(std::vector<EdgeBasedNodeSegment> &nodes)
{
    using std::swap; // Koenig swap
    swap(nodes, m_edge_based_node_segments);
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

    // Add edge-based node data for forward and reverse nodes indexed by edge_id
    BOOST_ASSERT(forward_data.edge_id != SPECIAL_EDGEID);
    m_edge_based_node_container.SetData(forward_data.edge_id,
                                        GeometryID{packed_geometry_id, true},
                                        forward_data.name_id,
                                        forward_data.travel_mode,
                                        forward_data.classes);
    if (reverse_data.edge_id != SPECIAL_EDGEID)
    {
        m_edge_based_node_container.SetData(reverse_data.edge_id,
                                            GeometryID{packed_geometry_id, false},
                                            reverse_data.name_id,
                                            reverse_data.travel_mode,
                                            reverse_data.classes);
    }

    // Add segments of edge-based nodes
    for (const auto i : util::irange(std::size_t{0}, segment_count))
    {
        BOOST_ASSERT(
            current_edge_source_coordinate_id ==
            m_compressed_edge_container.GetBucketReference(edge_id_2)[segment_count - 1 - i]
                .node_id);
        const NodeID current_edge_target_coordinate_id = forward_geometry[i].node_id;
        BOOST_ASSERT(current_edge_target_coordinate_id != current_edge_source_coordinate_id);

        // build edges
        m_edge_based_node_segments.emplace_back(edge_id_to_segment_id(forward_data.edge_id),
                                                edge_id_to_segment_id(reverse_data.edge_id),
                                                current_edge_source_coordinate_id,
                                                current_edge_target_coordinate_id,
                                                i);

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
    // heuristic: node-based graph node is a simple intersection with four edges (edge-based nodes)
    m_edge_based_node_weights.reserve(4 * m_node_based_graph->GetNumberOfNodes());

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

    // Allocate memory for edge-based nodes
    m_edge_based_node_container = EdgeBasedNodeDataContainer(m_max_edge_id + 1);

    util::Log() << "Generating edge expanded nodes ... ";
    {
        util::UnbufferedLog log;
        util::Percent progress(log, m_node_based_graph->GetNumberOfNodes());

        m_compressed_edge_container.InitializeBothwayVector();

        // loop over all edges and generate new set of nodes
        for (const auto nbg_node_u : util::irange(0u, m_node_based_graph->GetNumberOfNodes()))
        {
            BOOST_ASSERT(nbg_node_u != SPECIAL_NODEID);
            progress.PrintStatus(nbg_node_u);
            for (EdgeID nbg_edge_id : m_node_based_graph->GetAdjacentEdgeRange(nbg_node_u))
            {
                BOOST_ASSERT(nbg_edge_id != SPECIAL_EDGEID);

                const EdgeData &nbg_edge_data = m_node_based_graph->GetEdgeData(nbg_edge_id);
                const NodeID nbg_node_v = m_node_based_graph->GetTarget(nbg_edge_id);
                BOOST_ASSERT(nbg_node_v != SPECIAL_NODEID);
                BOOST_ASSERT(nbg_node_u != nbg_node_v);

                // pick only every other edge, since we have every edge as an outgoing
                // and incoming egde
                if (nbg_node_u >= nbg_node_v)
                {
                    continue;
                }

                // if we found a non-forward edge reverse and try again
                if (nbg_edge_data.edge_id == SPECIAL_NODEID)
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

    BOOST_ASSERT(m_edge_based_node_segments.size() == m_edge_based_node_is_startpoint.size());
    BOOST_ASSERT(m_max_edge_id + 1 == m_edge_based_node_weights.size());

    util::Log() << "Generated " << (m_max_edge_id + 1) << " nodes and "
                << m_edge_based_node_segments.size() << " segments in edge-expanded graph";

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
    guidance::lanes::TurnLaneHandler turn_lane_handler(
        *m_node_based_graph, lane_description_map, turn_analysis, lane_data_map);

    bearing_class_by_node_based_node.resize(m_node_based_graph->GetNumberOfNodes(),
                                            std::numeric_limits<std::uint32_t>::max());

    // FIXME these need to be tuned in pre-allocated size
    std::vector<TurnPenalty> turn_weight_penalties;
    std::vector<TurnPenalty> turn_duration_penalties;

    const auto weight_multiplier =
        scripting_environment.GetProfileProperties().GetWeightMultiplier();

    // The following block generates the edge-based-edges using a parallel processing
    // pipeline.  Sets of intersection IDs are batched in groups of GRAINSIZE (100)
    // `generator_stage`,
    // then those groups are processed in parallel `processor_stage`.  Finally, results are
    // appended to the various buffer vectors by the `output_stage` in the same order
    // that the `generator_stage` created them in (tbb::filter::serial_in_order creates this
    // guarantee).  The order needs to be maintained because we depend on it later in the
    // processing pipeline.
    {
        util::UnbufferedLog log;

        const NodeID node_count = m_node_based_graph->GetNumberOfNodes();
        util::Percent progress(log, node_count);
        // This counter is used to keep track of how far along we've made it
        std::uint64_t nodes_completed = 0;

        // going over all nodes (which form the center of an intersection), we compute all
        // possible turns along these intersections.

        NodeID current_node = 0;

        // Handle intersections in sets of 100.  The pipeline below has a serial bottleneck
        // during the writing phase, so we want to make the parallel workers do more work
        // to give the serial final stage time to complete its tasks.
        const constexpr unsigned GRAINSIZE = 100;

        // First part of the pipeline generates iterator ranges of IDs in sets of GRAINSIZE
        tbb::filter_t<void, tbb::blocked_range<NodeID>> generator_stage(
            tbb::filter::serial_in_order, [&](tbb::flow_control &fc) -> tbb::blocked_range<NodeID> {
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

        // This struct is the buffered output of the `processor_stage`.  This data is
        // appended to the various output arrays/files by the `output_stage`.
        struct IntersectionData
        {
            std::size_t nodes_processed = 0;
            std::vector<lookup::TurnIndexBlock> turn_indexes;
            std::vector<EdgeBasedEdge> edges_list;
            std::vector<TurnPenalty> turn_weight_penalties;
            std::vector<TurnPenalty> turn_duration_penalties;
            std::vector<TurnData> turn_data_container;
        };

        // Second part of the pipeline is where the intersection analysis is done for
        // each intersection
        tbb::filter_t<tbb::blocked_range<NodeID>, std::shared_ptr<IntersectionData>>
        processor_stage(
            tbb::filter::parallel, [&](const tbb::blocked_range<NodeID> &intersection_node_range) {

                auto buffer = std::make_shared<IntersectionData>();
                buffer->nodes_processed =
                    intersection_node_range.end() - intersection_node_range.begin();

                // If we get fed a 0-length range for some reason, we can just return right away
                if (buffer->nodes_processed == 0)
                    return buffer;

                for (auto node_at_center_of_intersection = intersection_node_range.begin(),
                          end = intersection_node_range.end();
                     node_at_center_of_intersection < end;
                     ++node_at_center_of_intersection)
                {

                    // We capture the thread-local work in these objects, then flush
                    // them in a controlled manner at the end of the parallel range

                    const auto shape_result =
                        turn_analysis.ComputeIntersectionShapes(node_at_center_of_intersection);

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
                    // From the flags alone, we cannot determine which nodes are connected to
                    // `b` by an outgoing edge. Therefore, we have to search all connected edges for
                    // edges entering `b`
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
                            turn_analysis.GetIntersectionGenerator()
                                .TransformIntersectionShapeIntoView(
                                    node_along_road_entering,
                                    incoming_edge,
                                    shape_result.annotated_normalized_shape.normalized_shape,
                                    shape_result.intersection_shape,
                                    shape_result.annotated_normalized_shape.performed_merges);

                        auto intersection =
                            turn_analysis.AssignTurnTypes(node_along_road_entering,
                                                          incoming_edge,
                                                          intersection_with_flags_and_angles);

                        OSRM_ASSERT(intersection.valid(),
                                    m_coordinates[node_at_center_of_intersection]);

                        intersection = turn_lane_handler.assignTurnLanes(
                            node_along_road_entering, incoming_edge, std::move(intersection));

                        // the entry class depends on the turn, so we have to classify the
                        // interesction for
                        // every edge
                        const auto turn_classification = classifyIntersection(
                            intersection, m_coordinates[node_at_center_of_intersection]);

                        const auto entry_class_id =
                            entry_class_hash.ConcurrentFindOrAdd(turn_classification.first);

                        const auto bearing_class_id =
                            bearing_class_hash.ConcurrentFindOrAdd(turn_classification.second);

                        // Note - this is strictly speaking not thread safe, but we know we
                        // should never be touching the same element twice, so we should
                        // be fine.
                        bearing_class_by_node_based_node[node_at_center_of_intersection] =
                            bearing_class_id;

                        for (const auto &turn : intersection)
                        {
                            // only keep valid turns
                            if (!turn.entry_allowed)
                                continue;

                            // only add an edge if turn is not prohibited
                            const EdgeData &edge_data1 =
                                m_node_based_graph->GetEdgeData(incoming_edge);
                            const EdgeData &edge_data2 = m_node_based_graph->GetEdgeData(turn.eid);

                            BOOST_ASSERT(edge_data1.edge_id != edge_data2.edge_id);
                            BOOST_ASSERT(!edge_data1.reversed);
                            BOOST_ASSERT(!edge_data2.reversed);

                            // the following is the core of the loop.
                            buffer->turn_data_container.push_back(
                                {turn.instruction,
                                 turn.lane_data_id,
                                 entry_class_id,
                                 util::guidance::TurnBearing(intersection[0].bearing),
                                 util::guidance::TurnBearing(turn.bearing)});

                            // compute weight and duration penalties
                            auto is_traffic_light =
                                m_traffic_lights.count(node_at_center_of_intersection);
                            ExtractionTurn extracted_turn(turn, is_traffic_light);
                            extracted_turn.source_restricted = edge_data1.restricted;
                            extracted_turn.target_restricted = edge_data2.restricted;
                            scripting_environment.ProcessTurn(extracted_turn);

                            // turn penalties are limited to [-2^15, 2^15) which roughly
                            // translates to 54 minutes and fits signed 16bit deci-seconds
                            auto weight_penalty = boost::numeric_cast<TurnPenalty>(
                                extracted_turn.weight * weight_multiplier);
                            auto duration_penalty =
                                boost::numeric_cast<TurnPenalty>(extracted_turn.duration * 10.);

                            BOOST_ASSERT(SPECIAL_NODEID != edge_data1.edge_id);
                            BOOST_ASSERT(SPECIAL_NODEID != edge_data2.edge_id);

                            // auto turn_id = m_edge_based_edge_list.size();
                            auto weight =
                                boost::numeric_cast<EdgeWeight>(edge_data1.weight + weight_penalty);
                            auto duration = boost::numeric_cast<EdgeWeight>(edge_data1.duration +
                                                                            duration_penalty);
                            buffer->edges_list.emplace_back(
                                edge_data1.edge_id,
                                edge_data2.edge_id,
                                SPECIAL_NODEID, // This will be updated once the main loop
                                                // completes!
                                weight,
                                duration,
                                true,
                                false);

                            BOOST_ASSERT(buffer->turn_weight_penalties.size() ==
                                         buffer->edges_list.size() - 1);
                            buffer->turn_weight_penalties.push_back(weight_penalty);
                            BOOST_ASSERT(buffer->turn_duration_penalties.size() ==
                                         buffer->edges_list.size() - 1);
                            buffer->turn_duration_penalties.push_back(duration_penalty);

                            // We write out the mapping between the edge-expanded edges and the
                            // original nodes. Since each edge represents a possible maneuver,
                            // external programs can use this to quickly perform updates to edge
                            // weights in order to penalize certain turns.

                            // If this edge is 'trivial' -- where the compressed edge corresponds
                            // exactly to an original OSM segment -- we can pull the turn's
                            // preceding node ID directly with `node_along_road_entering`;
                            // otherwise, we need to look up the node immediately preceding the turn
                            // from the compressed edge container.
                            const bool isTrivial =
                                m_compressed_edge_container.IsTrivial(incoming_edge);

                            const auto &from_node =
                                isTrivial ? node_along_road_entering
                                          : m_compressed_edge_container.GetLastEdgeSourceID(
                                                incoming_edge);
                            const auto &via_node =
                                m_compressed_edge_container.GetLastEdgeTargetID(incoming_edge);
                            const auto &to_node =
                                m_compressed_edge_container.GetFirstEdgeTargetID(turn.eid);

                            buffer->turn_indexes.push_back({from_node, via_node, to_node});
                        }
                    }
                }

                return buffer;
            });

        // Because we write TurnIndexBlock data as we go, we'll
        // buffer them into groups of 1000 to reduce the syscall
        // count by 1000x.  This doesn't need much memory, but
        // greatly reduces the syscall overhead of writing lots
        // of small objects
        const constexpr int TURN_INDEX_WRITE_BUFFER_SIZE = 1000;
        std::vector<lookup::TurnIndexBlock> turn_indexes_write_buffer;
        turn_indexes_write_buffer.reserve(TURN_INDEX_WRITE_BUFFER_SIZE);

        // Last part of the pipeline puts all the calculated data into the serial buffers
        tbb::filter_t<std::shared_ptr<IntersectionData>, void> output_stage(
            tbb::filter::serial_in_order, [&](const std::shared_ptr<IntersectionData> buffer) {

                nodes_completed += buffer->nodes_processed;
                progress.PrintStatus(nodes_completed);

                // NOTE: potential overflow here if we hit 2^32 routable edges
                m_edge_based_edge_list.append(buffer->edges_list.begin(), buffer->edges_list.end());
                BOOST_ASSERT(m_edge_based_edge_list.size() <= std::numeric_limits<NodeID>::max());

                turn_weight_penalties.insert(turn_weight_penalties.end(),
                                             buffer->turn_weight_penalties.begin(),
                                             buffer->turn_weight_penalties.end());
                turn_duration_penalties.insert(turn_duration_penalties.end(),
                                               buffer->turn_duration_penalties.begin(),
                                               buffer->turn_duration_penalties.end());
                turn_data_container.append(buffer->turn_data_container);

                turn_indexes_write_buffer.insert(turn_indexes_write_buffer.end(),
                                                 buffer->turn_indexes.begin(),
                                                 buffer->turn_indexes.end());

                // Buffer writes to reduce syscall count
                if (turn_indexes_write_buffer.size() >= TURN_INDEX_WRITE_BUFFER_SIZE)
                {
                    turn_penalties_index_file.WriteFrom(turn_indexes_write_buffer.data(),
                                                        turn_indexes_write_buffer.size());
                    turn_indexes_write_buffer.clear();
                }
            });

        // Now, execute the pipeline.  The value of "5" here was chosen by experimentation
        // on a 16-CPU machine and seemed to give the best performance.  This value needs
        // to be balanced with the GRAINSIZE above - ideally, the pipeline puts as much work
        // as possible in the `intersection_handler` step so that those parallel workers don't
        // get blocked too much by the slower (io-performing) `buffer_storage`
        tbb::parallel_pipeline(tbb::task_scheduler_init::default_num_threads() * 5,
                               generator_stage & processor_stage & output_stage);

        // Flush the turn_indexes_write_buffer if it's not empty
        if (!turn_indexes_write_buffer.empty())
        {
            turn_penalties_index_file.WriteFrom(turn_indexes_write_buffer.data(),
                                                turn_indexes_write_buffer.size());
            turn_indexes_write_buffer.clear();
        }
    }

    util::Log() << "Reunmbering turns";
    // Now, update the turn_id property on every EdgeBasedEdge - it will equal the
    // position in the m_edge_based_edge_list array for each object.
    tbb::parallel_for(tbb::blocked_range<NodeID>(0, m_edge_based_edge_list.size()),
                      [this](const tbb::blocked_range<NodeID> &range) {
                          for (auto x = range.begin(), end = range.end(); x != end; ++x)
                          {
                              m_edge_based_edge_list[x].data.turn_id = x;
                          }
                      });

    // write weight penalties per turn
    BOOST_ASSERT(turn_weight_penalties.size() == turn_duration_penalties.size());
    {
        storage::io::FileWriter writer(turn_weight_penalties_filename,
                                       storage::io::FileWriter::GenerateFingerprint);
        storage::serialization::write(writer, turn_weight_penalties);
    }

    {
        storage::io::FileWriter writer(turn_duration_penalties_filename,
                                       storage::io::FileWriter::GenerateFingerprint);
        storage::serialization::write(writer, turn_duration_penalties);
    }

    util::Log() << "Created " << entry_class_hash.data.size() << " entry classes and "
                << bearing_class_hash.data.size() << " Bearing Classes";

    util::Log() << "Writing Turn Lane Data to File...";
    {
        storage::io::FileWriter writer(turn_lane_data_filename,
                                       storage::io::FileWriter::GenerateFingerprint);

        std::vector<util::guidance::LaneTupleIdPair> lane_data(lane_data_map.data.size());
        // extract lane data sorted by ID
        for (auto itr : lane_data_map.data)
            lane_data[itr.second] = itr.first;

        storage::serialization::write(writer, lane_data);
    }
    util::Log() << "done.";

    files::writeTurnData(turn_data_filename, turn_data_container);

    util::Log() << "Generated " << m_edge_based_node_segments.size() << " edge based node segments";
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
    std::vector<util::guidance::BearingClass> result(bearing_class_hash.data.size());
    for (const auto &pair : bearing_class_hash.data)
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
    std::vector<util::guidance::EntryClass> result(entry_class_hash.data.size());
    for (const auto &pair : entry_class_hash.data)
    {
        BOOST_ASSERT(pair.second < result.size());
        result[pair.second] = pair.first;
    }
    return result;
}

} // namespace extractor
} // namespace osrm
