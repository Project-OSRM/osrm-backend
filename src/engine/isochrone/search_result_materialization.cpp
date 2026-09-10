#include "engine/isochrone/search_result_materialization.hpp"

#include "util/coordinate_calculation.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <unordered_set>
#include <vector>

namespace osrm::engine::isochrone
{
namespace
{

using DurationValue = EdgeDuration::value_type;

bool hasCapacity(const std::size_t used, const std::size_t additional, const std::size_t maximum)
{ return used <= maximum && additional <= maximum - used; }

bool makeDuration(const std::int64_t value, EdgeDuration &duration)
{
    if (value < std::numeric_limits<DurationValue>::min() ||
        value >= std::numeric_limits<DurationValue>::max())
    {
        return false;
    }
    duration = EdgeDuration{static_cast<DurationValue>(value)};
    return true;
}

template <typename DurationRange>
std::optional<std::int64_t> sumDurations(const DurationRange &durations)
{
    std::int64_t total = 0;
    for (const auto duration : durations)
    {
        if (duration == INVALID_SEGMENT_DURATION)
            return std::nullopt;
        total += from_alias<std::uint32_t>(duration);
        if (total >= std::numeric_limits<DurationValue>::max())
            return std::nullopt;
    }
    return total;
}

std::optional<std::int64_t> geometryDuration(const datafacade::BaseDataFacade &facade,
                                             const NodeID node)
{
    const auto geometry_index = facade.GetGeometryIndex(node);
    if (geometry_index.id == SPECIAL_GEOMETRYID)
        return std::nullopt;

    if (geometry_index.forward)
        return sumDurations(facade.GetUncompressedForwardDurations(geometry_index.id));
    return sumDurations(facade.GetUncompressedReverseDurations(geometry_index.id));
}

template <typename WeightRange> std::optional<std::int64_t> sumWeights(const WeightRange &weights)
{
    std::int64_t total = 0;
    for (const auto weight : weights)
    {
        if (weight == INVALID_SEGMENT_WEIGHT)
            return std::nullopt;
        total += from_alias<std::uint32_t>(weight);
        if (total >= std::numeric_limits<EdgeWeight::value_type>::max())
            return std::nullopt;
    }
    return total;
}

std::optional<std::int64_t> geometryWeight(const datafacade::BaseDataFacade &facade,
                                           const NodeID node)
{
    const auto geometry_index = facade.GetGeometryIndex(node);
    if (geometry_index.id == SPECIAL_GEOMETRYID)
        return std::nullopt;

    if (geometry_index.forward)
        return sumWeights(facade.GetUncompressedForwardWeights(geometry_index.id));
    return sumWeights(facade.GetUncompressedReverseWeights(geometry_index.id));
}

MaterializationError appendLabel(const datafacade::BaseDataFacade &facade,
                                 const SearchResult::Node &node,
                                 const bool inbound,
                                 std::vector<DirectedLabel> &labels)
{
    if (node.node == SPECIAL_NODEID || node.duration == INVALID_EDGE_DURATION)
        return MaterializationError::InvalidLabel;

    if (!inbound)
    {
        labels.push_back({node.node, node.duration, DurationAnchor::FirstCoordinate, node.weight});
        return MaterializationError::None;
    }

    const auto total_duration = geometryDuration(facade, node.node);
    const auto total_weight = geometryWeight(facade, node.node);
    if (!total_duration || !total_weight)
        return MaterializationError::InvalidSegmentDuration;

    EdgeDuration last_duration;
    if (!makeDuration(from_alias<std::int64_t>(node.duration) - *total_duration, last_duration))
        return MaterializationError::InvalidLabel;

    const auto last_weight_value = from_alias<std::int64_t>(node.weight) - *total_weight;
    if (last_weight_value < std::numeric_limits<EdgeWeight::value_type>::min() ||
        last_weight_value >= std::numeric_limits<EdgeWeight::value_type>::max())
    {
        return MaterializationError::InvalidLabel;
    }
    const auto last_weight = EdgeWeight{static_cast<EdgeWeight::value_type>(last_weight_value)};

    labels.push_back({node.node, last_duration, DurationAnchor::LastCoordinate, last_weight});
    return MaterializationError::None;
}

template <typename NodeRange, typename WeightRange, typename DurationRange>
MaterializationError appendPhantomClipForGeometry(const datafacade::BaseDataFacade &facade,
                                                  const PhantomPartialTraversal &partial,
                                                  const NodeRange &nodes,
                                                  const WeightRange &weights,
                                                  const DurationRange &durations,
                                                  const std::size_t segment_index,
                                                  const EdgeWeight prefix_weight,
                                                  const EdgeDuration prefix_duration,
                                                  std::vector<GeometryClip> &clips)
{
    if (nodes.size() < 2 || weights.size() + 1 != nodes.size() ||
        durations.size() + 1 != nodes.size() || segment_index >= durations.size() ||
        prefix_weight < EdgeWeight{0} || prefix_weight == INVALID_EDGE_WEIGHT ||
        prefix_duration < EdgeDuration{0} || prefix_duration == INVALID_EDGE_DURATION ||
        weights[segment_index] == INVALID_SEGMENT_WEIGHT ||
        durations[segment_index] == INVALID_SEGMENT_DURATION)
    {
        return MaterializationError::InvalidGeometry;
    }

    const auto full_segment_duration = from_alias<std::uint32_t>(durations[segment_index]);
    const auto full_segment_weight = from_alias<std::uint32_t>(weights[segment_index]);
    const auto duration_prefix = from_alias<std::int64_t>(prefix_duration);
    const auto weight_prefix = from_alias<std::int64_t>(prefix_weight);
    if (duration_prefix < 0 || duration_prefix > full_segment_duration || weight_prefix < 0 ||
        weight_prefix > full_segment_weight)
        return MaterializationError::InvalidGeometry;

    const auto segment_source = facade.GetCoordinateOfNode(nodes[segment_index]);
    const auto segment_target = facade.GetCoordinateOfNode(nodes[segment_index + 1]);
    util::Coordinate projected;
    double fraction = 0.;
    util::coordinate_calculation::perpendicularDistance(
        segment_source, segment_target, partial.phantom.location, projected, fraction);
    fraction = std::clamp(fraction, 0., 1.);

    const auto approach_duration = partial.phantom.approach_duration;
    const auto approach_weight = partial.phantom.approach_weight;
    if (approach_duration < EdgeDuration{0} || approach_duration == INVALID_EDGE_DURATION ||
        approach_weight < EdgeWeight{0} || approach_weight == INVALID_EDGE_WEIGHT)
        return MaterializationError::InvalidLabel;

    if (partial.kind == PhantomTraversalKind::OutboundInitial)
    {
        EdgeDuration remaining_duration;
        if (!makeDuration(static_cast<std::int64_t>(full_segment_duration) - duration_prefix,
                          remaining_duration))
        {
            return MaterializationError::InvalidGeometry;
        }
        const auto remaining_weight = EdgeWeight{static_cast<EdgeWeight::value_type>(
            static_cast<std::int64_t>(full_segment_weight) - weight_prefix)};

        clips.push_back(
            {{partial.node, approach_duration, DurationAnchor::FirstCoordinate, approach_weight},
             {segment_index, fraction},
             {durations.size() - 1, 1.},
             partial.phantom.location,
             std::nullopt,
             remaining_duration,
             std::nullopt,
             remaining_weight,
             std::nullopt});
    }
    else
    {
        clips.push_back(
            {{partial.node, approach_duration, DurationAnchor::LastCoordinate, approach_weight},
             {0, 0.},
             {segment_index, fraction},
             std::nullopt,
             partial.phantom.location,
             std::nullopt,
             prefix_duration,
             std::nullopt,
             prefix_weight});
    }
    return MaterializationError::None;
}

MaterializationError phantomGeometryIndex(const datafacade::BaseDataFacade &facade,
                                          const PhantomPartialTraversal &partial,
                                          GeometryID &geometry_index)
{
    if (partial.node == SPECIAL_NODEID ||
        (partial.direction != PhantomTraversalDirection::Forward &&
         partial.direction != PhantomTraversalDirection::Reverse))
    {
        return MaterializationError::InvalidLabel;
    }

    const auto forward = partial.direction == PhantomTraversalDirection::Forward;
    const auto expected_node =
        forward ? partial.phantom.forward_segment_id.id : partial.phantom.reverse_segment_id.id;
    geometry_index = facade.GetGeometryIndex(partial.node);
    if (partial.node != expected_node || geometry_index.id == SPECIAL_GEOMETRYID ||
        geometry_index.forward != forward)
    {
        return MaterializationError::InvalidGeometry;
    }

    return MaterializationError::None;
}

MaterializationError appendPhantomClip(const datafacade::BaseDataFacade &facade,
                                       const PhantomPartialTraversal &partial,
                                       std::vector<GeometryClip> &clips)
{
    GeometryID geometry_index;
    const auto index_error = phantomGeometryIndex(facade, partial, geometry_index);
    if (index_error != MaterializationError::None)
        return index_error;

    const auto forward = partial.direction == PhantomTraversalDirection::Forward;
    if (forward)
    {
        const auto nodes = facade.GetUncompressedForwardGeometry(geometry_index.id);
        const auto weights = facade.GetUncompressedForwardWeights(geometry_index.id);
        const auto durations = facade.GetUncompressedForwardDurations(geometry_index.id);
        if (partial.phantom.fwd_segment_position >= durations.size())
            return MaterializationError::InvalidGeometry;
        return appendPhantomClipForGeometry(facade,
                                            partial,
                                            nodes,
                                            weights,
                                            durations,
                                            partial.phantom.fwd_segment_position,
                                            partial.phantom.forward_weight,
                                            partial.phantom.forward_duration,
                                            clips);
    }

    const auto nodes = facade.GetUncompressedReverseGeometry(geometry_index.id);
    const auto weights = facade.GetUncompressedReverseWeights(geometry_index.id);
    const auto durations = facade.GetUncompressedReverseDurations(geometry_index.id);
    if (partial.phantom.fwd_segment_position >= durations.size())
        return MaterializationError::InvalidGeometry;
    const auto segment_index = durations.size() - partial.phantom.fwd_segment_position - 1;
    return appendPhantomClipForGeometry(facade,
                                        partial,
                                        nodes,
                                        weights,
                                        durations,
                                        segment_index,
                                        partial.phantom.reverse_weight,
                                        partial.phantom.reverse_duration,
                                        clips);
}

struct ApproachKey
{
    util::Coordinate input_location;
    util::Coordinate snapped_location;
    EdgeDuration duration;

