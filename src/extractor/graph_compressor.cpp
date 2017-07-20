#include "extractor/graph_compressor.hpp"

#include "extractor/compressed_edge_container.hpp"
#include "extractor/extraction_turn.hpp"
#include "extractor/guidance/intersection.hpp"
#include "extractor/restriction.hpp"
#include "extractor/restriction_compressor.hpp"

#include "util/dynamic_graph.hpp"
#include "util/node_based_graph.hpp"
#include "util/percent.hpp"

#include "util/log.hpp"

#include <boost/assert.hpp>
#include <unordered_set>

namespace osrm
{
namespace extractor
{

void GraphCompressor::Compress(const std::unordered_set<NodeID> &barrier_nodes,
                               const std::unordered_set<NodeID> &traffic_signals,
                               ScriptingEnvironment &scripting_environment,
                               std::vector<TurnRestriction> &turn_restrictions,
                               util::NodeBasedDynamicGraph &graph,
                               CompressedEdgeContainer &geometry_compressor)
{
    const unsigned original_number_of_nodes = graph.GetNumberOfNodes();
    const unsigned original_number_of_edges = graph.GetNumberOfEdges();

    RestrictionCompressor restriction_compressor(turn_restrictions);

    // we do not compress turn restrictions on degree two nodes. These nodes are usually used to
    // indicated `directed` barriers
    std::unordered_set<NodeID> restriction_via_nodes;

    const auto remember_via_nodes = [&](const auto &restriction) {
        if (restriction.Type() == RestrictionType::NODE_RESTRICTION)
        {
            const auto &node = restriction.AsNodeRestriction();
            restriction_via_nodes.insert(node.via);
        }
        else
        {
            BOOST_ASSERT(restriction.Type() == RestrictionType::WAY_RESTRICTION);
            const auto &way = restriction.AsWayRestriction();
            restriction_via_nodes.insert(way.in_restriction.via);
            restriction_via_nodes.insert(way.out_restriction.via);
        }
    };
    std::for_each(turn_restrictions.begin(), turn_restrictions.end(), remember_via_nodes);

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

            // check if v is a via node for a turn restriction, i.e. a 'directed' barrier node
            if (restriction_via_nodes.count(node_v))
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

            if (graph.FindEdgeInEitherDirection(node_u, node_w) != SPECIAL_EDGEID)
            {
                continue;
            }

            // this case can happen if two ways with different names overlap
            if (fwd_edge_data1.name_id != rev_edge_data1.name_id ||
                fwd_edge_data2.name_id != rev_edge_data2.name_id)
            {
                continue;
            }

            if (fwd_edge_data1.CanCombineWith(fwd_edge_data2) &&
                rev_edge_data1.CanCombineWith(rev_edge_data2))
            {
                BOOST_ASSERT(graph.GetEdgeData(forward_e1).name_id ==
                             graph.GetEdgeData(reverse_e1).name_id);
                BOOST_ASSERT(graph.GetEdgeData(forward_e2).name_id ==
                             graph.GetEdgeData(reverse_e2).name_id);

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
                 * turn-lanes. Without this,we would have to treat any turn-lane beginning/ending
                 * just
                 * like a barrier.
                 */
                const auto selectLaneID = [](const LaneDescriptionID front,
                                             const LaneDescriptionID back) {
                    // A lane has tags: u - (front) - v - (back) - w
                    // During contraction, we keep only one of the tags. Usually the one closer to
                    // the
                    // intersection is preferred. If its empty, however, we keep the non-empty one
                    if (back == INVALID_LANE_DESCRIPTIONID)
                        return front;
                    return back;
                };
                graph.GetEdgeData(forward_e1).lane_description_id = selectLaneID(
                    fwd_edge_data1.lane_description_id, fwd_edge_data2.lane_description_id);
                graph.GetEdgeData(reverse_e1).lane_description_id = selectLaneID(
                    rev_edge_data1.lane_description_id, rev_edge_data2.lane_description_id);
                graph.GetEdgeData(forward_e2).lane_description_id = selectLaneID(
                    fwd_edge_data2.lane_description_id, fwd_edge_data1.lane_description_id);
                graph.GetEdgeData(reverse_e2).lane_description_id = selectLaneID(
                    rev_edge_data2.lane_description_id, rev_edge_data1.lane_description_id);

                /*
                // Do not compress edge if it crosses a traffic signal.
                // This can't be done in CanCombineWith, becase we only store the
                // traffic signals in the `traffic signal` list, which EdgeData
                // doesn't have access to.
                */
                const bool has_node_penalty = traffic_signals.find(node_v) != traffic_signals.end();
                boost::optional<EdgeDuration> node_duration_penalty = boost::none;
                boost::optional<EdgeWeight> node_weight_penalty = boost::none;
                if (has_node_penalty)
                {
                    // generate an artifical turn for the turn penalty generation
                    ExtractionTurn extraction_turn(true);

                    extraction_turn.source_restricted = fwd_edge_data1.restricted;
                    extraction_turn.target_restricted = fwd_edge_data2.restricted;

                    // we cannot handle this as node penalty, if it depends on turn direction
                    if (extraction_turn.source_restricted != extraction_turn.target_restricted)
                        continue;

                    scripting_environment.ProcessTurn(extraction_turn);
                    node_duration_penalty = extraction_turn.duration * 10;
                    node_weight_penalty = extraction_turn.weight * weight_multiplier;
                }

                // Get weights before graph is modified
                const auto forward_weight1 = fwd_edge_data1.weight;
                const auto forward_weight2 = fwd_edge_data2.weight;
                const auto forward_duration1 = fwd_edge_data1.duration;
                const auto forward_duration2 = fwd_edge_data2.duration;

                BOOST_ASSERT(0 != forward_weight1);
                BOOST_ASSERT(0 != forward_weight2);

                const auto reverse_weight1 = rev_edge_data1.weight;
                const auto reverse_weight2 = rev_edge_data2.weight;
                const auto reverse_duration1 = rev_edge_data1.duration;
                const auto reverse_duration2 = rev_edge_data2.duration;

                BOOST_ASSERT(0 != reverse_weight1);
                BOOST_ASSERT(0 != reverse_weight2);

                // add weight of e2's to e1
                graph.GetEdgeData(forward_e1).weight += forward_weight2;
                graph.GetEdgeData(reverse_e1).weight += reverse_weight2;

                // add duration of e2's to e1
                graph.GetEdgeData(forward_e1).duration += forward_duration2;
                graph.GetEdgeData(reverse_e1).duration += reverse_duration2;

                if (node_weight_penalty && node_duration_penalty)
                {
                    graph.GetEdgeData(forward_e1).weight += *node_weight_penalty;
                    graph.GetEdgeData(reverse_e1).weight += *node_weight_penalty;
                    graph.GetEdgeData(forward_e1).duration += *node_duration_penalty;
                    graph.GetEdgeData(reverse_e1).duration += *node_duration_penalty;
                }

                // extend e1's to targets of e2's
                graph.SetTarget(forward_e1, node_w);
                graph.SetTarget(reverse_e1, node_u);

                // remove e2's (if bidir, otherwise only one)
                graph.DeleteEdge(node_v, forward_e2);
                graph.DeleteEdge(node_v, reverse_e2);

                // update any involved turn restrictions
                restriction_compressor.Compress(node_u, node_v, node_w);

                // store compressed geometry in container
                geometry_compressor.CompressEdge(forward_e1,
                                                 forward_e2,
                                                 node_v,
                                                 node_w,
                                                 forward_weight1,
                                                 forward_weight2,
                                                 forward_duration1,
                                                 forward_duration2,
                                                 node_weight_penalty,
                                                 node_duration_penalty);
                geometry_compressor.CompressEdge(reverse_e1,
                                                 reverse_e2,
                                                 node_v,
                                                 node_u,
                                                 reverse_weight1,
                                                 reverse_weight2,
                                                 reverse_duration1,
                                                 reverse_duration2,
                                                 node_weight_penalty,
                                                 node_duration_penalty);
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
}
}
