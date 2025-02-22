#include "guidance/guidance_processing.hpp"
#include "guidance/turn_analysis.hpp"
#include "guidance/turn_lane_handler.hpp"

#include "extractor/intersection/intersection_analysis.hpp"

#include "util/assert.hpp"
#include "util/connectivity_checksum.hpp"
#include "util/percent.hpp"

#include <tbb/blocked_range.h>
#include <tbb/parallel_pipeline.h>

#include <thread>

namespace osrm::guidance
{

void annotateTurns(const util::NodeBasedDynamicGraph &node_based_graph,
                   const extractor::EdgeBasedNodeDataContainer &edge_based_node_container,
                   const std::vector<util::Coordinate> &node_coordinates,
                   const extractor::CompressedEdgeContainer &compressed_edge_container,
                   const std::unordered_set<NodeID> &barrier_nodes,
                   const extractor::RestrictionMap &node_restriction_map,
                   const extractor::WayRestrictionMap &way_restriction_map,
                   const extractor::NameTable &name_table,
                   const extractor::SuffixTable &suffix_table,
                   const extractor::TurnLanesIndexedArray &turn_lanes_data,
                   extractor::LaneDescriptionMap &lane_description_map,
                   util::guidance::LaneDataIdMap &lane_data_map,
                   guidance::TurnDataExternalContainer &turn_data_container,
                   BearingClassesVector &bearing_class_by_node_based_node,
                   BearingClassesMap &bearing_class_hash,
                   EntryClassesMap &entry_class_hash,
                   std::uint32_t &connectivity_checksum)
{
    util::Log() << "Generating guidance turns ";

    extractor::intersection::MergableRoadDetector mergable_road_detector(node_based_graph,
                                                                         edge_based_node_container,
                                                                         node_coordinates,
                                                                         compressed_edge_container,
                                                                         node_restriction_map,
                                                                         barrier_nodes,
                                                                         turn_lanes_data,
                                                                         name_table,
                                                                         suffix_table);

    guidance::TurnAnalysis turn_analysis(node_based_graph,
                                         edge_based_node_container,
                                         node_coordinates,
                                         compressed_edge_container,
                                         node_restriction_map,
                                         barrier_nodes,
                                         turn_lanes_data,
                                         name_table,
                                         suffix_table);

    guidance::lanes::TurnLaneHandler turn_lane_handler(node_based_graph,
                                                       edge_based_node_container,
                                                       node_coordinates,
                                                       compressed_edge_container,
                                                       node_restriction_map,
                                                       barrier_nodes,
                                                       turn_lanes_data,
                                                       lane_description_map,
                                                       turn_analysis,
                                                       lane_data_map);

    bearing_class_by_node_based_node.resize(node_based_graph.GetNumberOfNodes(),
                                            std::numeric_limits<std::uint32_t>::max());

    struct TurnsPipelineBuffer
    {
        std::size_t nodes_processed = 0;

        std::vector<guidance::TurnData> continuous_turn_data; // populate answers from guidance
        std::vector<guidance::TurnData> delayed_turn_data;    // populate answers from guidance

        util::ConnectivityChecksum checksum;
    };
    using TurnsPipelineBufferPtr = std::shared_ptr<TurnsPipelineBuffer>;

    // going over all nodes (which form the center of an intersection), we compute all
    // possible turns along these intersections.
    {
        const NodeID node_count = node_based_graph.GetNumberOfNodes();
        NodeID current_node = 0;

        connectivity_checksum = 0;

        // Handle intersections in sets of 100.  The pipeline below has a serial bottleneck
        // during the writing phase, so we want to make the parallel workers do more work
        // to give the serial final stage time to complete its tasks.
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

        //
        // Guidance stage
        //
        tbb::filter<tbb::blocked_range<NodeID>, TurnsPipelineBufferPtr> guidance_stage(
            tbb::filter_mode::parallel,
            [&](const tbb::blocked_range<NodeID> &intersection_node_range)
            {
                auto buffer = std::make_shared<TurnsPipelineBuffer>();
                buffer->nodes_processed = intersection_node_range.size();

                for (auto intersection_node = intersection_node_range.begin(),
                          end = intersection_node_range.end();
                     intersection_node < end;
                     ++intersection_node)
                {
                    // We capture the thread-local work in these objects, then flush
                    // them in a controlled manner at the end of the parallel range
                    const auto &incoming_edges = extractor::intersection::getIncomingEdges(
                        node_based_graph, intersection_node);
                    const auto &outgoing_edges = extractor::intersection::getOutgoingEdges(
                        node_based_graph, intersection_node);
                    const auto &edge_geometries_and_merged_edges =
                        extractor::intersection::getIntersectionGeometries(
                            node_based_graph,
                            compressed_edge_container,
                            node_coordinates,
                            mergable_road_detector,
                            intersection_node);
                    const auto &edge_geometries = edge_geometries_and_merged_edges.first;
                    const auto &merged_edge_ids = edge_geometries_and_merged_edges.second;

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
                    // From the flags alone, we cannot determine which nodes are connected to
                    // `b` by an outgoing edge. Therefore, we have to search all connected edges for
                    // edges entering `b`

                    for (const auto &incoming_edge : incoming_edges)
                    {
                        const auto intersection_view =
                            extractor::intersection::convertToIntersectionView(
                                node_based_graph,
                                edge_based_node_container,
                                node_restriction_map,
                                barrier_nodes,
                                edge_geometries,
                                turn_lanes_data,
                                incoming_edge,
                                outgoing_edges,
                                merged_edge_ids);

                        auto intersection = turn_analysis.AssignTurnTypes(
                            incoming_edge.node, incoming_edge.edge, intersection_view);

                        OSRM_ASSERT(intersection.valid(), node_coordinates[intersection_node]);
                        intersection = turn_lane_handler.assignTurnLanes(
                            incoming_edge.node, incoming_edge.edge, std::move(intersection));

                        // the entry class depends on the turn, so we have to classify the
                        // interesction for every edge
                        const auto turn_classification =
                            classifyIntersection(intersection, node_coordinates[intersection_node]);

                        const auto entry_class_id =
                            entry_class_hash.ConcurrentFindOrAdd(turn_classification.first);

                        const auto bearing_class_id =
                            bearing_class_hash.ConcurrentFindOrAdd(turn_classification.second);

                        // Note - this is strictly speaking not thread safe, but we know we
                        // should never be touching the same element twice, so we should
                        // be fine.
                        bearing_class_by_node_based_node[intersection_node] = bearing_class_id;

                        // check if we on a restriction via edge
                        const auto is_restriction_via_edge =
                            way_restriction_map.IsViaWayEdge(incoming_edge.node, intersection_node);

                        for (const auto &outgoing_edge : outgoing_edges)
                        {
                            auto is_turn_allowed =
                                extractor::intersection::isTurnAllowed(node_based_graph,
                                                                       edge_based_node_container,
                                                                       node_restriction_map,
                                                                       barrier_nodes,
                                                                       edge_geometries,
                                                                       turn_lanes_data,
                                                                       incoming_edge,
                                                                       outgoing_edge);

                            buffer->checksum.process_bit(is_turn_allowed);

                            if (!is_turn_allowed)
                                continue;

                            const auto turn =
                                std::find_if(intersection.begin(),
                                             intersection.end(),
                                             [edge = outgoing_edge.edge](const auto &road)
                                             { return road.eid == edge; });

                            OSRM_ASSERT(turn != intersection.end(),
                                        node_coordinates[intersection_node]);

                            buffer->continuous_turn_data.push_back(guidance::TurnData{
                                turn->instruction,
                                turn->lane_data_id,
                                entry_class_id,
                                guidance::TurnBearing(intersection[0].perceived_bearing),
                                guidance::TurnBearing(turn->perceived_bearing)});

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
                                    auto const node_at_end_of_turn =
                                        node_based_graph.GetTarget(outgoing_edge.edge);

                                    const auto is_way_restricted = way_restriction_map.IsRestricted(
                                        duplicated_node_id, node_at_end_of_turn);

                                    if (is_way_restricted)
                                    {
                                        auto const restrictions =
                                            way_restriction_map.GetRestrictions(
                                                duplicated_node_id, node_at_end_of_turn);

                                        auto has_unconditional =
                                            std::any_of(restrictions.begin(),
                                                        restrictions.end(),
                                                        [](const auto &restriction)
                                                        { return restriction->IsUnconditional(); });

                                        if (has_unconditional)
                                            continue;

                                        buffer->delayed_turn_data.push_back(guidance::TurnData{
                                            turn->instruction,
                                            turn->lane_data_id,
                                            entry_class_id,
                                            guidance::TurnBearing(
                                                intersection[0].perceived_bearing),
                                            guidance::TurnBearing(turn->perceived_bearing)});
                                    }
                                    else
                                    {
                                        buffer->delayed_turn_data.push_back(guidance::TurnData{
                                            turn->instruction,
                                            turn->lane_data_id,
                                            entry_class_id,
                                            guidance::TurnBearing(
                                                intersection[0].perceived_bearing),
                                            guidance::TurnBearing(turn->perceived_bearing)});
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
        util::Percent guidance_progress(log, node_count);
        std::vector<guidance::TurnData> delayed_turn_data;

        tbb::filter<TurnsPipelineBufferPtr, void> guidance_output_stage(
            tbb::filter_mode::serial_in_order,
            [&](auto buffer)
            {
                guidance_progress.PrintAddition(buffer->nodes_processed);

                connectivity_checksum = buffer->checksum.update_checksum(connectivity_checksum);

                // Guidance data
                std::for_each(buffer->continuous_turn_data.begin(),
                              buffer->continuous_turn_data.end(),
                              [&turn_data_container](const auto &turn_data)
                              { turn_data_container.push_back(turn_data); });

                // Copy via-way restrictions delayed data
                delayed_turn_data.insert(delayed_turn_data.end(),
                                         buffer->delayed_turn_data.begin(),
                                         buffer->delayed_turn_data.end());
            });

        // Now, execute the pipeline.  The value of "5" here was chosen by experimentation
        // on a 16-CPU machine and seemed to give the best performance.  This value needs
        // to be balanced with the GRAINSIZE above - ideally, the pipeline puts as much work
        // as possible in the `intersection_handler` step so that those parallel workers don't
        // get blocked too much by the slower (io-performing) `buffer_storage`
        tbb::parallel_pipeline(std::thread::hardware_concurrency() * 5,
                               generator_stage & guidance_stage & guidance_output_stage);

        // NOTE: EBG edges delayed_data and turns delayed_turn_data have the same index
        std::for_each(delayed_turn_data.begin(),
                      delayed_turn_data.end(),
                      [&turn_data_container](const auto &turn_data)
                      { turn_data_container.push_back(turn_data); });
    }

    util::Log() << "done.";

    util::Log() << "Created " << entry_class_hash.data.size() << " entry classes and "
                << bearing_class_hash.data.size() << " Bearing Classes";
}

} // namespace osrm::guidance
