#ifndef OSRM_ENGINE_ISOCHRONE_DURATION_GRAPH_BUILDER_HPP
#define OSRM_ENGINE_ISOCHRONE_DURATION_GRAPH_BUILDER_HPP

#include "engine/isochrone/duration_graph.hpp"

#include "extractor/isochrone_transition.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/typedefs.hpp"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace osrm::engine::isochrone
{
namespace detail
{
inline bool isUnavailableNodeWeight(const EdgeWeight weight)
{ return (weight & EdgeWeight{0x7fffffff}) == INVALID_EDGE_WEIGHT; }
} // namespace detail

inline DurationGraph
buildDurationGraph(const EdgeID number_of_nodes,
                   const std::vector<extractor::IsochroneTransition> &transitions,
                   const std::vector<EdgeWeight> &node_weights,
                   const std::vector<EdgeDuration> &node_durations,
                   const std::vector<TurnPenalty> &turn_weight_penalties,
                   const std::vector<TurnPenalty> &turn_duration_penalties,
                   const std::vector<EdgeDuration> &node_duration_lower_bounds = {},
                   const std::vector<EdgeWeight> &node_weight_lower_bounds = {})
{
    if (node_weights.size() != number_of_nodes || node_durations.size() != number_of_nodes ||
        turn_weight_penalties.size() != turn_duration_penalties.size() ||
        (!node_duration_lower_bounds.empty() &&
         node_duration_lower_bounds.size() != number_of_nodes) ||
        (!node_weight_lower_bounds.empty() && node_weight_lower_bounds.size() != number_of_nodes))
    {
        throw util::exception("Cannot generate isochrone data from inconsistent metric arrays" +
                              std::string(SOURCE_REF));
    }

    if (!std::is_sorted(transitions.begin(),
                        transitions.end(),
                        [](const auto &left, const auto &right)
                        {
                            return std::tie(left.source, left.target, left.turn_id) <
                                   std::tie(right.source, right.target, right.turn_id);
                        }))
    {
        throw util::exception("Cannot generate isochrone data from unsorted transitions" +
                              std::string(SOURCE_REF));
    }

    const auto evaluate =
        [&](const extractor::IsochroneTransition &transition) -> std::optional<DurationGraphArc>
    {
        if (transition.source >= number_of_nodes || transition.target >= number_of_nodes ||
            transition.turn_id >= turn_duration_penalties.size())
        {
            throw util::exception("Cannot generate isochrone data from an invalid transition" +
                                  std::string(SOURCE_REF));
        }

        if (detail::isUnavailableNodeWeight(node_weights[transition.source]) ||
            detail::isUnavailableNodeWeight(node_weights[transition.target]) ||
            turn_weight_penalties[transition.turn_id] == INVALID_TURN_PENALTY ||
            turn_duration_penalties[transition.turn_id] == INVALID_TURN_PENALTY)
        {
            return std::nullopt;
        }

        const auto node_weight = EdgeWeight{static_cast<EdgeWeight::value_type>(
            static_cast<std::uint32_t>(
                from_alias<EdgeWeight::value_type>(node_weights[transition.source])) &
            0x7fffffffU)};
        const auto node_duration = node_durations[transition.source];
        const auto turn_weight = turn_weight_penalties[transition.turn_id];
        const auto turn_duration = turn_duration_penalties[transition.turn_id];
        if (node_duration < EdgeDuration{0} || node_duration == INVALID_EDGE_DURATION)
        {
            throw util::exception("Cannot generate isochrone data from an invalid node duration" +
                                  std::string(SOURCE_REF));
        }

        // A negative turn duration makes the duration at the beginning of the next geometry
        // less than the duration at the end of this one.  Although the total arc might still be
        // nonnegative, that violates the monotonic geometry labels required to materialize an
        // exact outbound or inbound contour.
        if (turn_duration < TurnPenalty{0})
        {
            throw util::exception("Cannot generate isochrone data with a negative turn duration "
                                  "penalty." +
                                  std::string(SOURCE_REF));
        }

        const auto turn_weight_value =
            static_cast<std::int64_t>(from_alias<TurnPenalty::value_type>(turn_weight));
        const auto turn_duration_value =
            static_cast<std::int64_t>(from_alias<TurnPenalty::value_type>(turn_duration));
        auto weight = static_cast<std::int64_t>(from_alias<EdgeWeight::value_type>(node_weight)) +
                      turn_weight_value;
        auto duration =
            static_cast<std::int64_t>(from_alias<EdgeDuration::value_type>(node_duration)) +
            turn_duration_value;
        if (!node_weight_lower_bounds.empty())
        {
            const auto lower_bound = static_cast<std::int64_t>(
                from_alias<EdgeWeight::value_type>(node_weight_lower_bounds[transition.source]));
            // When a traffic update leaves a nonnegative turn penalty in place, Updater clamps
            // the source geometry before adding that turn.  Reproduce that per-transition rule
            // without corrupting the shared node metric for other turns.
            if (weight < lower_bound)
                weight = lower_bound + turn_weight_value;
        }
        if (!node_duration_lower_bounds.empty())
        {
            const auto lower_bound = static_cast<std::int64_t>(from_alias<EdgeDuration::value_type>(
                node_duration_lower_bounds[transition.source]));
            // Updater applies this bound to the source geometry before adding a nonnegative
            // turn penalty, but only when their original sum is below the bound. Mirror that
            // per-transition rule rather than clamping the shared node duration.
            if (duration < lower_bound)
                duration = lower_bound + turn_duration_value;
        }
        // The normal edge-expanded graph keeps routing costs positive even if a profile or a
        // turn adjustment produces a lower value.  Its duration remains an auxiliary metric.
        weight = std::max(weight, std::int64_t{1});
        if (weight >= from_alias<std::int64_t>(INVALID_EDGE_WEIGHT) || duration < 0 ||
            duration >= from_alias<std::int64_t>(INVALID_EDGE_DURATION))
        {
            throw util::exception("Cannot generate isochrone data: transition " +
                                  std::to_string(transition.source) + " -> " +
                                  std::to_string(transition.target) + " has invalid metrics " +
                                  std::to_string(weight) + ", " + std::to_string(duration) + "." +
                                  std::string(SOURCE_REF));
        }

        return DurationGraphArc{transition.target,
                                EdgeWeight{static_cast<EdgeWeight::value_type>(weight)},
                                EdgeDuration{static_cast<EdgeDuration::value_type>(duration)}};
    };

    const auto for_each_best_arc = [&](const auto &callback)
    {
        for (auto begin = transitions.begin(); begin != transitions.end();)
        {
            const auto source = begin->source;
            const auto target = begin->target;
            auto end = begin;
            std::optional<DurationGraphArc> best_arc;
            while (end != transitions.end() && end->source == source && end->target == target)
            {
                if (const auto arc = evaluate(*end);
                    arc && (!best_arc || std::tie(arc->weight, arc->duration) <
                                             std::tie(best_arc->weight, best_arc->duration)))
                {
                    best_arc = *arc;
                }
                ++end;
            }
            if (best_arc)
                callback(source, target, *best_arc);
            begin = end;
        }
    };

    DurationGraph graph;
    graph.forward_offsets.assign(static_cast<std::size_t>(number_of_nodes) + 1, 0);
    graph.reverse_offsets.assign(static_cast<std::size_t>(number_of_nodes) + 1, 0);
    for_each_best_arc(
        [&](const NodeID source, const NodeID target, const DurationGraphArc &)
        {
            ++graph.forward_offsets[source + 1];
            ++graph.reverse_offsets[target + 1];
        });
    std::partial_sum(
        graph.forward_offsets.begin(), graph.forward_offsets.end(), graph.forward_offsets.begin());
    std::partial_sum(
        graph.reverse_offsets.begin(), graph.reverse_offsets.end(), graph.reverse_offsets.begin());

    graph.forward_arcs.resize(graph.forward_offsets.back());
    graph.reverse_arcs.resize(graph.reverse_offsets.back());
    auto next_forward = graph.forward_offsets;
    auto next_reverse = graph.reverse_offsets;
    for_each_best_arc(
        [&](const NodeID source, const NodeID target, const DurationGraphArc &arc)
        {
            graph.forward_arcs[next_forward[source]++] = arc;
            graph.reverse_arcs[next_reverse[target]++] = {source, arc.weight, arc.duration};
        });

    if (!isValidDurationGraph(graph, number_of_nodes))
    {
        throw util::exception("Generated an invalid isochrone duration graph" +
                              std::string(SOURCE_REF));
    }
    return graph;
}

} // namespace osrm::engine::isochrone

#endif // OSRM_ENGINE_ISOCHRONE_DURATION_GRAPH_BUILDER_HPP
