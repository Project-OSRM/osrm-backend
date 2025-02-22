#include "extractor/graph_compressor.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/extraction_turn.hpp"
#include "extractor/restriction.hpp"
#include "extractor/turn_path_compressor.hpp"

#include "util/dynamic_graph.hpp"
#include "util/node_based_graph.hpp"
#include "util/percent.hpp"

#include "util/log.hpp"

#include <boost/assert.hpp>
#include <unordered_set>

namespace osrm::extractor
{

static constexpr int SECOND_TO_DECISECOND = 10;

void GraphCompressor::Compress(const std::unordered_set<NodeID> &barrier_nodes,
                               TrafficSignals &traffic_signals,
                               ScriptingEnvironment &scripting_environment,
                               std::vector<TurnRestriction> &turn_restrictions,
                               std::vector<UnresolvedManeuverOverride> &maneuver_overrides,
                               util::NodeBasedDynamicGraph &graph,
                               const std::vector<NodeBasedEdgeAnnotation> &node_data_container,
                               CompressedEdgeContainer &geometry_compressor)
{
    const unsigned original_number_of_nodes = graph.GetNumberOfNodes();
    const unsigned original_number_of_edges = graph.GetNumberOfEdges();

    TurnPathCompressor turn_path_compressor(turn_restrictions, maneuver_overrides);

    // Some degree two nodes are not compressed if they act as entry/exit points into a
    // restriction path.
    std::unordered_set<NodeID> incompressible_via_nodes;

    const auto remember_via_nodes = [&](const auto &restriction)
    {
        if (restriction.turn_path.Type() == TurnPathType::VIA_NODE_TURN_PATH)
        {
            incompressible_via_nodes.insert(restriction.turn_path.AsViaNodePath().via);
        }
        else
        {
            BOOST_ASSERT(restriction.turn_path.Type() == TurnPathType::VIA_WAY_TURN_PATH);
            const auto &way_restriction = restriction.turn_path.AsViaWayPath();
            // We do not compress the first and last via nodes so that we know how to enter/exit
            // a restriction path and apply the restrictions correctly.
            incompressible_via_nodes.insert(way_restriction.via.front());
            incompressible_via_nodes.insert(way_restriction.via.back());
        }
    };
    std::for_each(turn_restrictions.begin(), turn_restrictions.end(), remember_via_nodes);
    for (const auto &maneuver : maneuver_overrides)
    {
        // Only incompressible is where the instruction occurs.
        incompressible_via_nodes.insert(maneuver.instruction_node);
    }

    {
        const auto weight_multiplier =
            scripting_environment.GetProfileProperties().GetWeightMultiplier();
        util::UnbufferedLog log;
        util::Percent progress(log, original_number_of_nodes);

        for (const NodeID node_v : util::irange(0u, original_number_of_nodes))
        {
            progress.PrintStatus(node_v);

            // only contract degree 2 vertices
            if (2 != graph.GetOutDegree(node_v))
            {
                continue;
            }

            // don't contract barrier node
            if (barrier_nodes.end() != barrier_nodes.find(node_v))
            {
                continue;
            }

            // check if v is an entry/exit via node for a turn restriction
            if (incompressible_via_nodes.contains(node_v))
            {
                continue;
            }

            //    reverse_e2   forward_e2
            // u <---------- v -----------> w
            //    ----------> <-----------
            //    forward_e1   reverse_e1
            //
            // Will be compressed to:
            //
            //    reverse_e1
            // u <---------- w
            //    ---------->
            //    forward_e1
            //
            // If the edges are compatible.
            const bool reverse_edge_order = graph.GetEdgeData(graph.BeginEdges(node_v)).reversed;
            const EdgeID forward_e2 = graph.BeginEdges(node_v) + reverse_edge_order;
            BOOST_ASSERT(SPECIAL_EDGEID != forward_e2);
            BOOST_ASSERT(forward_e2 >= graph.BeginEdges(node_v) &&
                         forward_e2 < graph.EndEdges(node_v));
            const EdgeID reverse_e2 = graph.BeginEdges(node_v) + 1 - reverse_edge_order;

            BOOST_ASSERT(SPECIAL_EDGEID != reverse_e2);
            BOOST_ASSERT(reverse_e2 >= graph.BeginEdges(node_v) &&
                         reverse_e2 < graph.EndEdges(node_v));

            const EdgeData &fwd_edge_data2 = graph.GetEdgeData(forward_e2);
            const EdgeData &rev_edge_data2 = graph.GetEdgeData(reverse_e2);

            const NodeID node_w = graph.GetTarget(forward_e2);
            BOOST_ASSERT(SPECIAL_NODEID != node_w);
            BOOST_ASSERT(node_v != node_w);
            const NodeID node_u = graph.GetTarget(reverse_e2);
            BOOST_ASSERT(SPECIAL_NODEID != node_u);
            BOOST_ASSERT(node_u != node_v);

            const EdgeID forward_e1 = graph.FindEdge(node_u, node_v);
            BOOST_ASSERT(SPECIAL_EDGEID != forward_e1);
            BOOST_ASSERT(node_v == graph.GetTarget(forward_e1));
            const EdgeID reverse_e1 = graph.FindEdge(node_w, node_v);
            BOOST_ASSERT(SPECIAL_EDGEID != reverse_e1);
            BOOST_ASSERT(node_v == graph.GetTarget(reverse_e1));

            const EdgeData &fwd_edge_data1 = graph.GetEdgeData(forward_e1);
            const EdgeData &rev_edge_data1 = graph.GetEdgeData(reverse_e1);
            const auto fwd_annotation_data1 = node_data_container[fwd_edge_data1.annotation_data];
            const auto fwd_annotation_data2 = node_data_container[fwd_edge_data2.annotation_data];
            const auto rev_annotation_data1 = node_data_container[rev_edge_data1.annotation_data];
            const auto rev_annotation_data2 = node_data_container[rev_edge_data2.annotation_data];

            if (graph.FindEdgeInEitherDirection(node_u, node_w) != SPECIAL_EDGEID)
            {
                continue;
            }

            // this case can happen if two ways with different names overlap
            if ((fwd_annotation_data1.name_id != rev_annotation_data1.name_id) ||
                (fwd_annotation_data2.name_id != rev_annotation_data2.name_id))
            {
                continue;
            }

            if ((fwd_edge_data1.flags == fwd_edge_data2.flags) &&
                (rev_edge_data1.flags == rev_edge_data2.flags) &&
                (fwd_edge_data1.reversed == fwd_edge_data2.reversed) &&
                (rev_edge_data1.reversed == rev_edge_data2.reversed) &&
                // annotations need to match, except for the lane-id which can differ
                fwd_annotation_data1.CanCombineWith(fwd_annotation_data2) &&
                rev_annotation_data1.CanCombineWith(rev_annotation_data2))
            {
                BOOST_ASSERT(!(graph.GetEdgeData(forward_e1).reversed &&
                               graph.GetEdgeData(reverse_e1).reversed));
                /*
                 * Remember Lane Data for compressed parts. This handles scenarios where lane-data
                 * is
                 * only kept up until a traffic light.
                 *
                 *                |    |
                 * ----------------    |
                 *         -^ |        |
                 * -----------         |
                 *         -v |        |
                 * ---------------     |
                 *                |    |
                 *
                 *  u ------- v ---- w
                 *
                 * Since the edge is compressable, we can transfer:
                 * "left|right" (uv) and "" (uw) into a string with "left|right" (uw) for the
                 * compressed
                 * edge.
                 * Doing so, we might mess up the point from where the lanes are shown. It should be
                 * reasonable, since the announcements have to come early anyhow. So there is a
                 * potential danger in here, but it saves us from adding a lot of additional edges
                 * for
                 * turn-lanes. Without this, we would have to treat any turn-lane beginning/ending
                 * just
                 * like a barrier.
                 */
                const auto selectAnnotation =
                    [&node_data_container](const AnnotationID front_annotation,
                                           const AnnotationID back_annotation)
                {
                    // A lane has tags: u - (front) - v - (back) - w
                    // During contraction, we keep only one of the tags. Usually the one closer
                    // to the intersection is preferred. If its empty, however, we keep the
                    // non-empty one
                    if (node_data_container[back_annotation].lane_description_id ==
                        INVALID_LANE_DESCRIPTIONID)
                        return front_annotation;
                    return back_annotation;
                };

                graph.GetEdgeData(forward_e1).annotation_data = selectAnnotation(
                    fwd_edge_data1.annotation_data, fwd_edge_data2.annotation_data);
                graph.GetEdgeData(reverse_e1).annotation_data = selectAnnotation(
                    rev_edge_data1.annotation_data, rev_edge_data2.annotation_data);
                graph.GetEdgeData(forward_e2).annotation_data = selectAnnotation(
                    fwd_edge_data2.annotation_data, fwd_edge_data1.annotation_data);
                graph.GetEdgeData(reverse_e2).annotation_data = selectAnnotation(
                    rev_edge_data2.annotation_data, rev_edge_data1.annotation_data);

                // Add node penalty when compress edge crosses a traffic signal
                const bool has_forward_signal = traffic_signals.HasSignal(node_u, node_v);
                const bool has_reverse_signal = traffic_signals.HasSignal(node_w, node_v);

                EdgeDuration forward_node_duration_penalty = MAXIMAL_EDGE_DURATION;
                EdgeWeight forward_node_weight_penalty = INVALID_EDGE_WEIGHT;
                EdgeDuration reverse_node_duration_penalty = MAXIMAL_EDGE_DURATION;
                EdgeWeight reverse_node_weight_penalty = INVALID_EDGE_WEIGHT;
                if (has_forward_signal || has_reverse_signal)
                {
                    // we cannot handle this as node penalty, if it depends on turn direction
                    if (fwd_edge_data1.flags.restricted != fwd_edge_data2.flags.restricted)
                        continue;

                    // generate an artificial turn for the turn penalty generation
                    std::vector<ExtractionTurnLeg> roads_on_the_right;
                    std::vector<ExtractionTurnLeg> roads_on_the_left;
                    ExtractionTurn extraction_turn(0,
                                                   2,
                                                   false,
                                                   true,
                                                   false,
                                                   false,
                                                   TRAVEL_MODE_DRIVING,
                                                   false,
                                                   false,
                                                   1,
                                                   0,
                                                   0,
                                                   0,
                                                   0,
                                                   false,
                                                   TRAVEL_MODE_DRIVING,
                                                   false,
                                                   false,
                                                   1,
                                                   0,
                                                   0,
                                                   0,
                                                   0,
                                                   roads_on_the_right,
                                                   roads_on_the_left);
                    scripting_environment.ProcessTurn(extraction_turn);

                    auto update_direction_penalty =
                        [&extraction_turn, weight_multiplier](
                            bool signal, EdgeDuration &duration_penalty, EdgeWeight &weight_penalty)
                    {
                        if (signal)
                        {
                            duration_penalty = to_alias<EdgeDuration>(extraction_turn.duration *
                                                                      SECOND_TO_DECISECOND);
                            weight_penalty =
                                to_alias<EdgeWeight>(extraction_turn.weight * weight_multiplier);
                        }
                    };

                    update_direction_penalty(has_forward_signal,
                                             forward_node_duration_penalty,
                                             forward_node_weight_penalty);
                    update_direction_penalty(has_reverse_signal,
                                             reverse_node_duration_penalty,
                                             reverse_node_weight_penalty);
                }

                // Get weights before graph is modified
                const auto forward_weight1 = fwd_edge_data1.weight;
                const auto forward_weight2 = fwd_edge_data2.weight;
                const auto forward_duration1 = fwd_edge_data1.duration;
                const auto forward_duration2 = fwd_edge_data2.duration;

                BOOST_ASSERT(EdgeWeight{0} != forward_weight1);
                BOOST_ASSERT(EdgeWeight{0} != forward_weight2);

                const auto reverse_weight1 = rev_edge_data1.weight;
                const auto reverse_weight2 = rev_edge_data2.weight;
                const auto reverse_duration1 = rev_edge_data1.duration;
                const auto reverse_duration2 = rev_edge_data2.duration;

#ifndef NDEBUG
                // Because distances are symmetrical, we only need one
                // per edge - here we double-check that they match
                // their mirrors.
                const auto reverse_distance1 = rev_edge_data1.distance;
                const auto forward_distance1 = fwd_edge_data1.distance;
                const auto forward_distance2 = fwd_edge_data2.distance;
                const auto reverse_distance2 = rev_edge_data2.distance;
                BOOST_ASSERT(forward_distance1 == reverse_distance2);
                BOOST_ASSERT(forward_distance2 == reverse_distance1);
#endif

                BOOST_ASSERT(EdgeWeight{0} != reverse_weight1);
                BOOST_ASSERT(EdgeWeight{0} != reverse_weight2);

                auto apply_e2_to_e1 = [&graph](EdgeID edge1,
                                               EdgeID edge2,
                                               EdgeWeight &weight_penalty,
                                               EdgeDuration &duration_penalty)
                {
                    auto &edge1_data = graph.GetEdgeData(edge1);
                    const auto &edge2_data = graph.GetEdgeData(edge2);
                    edge1_data.weight += edge2_data.weight;
                    edge1_data.duration += edge2_data.duration;
                    edge1_data.distance += edge2_data.distance;
                    if (weight_penalty != INVALID_EDGE_WEIGHT &&
                        duration_penalty != MAXIMAL_EDGE_DURATION)
                    {
                        edge1_data.weight += weight_penalty;
                        edge1_data.duration += duration_penalty;
                        // Note: no penalties for distances
                    }
                };

                apply_e2_to_e1(forward_e1,
                               forward_e2,
                               forward_node_weight_penalty,
                               forward_node_duration_penalty);
                apply_e2_to_e1(reverse_e1,
                               reverse_e2,
                               reverse_node_weight_penalty,
                               reverse_node_duration_penalty);

                // extend e1's to targets of e2's
                graph.SetTarget(forward_e1, node_w);
                graph.SetTarget(reverse_e1, node_u);

                // remove e2's (if bidir, otherwise only one)
                graph.DeleteEdge(node_v, forward_e2);
                graph.DeleteEdge(node_v, reverse_e2);

                // update any involved turn relations
                turn_path_compressor.Compress(node_u, node_v, node_w);

                // Update traffic signal paths containing compressed node.
                traffic_signals.Compress(node_u, node_v, node_w);

                // Forward and reversed compressed edge lengths need to match.
                // Set a dummy empty penalty weight if opposite value exists.
                auto set_dummy_penalty = [](EdgeWeight &weight_penalty,
                                            EdgeDuration &duration_penalty,
                                            EdgeWeight &other_weight_penalty)
                {
                    if (weight_penalty == INVALID_EDGE_WEIGHT &&
                        other_weight_penalty != INVALID_EDGE_WEIGHT)
                    {
                        weight_penalty = {0};
                        duration_penalty = {0};
                    }
                };
                set_dummy_penalty(forward_node_weight_penalty,
                                  forward_node_duration_penalty,
                                  reverse_node_weight_penalty);
                set_dummy_penalty(reverse_node_weight_penalty,
                                  reverse_node_duration_penalty,
                                  forward_node_weight_penalty);

                // store compressed geometry in container
                geometry_compressor.CompressEdge(forward_e1,
                                                 forward_e2,
                                                 node_v,
                                                 node_w,
                                                 forward_weight1,
                                                 forward_weight2,
                                                 forward_duration1,
                                                 forward_duration2,
                                                 forward_node_weight_penalty,
                                                 forward_node_duration_penalty);
                geometry_compressor.CompressEdge(reverse_e1,
                                                 reverse_e2,
                                                 node_v,
                                                 node_u,
                                                 reverse_weight1,
                                                 reverse_weight2,
                                                 reverse_duration1,
                                                 reverse_duration2,
                                                 reverse_node_weight_penalty,
                                                 reverse_node_duration_penalty);
            }
        }
    }

    PrintStatistics(original_number_of_nodes, original_number_of_edges, graph);

    // Repeate the loop, but now add all edges as uncompressed values.
    // The function AddUncompressedEdge does nothing if the edge is already
    // in the CompressedEdgeContainer.
    for (const NodeID node_u : util::irange(0u, original_number_of_nodes))
    {
        for (const auto edge_id : util::irange(graph.BeginEdges(node_u), graph.EndEdges(node_u)))
        {
            const EdgeData &data = graph.GetEdgeData(edge_id);
            const NodeID target = graph.GetTarget(edge_id);
            geometry_compressor.AddUncompressedEdge(edge_id, target, data.weight, data.duration);
        }
    }
}

void GraphCompressor::PrintStatistics(unsigned original_number_of_nodes,
                                      unsigned original_number_of_edges,
                                      const util::NodeBasedDynamicGraph &graph) const
{

    unsigned new_node_count = 0;
    unsigned new_edge_count = 0;

    for (const auto i : util::irange(0u, graph.GetNumberOfNodes()))
    {
        if (graph.GetOutDegree(i) > 0)
        {
            ++new_node_count;
            new_edge_count += (graph.EndEdges(i) - graph.BeginEdges(i));
        }
    }
    util::Log() << "Node compression ratio: " << new_node_count / (double)original_number_of_nodes;
    util::Log() << "Edge compression ratio: " << new_edge_count / (double)original_number_of_edges;
}
} // namespace osrm::extractor