    bool operator==(const ApproachKey &) const = default;
};

struct ApproachKeyHash
{
    std::size_t operator()(const ApproachKey &key) const
    {
        const auto combine = [](const std::size_t seed, const std::int32_t value)
        {
            return seed ^ (std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(value)) +
                           0x9e3779b9U + (seed << 6) + (seed >> 2));
        };

        auto hash = combine(0, from_alias<std::int32_t>(key.input_location.lon));
        hash = combine(hash, from_alias<std::int32_t>(key.input_location.lat));
        hash = combine(hash, from_alias<std::int32_t>(key.snapped_location.lon));
        hash = combine(hash, from_alias<std::int32_t>(key.snapped_location.lat));
        return combine(hash, from_alias<std::int32_t>(key.duration));
    }
};

MaterializationError appendApproach(const PhantomPartialTraversal &partial,
                                    std::size_t &remaining_fragments,
                                    std::unordered_set<ApproachKey, ApproachKeyHash> &approach_keys,
                                    std::vector<WeightedApproach> &approaches)
{
    const auto &phantom = partial.phantom;
    if (phantom.approach_duration < EdgeDuration{0} ||
        phantom.approach_duration == INVALID_EDGE_DURATION ||
        phantom.approach_weight < EdgeWeight{0} || phantom.approach_weight == INVALID_EDGE_WEIGHT)
    {
        return MaterializationError::InvalidLabel;
    }

    if (phantom.approach_duration == EdgeDuration{0} &&
        phantom.approach_distance == EdgeDistance{0})
    {
        return MaterializationError::None;
    }

    const ApproachKey key{phantom.input_location, phantom.location, phantom.approach_duration};
    if (approach_keys.contains(key))
        return MaterializationError::None;
    if (remaining_fragments == 0)
        return MaterializationError::BudgetExceeded;

    approach_keys.insert(key);
    approaches.push_back({{phantom.input_location, EdgeDuration{0}, EdgeWeight{0}},
                          {phantom.location, phantom.approach_duration, phantom.approach_weight}});
    --remaining_fragments;
    return MaterializationError::None;
}

MaterializationResult errorResult(const MaterializationError error) { return {{}, error}; }

} // namespace

MaterializationResult materializeSearchResult(const datafacade::BaseDataFacade &facade,
                                              const SearchResult &search_result,
                                              const EdgeDuration maximum_duration,
                                              const bool inbound,
                                              const MaterializationLimits &limits)
{
    if (!search_result.isComplete())
        return errorResult(MaterializationError::InvalidOptions);
    if (!inbound && !search_result.inbound_frontiers.empty())
        return errorResult(MaterializationError::InvalidLabel);

    const auto maximum_fragments = limits.maximum_input_fragments;
    if (!hasCapacity(search_result.nodes.size(),
                     search_result.inbound_frontiers.size(),
                     maximum_fragments) ||
        !hasCapacity(search_result.nodes.size() + search_result.inbound_frontiers.size(),
                     search_result.competitors.size(),
                     maximum_fragments))
    {
        return errorResult(MaterializationError::BudgetExceeded);
    }

    const auto label_count = search_result.nodes.size() + search_result.inbound_frontiers.size() +
                             search_result.competitors.size();
    std::vector<DirectedLabel> labels;
    labels.reserve(label_count);
    for (const auto &node : search_result.nodes)
    {
        const auto error = appendLabel(facade, node, inbound, labels);
        if (error != MaterializationError::None)
            return errorResult(error);
    }
    for (const auto &frontier : search_result.inbound_frontiers)
    {
        const auto error = appendLabel(facade, frontier.entry, true, labels);
        if (error != MaterializationError::None)
            return errorResult(error);
    }
    for (const auto &competitor : search_result.competitors)
    {
        const auto error = appendLabel(facade, competitor, inbound, labels);
        if (error != MaterializationError::None)
            return errorResult(error);
    }

    std::unordered_set<PackedGeometryID> candidate_geometries;
    candidate_geometries.reserve(labels.size() + search_result.phantom_partials.size());
    for (const auto &label : labels)
    {
        const auto geometry_id = facade.GetGeometryIndex(label.node).id;
        if (geometry_id == SPECIAL_GEOMETRYID)
            return errorResult(MaterializationError::InvalidGeometry);
        candidate_geometries.insert(geometry_id);
    }

    const auto expected_kind =
        inbound ? PhantomTraversalKind::InboundTerminal : PhantomTraversalKind::OutboundInitial;
    for (const auto &partial : search_result.phantom_partials)
    {
        if (partial.kind != expected_kind)
            return errorResult(MaterializationError::InvalidLabel);
        if (!partial.reaches_network)
            continue;

        GeometryID geometry_index;
        const auto error = phantomGeometryIndex(facade, partial, geometry_index);
        if (error != MaterializationError::None)
            return errorResult(error);
        candidate_geometries.insert(geometry_index.id);
    }

    std::vector<GeometryClip> clips;
    std::vector<WeightedApproach> approaches;
    std::unordered_set<ApproachKey, ApproachKeyHash> approach_keys;
    auto remaining_fragments = maximum_fragments - label_count;
    for (const auto &partial : search_result.phantom_partials)
    {
        auto error = appendApproach(partial, remaining_fragments, approach_keys, approaches);
        if (error != MaterializationError::None)
            return errorResult(error);

        GeometryID geometry_index;
        error = phantomGeometryIndex(facade, partial, geometry_index);
        if (error != MaterializationError::None)
            return errorResult(error);
        const auto retain_clip =
            partial.reaches_network || candidate_geometries.contains(geometry_index.id);
        if (!retain_clip)
            continue;

        if (remaining_fragments == 0)
            return errorResult(MaterializationError::BudgetExceeded);
        error = appendPhantomClip(facade, partial, clips);
        if (error != MaterializationError::None)
            return errorResult(error);
        --remaining_fragments;
    }

    return materializeWeightedPolylines(
        facade, labels, clips, approaches, {maximum_duration, limits, false});
}

} // namespace osrm::engine::isochrone
