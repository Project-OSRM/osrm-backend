#include "engine/isochrone/weighted_polyline_materialization.hpp"

#include "engine/isochrone/duration.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

namespace osrm::engine::isochrone
{
namespace
{

struct InternalPoint
{
    util::Coordinate coordinate;
    long double weight;
    long double duration;
};

using InternalPolyline = std::vector<InternalPoint>;

struct GeometryData
{
    std::vector<util::Coordinate> coordinates;
    std::vector<SegmentWeight> segment_weights;
    std::vector<SegmentDuration> segment_durations;
};

bool isValidLabelDuration(const EdgeDuration duration) { return duration != INVALID_EDGE_DURATION; }

bool isValidLabelWeight(const EdgeWeight weight) { return weight != INVALID_EDGE_WEIGHT; }

bool isValidNonnegativeDuration(const EdgeDuration duration)
{ return isValidLabelDuration(duration) && from_alias<std::int64_t>(duration) >= 0; }

bool isValidAnchor(const DurationAnchor anchor)
{ return anchor == DurationAnchor::FirstCoordinate || anchor == DurationAnchor::LastCoordinate; }

bool hasCapacity(const std::size_t used, const std::size_t additional, const std::size_t maximum)
{ return used <= maximum && additional <= maximum - used; }

bool hasValidOptions(const MaterializationOptions &options)
{ return isValidNonnegativeDuration(options.maximum_duration); }

double longitude(const util::Coordinate coordinate)
{ return static_cast<double>(util::toFloating(coordinate.lon)); }

double latitude(const util::Coordinate coordinate)
{ return static_cast<double>(util::toFloating(coordinate.lat)); }

MaterializationError validateCoordinate(const util::Coordinate coordinate)
{
    if (!coordinate.IsValid())
        return MaterializationError::InvalidCoordinate;

    if (std::abs(latitude(coordinate)) >= 90.)
        return MaterializationError::TouchesPole;

    return MaterializationError::None;
}

MaterializationError validateSegment(const util::Coordinate from, const util::Coordinate to)
{
    const auto from_error = validateCoordinate(from);
    if (from_error != MaterializationError::None)
        return from_error;

    const auto to_error = validateCoordinate(to);
    if (to_error != MaterializationError::None)
        return to_error;

    if (std::abs(longitude(to) - longitude(from)) > 180.)
        return MaterializationError::RequiresLongitudeWrap;

    return MaterializationError::None;
}

util::Coordinate interpolateCoordinate(const util::Coordinate from,
                                       const util::Coordinate to,
                                       const long double fraction)
{
    BOOST_ASSERT(fraction >= 0.);
    BOOST_ASSERT(fraction <= 1.);

    if (fraction == 0.)
        return from;
    if (fraction == 1.)
        return to;

    const auto interpolated_lon = static_cast<double>(
        static_cast<long double>(longitude(from)) +
        (static_cast<long double>(longitude(to)) - static_cast<long double>(longitude(from))) *
            fraction);
    const auto interpolated_lat = static_cast<double>(
        static_cast<long double>(latitude(from)) +
        (static_cast<long double>(latitude(to)) - static_cast<long double>(latitude(from))) *
            fraction);
    return {util::FloatLongitude{interpolated_lon}, util::FloatLatitude{interpolated_lat}};
}

bool canonicalizePosition(GeometryPosition &position,
                          const std::size_t segment_count,
                          const bool has_segment_duration_override)
{
    if (position.segment_index >= segment_count || !std::isfinite(position.fraction) ||
        position.fraction < 0. || position.fraction > 1.)
        return false;

    if (position.fraction == 1. && position.segment_index + 1 < segment_count &&
        !has_segment_duration_override)
    {
        ++position.segment_index;
        position.fraction = 0.;
    }
    return true;
}

bool precedesOrEquals(const GeometryPosition left, const GeometryPosition right)
{
    return left.segment_index < right.segment_index ||
           (left.segment_index == right.segment_index && left.fraction <= right.fraction);
}

bool isSamePosition(const GeometryPosition left, const GeometryPosition right)
{ return left.segment_index == right.segment_index && left.fraction == right.fraction; }

MaterializationError readGeometry(const datafacade::BaseDataFacade &facade,
                                  const DirectedLabel &label,
                                  const MaterializationLimits &limits,
                                  std::size_t &materialized_geometry_points,
                                  GeometryData &geometry)
{
    if (label.node == SPECIAL_NODEID || !isValidLabelDuration(label.duration) ||
        !isValidLabelWeight(label.weight) || !isValidAnchor(label.anchor))
        return MaterializationError::InvalidLabel;

    const auto geometry_index = facade.GetGeometryIndex(label.node);
    if (geometry_index.id == SPECIAL_GEOMETRYID)
        return MaterializationError::InvalidGeometry;

    geometry.coordinates.clear();
    geometry.segment_weights.clear();
    geometry.segment_durations.clear();

    if (geometry_index.forward)
    {
        const auto nodes = facade.GetUncompressedForwardGeometry(geometry_index.id);
        const auto weights = facade.GetUncompressedForwardWeights(geometry_index.id);
        const auto durations = facade.GetUncompressedForwardDurations(geometry_index.id);
        if (nodes.size() < 2 || weights.size() + 1 != nodes.size() ||
            durations.size() + 1 != nodes.size())
            return MaterializationError::InvalidGeometry;
        if (!hasCapacity(materialized_geometry_points,
                         nodes.size(),
                         limits.maximum_materialized_geometry_points))
            return MaterializationError::BudgetExceeded;

        geometry.coordinates.reserve(nodes.size());
        geometry.segment_weights.reserve(weights.size());
        geometry.segment_durations.reserve(durations.size());
        for (const auto node : nodes)
            geometry.coordinates.push_back(facade.GetCoordinateOfNode(node));
        for (const auto weight : weights)
            geometry.segment_weights.push_back(weight);
        for (const auto duration : durations)
            geometry.segment_durations.push_back(duration);
    }
    else
    {
        const auto nodes = facade.GetUncompressedReverseGeometry(geometry_index.id);
        const auto weights = facade.GetUncompressedReverseWeights(geometry_index.id);
        const auto durations = facade.GetUncompressedReverseDurations(geometry_index.id);
        if (nodes.size() < 2 || weights.size() + 1 != nodes.size() ||
            durations.size() + 1 != nodes.size())
            return MaterializationError::InvalidGeometry;
        if (!hasCapacity(materialized_geometry_points,
                         nodes.size(),
                         limits.maximum_materialized_geometry_points))
            return MaterializationError::BudgetExceeded;

        geometry.coordinates.reserve(nodes.size());
        geometry.segment_weights.reserve(weights.size());
        geometry.segment_durations.reserve(durations.size());
        for (const auto node : nodes)
            geometry.coordinates.push_back(facade.GetCoordinateOfNode(node));
        for (const auto weight : weights)
            geometry.segment_weights.push_back(weight);
        for (const auto duration : durations)
            geometry.segment_durations.push_back(duration);
    }
    materialized_geometry_points += geometry.coordinates.size();

    for (const auto coordinate : geometry.coordinates)
    {
        const auto error = validateCoordinate(coordinate);
        if (error != MaterializationError::None)
            return error;
    }

    for (std::size_t index = 0; index < geometry.segment_durations.size(); ++index)
    {
        const auto error =
            validateSegment(geometry.coordinates[index], geometry.coordinates[index + 1]);
        if (error != MaterializationError::None)
            return error;
    }
    return MaterializationError::None;
}

MaterializationError
buildClippedGeometry(const GeometryData &geometry,
                     GeometryPosition first,
                     GeometryPosition last,
                     const std::optional<util::Coordinate> &first_coordinate_override,
                     const std::optional<util::Coordinate> &last_coordinate_override,
                     const std::optional<EdgeDuration> &first_to_segment_end_duration,
                     const std::optional<EdgeDuration> &last_from_segment_start_duration,
                     const std::optional<EdgeWeight> &first_to_segment_end_weight,
                     const std::optional<EdgeWeight> &last_from_segment_start_weight,
                     const DirectedLabel &label,
                     InternalPolyline &polyline)
{
    const auto segment_count = geometry.segment_durations.size();
    if (first_to_segment_end_duration.has_value() != first_to_segment_end_weight.has_value() ||
        last_from_segment_start_duration.has_value() !=
            last_from_segment_start_weight.has_value() ||
        !canonicalizePosition(first, segment_count, first_to_segment_end_duration.has_value()) ||
        !canonicalizePosition(last, segment_count, last_from_segment_start_duration.has_value()) ||
        !precedesOrEquals(first, last))
        return MaterializationError::InvalidGeometry;

    // A phantom source/target may legally cover only one side of a geometry whose other side was
    // closed by a traffic update. Complete labels still span and validate the entire geometry.
    for (std::size_t index = first.segment_index; index <= last.segment_index; ++index)
    {
        if (geometry.segment_weights[index] == INVALID_SEGMENT_WEIGHT ||
            geometry.segment_durations[index] == INVALID_SEGMENT_DURATION)
            return MaterializationError::InvalidSegmentDuration;
    }

    if ((first_to_segment_end_duration &&
         !isValidNonnegativeDuration(*first_to_segment_end_duration)) ||
        (last_from_segment_start_duration &&
         !isValidNonnegativeDuration(*last_from_segment_start_duration)))
        return MaterializationError::InvalidGeometry;
    if ((first_to_segment_end_weight && (!isValidLabelWeight(*first_to_segment_end_weight) ||
                                         *first_to_segment_end_weight < EdgeWeight{0})) ||
        (last_from_segment_start_weight && (!isValidLabelWeight(*last_from_segment_start_weight) ||
                                            *last_from_segment_start_weight < EdgeWeight{0})))
        return MaterializationError::InvalidGeometry;
    if ((first_to_segment_end_duration &&
         from_alias<std::uint32_t>(*first_to_segment_end_duration) >
             from_alias<std::uint32_t>(geometry.segment_durations[first.segment_index])) ||
        (last_from_segment_start_duration &&
         from_alias<std::uint32_t>(*last_from_segment_start_duration) >
             from_alias<std::uint32_t>(geometry.segment_durations[last.segment_index])))
        return MaterializationError::InvalidGeometry;
    if ((first_to_segment_end_weight &&
         from_alias<std::uint32_t>(*first_to_segment_end_weight) >
             from_alias<std::uint32_t>(geometry.segment_weights[first.segment_index])) ||
        (last_from_segment_start_weight &&
         from_alias<std::uint32_t>(*last_from_segment_start_weight) >
             from_alias<std::uint32_t>(geometry.segment_weights[last.segment_index])))
        return MaterializationError::InvalidGeometry;

    polyline.clear();
    const auto pointAt = [&](const GeometryPosition position,
                             const std::optional<util::Coordinate> &coordinate_override)
    {
        if (coordinate_override)
            return *coordinate_override;
        return interpolateCoordinate(geometry.coordinates[position.segment_index],
                                     geometry.coordinates[position.segment_index + 1],
                                     position.fraction);
    };
    const auto segmentDuration = [&](const std::size_t index)
    {
        const auto full_duration = from_alias<long double>(geometry.segment_durations[index]);
        if (first.segment_index == last.segment_index)
        {
            if (first_to_segment_end_duration && last_from_segment_start_duration)
                return -1.L;
            if (first_to_segment_end_duration)
            {
                if (last.fraction != 1.)
                    return -1.L;
                return from_alias<long double>(*first_to_segment_end_duration);
            }
            if (last_from_segment_start_duration)
            {
                if (first.fraction != 0.)
                    return -1.L;
                return from_alias<long double>(*last_from_segment_start_duration);
            }
            return (static_cast<long double>(last.fraction) - first.fraction) * full_duration;
        }
        if (index == first.segment_index && first_to_segment_end_duration)
            return from_alias<long double>(*first_to_segment_end_duration);
        if (index == last.segment_index && last_from_segment_start_duration)
            return from_alias<long double>(*last_from_segment_start_duration);

        const auto start_fraction = index == first.segment_index ? first.fraction : 0.;
        const auto end_fraction = index == last.segment_index ? last.fraction : 1.;
        return (static_cast<long double>(end_fraction) - start_fraction) * full_duration;
    };
    const auto segmentWeight = [&](const std::size_t index)
    {
        const auto full_weight = from_alias<long double>(geometry.segment_weights[index]);
        if (first.segment_index == last.segment_index)
        {
            if (first_to_segment_end_weight && last_from_segment_start_weight)
                return -1.L;
            if (first_to_segment_end_weight)
            {
                if (last.fraction != 1.)
                    return -1.L;
                return from_alias<long double>(*first_to_segment_end_weight);
            }
            if (last_from_segment_start_weight)
            {
                if (first.fraction != 0.)
                    return -1.L;
                return from_alias<long double>(*last_from_segment_start_weight);
            }
            return (static_cast<long double>(last.fraction) - first.fraction) * full_weight;
        }
        if (index == first.segment_index && first_to_segment_end_weight)
            return from_alias<long double>(*first_to_segment_end_weight);
        if (index == last.segment_index && last_from_segment_start_weight)
            return from_alias<long double>(*last_from_segment_start_weight);

        const auto start_fraction = index == first.segment_index ? first.fraction : 0.;
        const auto end_fraction = index == last.segment_index ? last.fraction : 1.;
        return (static_cast<long double>(end_fraction) - start_fraction) * full_weight;
    };

    const auto first_coordinate =
        isSamePosition(first, last) && !first_coordinate_override && last_coordinate_override
            ? *last_coordinate_override
            : pointAt(first, first_coordinate_override);
    const auto first_coordinate_error = validateCoordinate(first_coordinate);
    if (first_coordinate_error != MaterializationError::None)
        return first_coordinate_error;
    if (last_coordinate_override)
    {
        const auto last_coordinate_error = validateCoordinate(*last_coordinate_override);
        if (last_coordinate_error != MaterializationError::None)
            return last_coordinate_error;
    }
    if (isSamePosition(first, last))
    {
        if (first_coordinate_override && last_coordinate_override &&
            *first_coordinate_override != *last_coordinate_override)
            return MaterializationError::InvalidGeometry;
        if ((first_to_segment_end_duration && *first_to_segment_end_duration != EdgeDuration{0}) ||
            (last_from_segment_start_duration &&
             *last_from_segment_start_duration != EdgeDuration{0}))
            return MaterializationError::InvalidGeometry;
        polyline.push_back({first_coordinate,
                            from_alias<long double>(label.weight),
                            from_alias<long double>(label.duration)});
        return MaterializationError::None;
    }

    long double total_duration = 0.;
    long double total_weight = 0.;
    for (std::size_t index = first.segment_index; index <= last.segment_index; ++index)
    {
        const auto segment_duration = segmentDuration(index);
        const auto segment_weight = segmentWeight(index);
        if (segment_duration < 0. || segment_weight < 0.)
            return MaterializationError::InvalidGeometry;
        total_duration += segment_duration;
        total_weight += segment_weight;
    }

    const auto anchor_duration = from_alias<long double>(label.duration);
    const auto anchor_weight = from_alias<long double>(label.weight);
    polyline.push_back(
        {first_coordinate,
         label.anchor == DurationAnchor::FirstCoordinate ? anchor_weight
                                                         : anchor_weight + total_weight,
         label.anchor == DurationAnchor::FirstCoordinate ? anchor_duration
                                                         : anchor_duration + total_duration});

    auto duration_from_first = 0.L;
    auto weight_from_first = 0.L;
    for (std::size_t index = first.segment_index; index <= last.segment_index; ++index)
    {
        const auto end_fraction = index == last.segment_index ? last.fraction : 1.;
        duration_from_first += segmentDuration(index);
        weight_from_first += segmentWeight(index);
        const auto endpoint = GeometryPosition{index, end_fraction};
        const auto duration = label.anchor == DurationAnchor::FirstCoordinate
                                  ? anchor_duration + duration_from_first
                                  : anchor_duration + total_duration - duration_from_first;
        const auto weight = label.anchor == DurationAnchor::FirstCoordinate
                                ? anchor_weight + weight_from_first
                                : anchor_weight + total_weight - weight_from_first;
        const auto coordinate_override =
            index == last.segment_index ? last_coordinate_override : std::nullopt;
        polyline.push_back({pointAt(endpoint, coordinate_override), weight, duration});
    }

    for (std::size_t index = 1; index < polyline.size(); ++index)
    {
        const auto error =
            validateSegment(polyline[index - 1].coordinate, polyline[index].coordinate);
        if (error != MaterializationError::None)
            return error;
    }

    return MaterializationError::None;
}

MaterializationError appendClippedPolyline(const InternalPolyline &input,
                                           const long double maximum_duration,
                                           const std::optional<PackedGeometryID> geometry_id,
                                           const bool clip_to_maximum_duration,
                                           const MaterializationLimits &limits,
                                           std::vector<WeightedPolyline> &output,
                                           std::size_t &output_points)
{
    BOOST_ASSERT(!input.empty());

    WeightedPolyline active;
    const auto addPoint = [&](const InternalPoint &point) -> MaterializationError
    {
        const auto duration = durationToSeconds(point.duration);
        const auto weight = static_cast<double>(point.weight);
        if (!std::isfinite(duration) || !std::isfinite(weight))
            return MaterializationError::InvalidGeometry;
        if (!active.empty() && active.back().coordinate == point.coordinate &&
            active.back().duration == duration && active.back().weight == weight)
            return MaterializationError::None;
        if (!hasCapacity(output_points, 1, limits.maximum_output_points))
            return MaterializationError::BudgetExceeded;
        active.push_back({point.coordinate, duration, weight, geometry_id});
        ++output_points;
        return MaterializationError::None;
    };
    const auto flush = [&]() -> MaterializationError
    {
        if (active.empty())
            return MaterializationError::None;
        const auto has_spatial_extent = std::any_of(
            active.begin() + 1,
            active.end(),
            [&](const auto &point) { return point.coordinate != active.front().coordinate; });
        if (!has_spatial_extent)
        {
            BOOST_ASSERT(output_points >= active.size());
            output_points -= active.size();
            active.clear();
            return MaterializationError::None;
        }
        if (!hasCapacity(output.size(), 1, limits.maximum_output_polylines))
            return MaterializationError::BudgetExceeded;
        output.push_back(std::move(active));
        active.clear();
        return MaterializationError::None;
    };
    const auto interpolate =
        [](const InternalPoint &from, const InternalPoint &to, const long double cutoff)
    {
        BOOST_ASSERT(from.duration != to.duration);
        const auto fraction = (cutoff - from.duration) / (to.duration - from.duration);
        BOOST_ASSERT(fraction >= 0.);
        BOOST_ASSERT(fraction <= 1.);
        return InternalPoint{interpolateCoordinate(from.coordinate, to.coordinate, fraction),
                             from.weight + (to.weight - from.weight) * fraction,
                             cutoff};
    };

    if (!clip_to_maximum_duration)
    {
        for (const auto &point : input)
        {
            const auto error = addPoint(point);
            if (error != MaterializationError::None)
                return error;
        }
        return flush();
    }

    if (input.front().duration <= maximum_duration)
    {
        const auto error = addPoint(input.front());
        if (error != MaterializationError::None)
            return error;
    }

    for (std::size_t index = 1; index < input.size(); ++index)
    {
        const auto &from = input[index - 1];
        const auto &to = input[index];
        const auto from_in_range = from.duration <= maximum_duration;
        const auto to_in_range = to.duration <= maximum_duration;

        if (from_in_range && to_in_range)
        {
            if (active.empty())
            {
                const auto error = addPoint(from);
                if (error != MaterializationError::None)
                    return error;
            }
            const auto error = addPoint(to);
            if (error != MaterializationError::None)
                return error;
        }
        else if (from_in_range)
        {
            const auto error = addPoint(interpolate(from, to, maximum_duration));
            if (error != MaterializationError::None)
                return error;
            const auto flush_error = flush();
            if (flush_error != MaterializationError::None)
                return flush_error;
        }
        else if (to_in_range)
        {
            const auto error = addPoint(interpolate(from, to, maximum_duration));
            if (error != MaterializationError::None)
                return error;
            const auto to_error = addPoint(to);
            if (to_error != MaterializationError::None)
                return to_error;
        }
    }
    return flush();
}

MaterializationError
materializeGeometry(const datafacade::BaseDataFacade &facade,
                    const DirectedLabel &label,
                    GeometryPosition first,
                    GeometryPosition last,
                    const std::optional<util::Coordinate> &first_coordinate_override,
                    const std::optional<util::Coordinate> &last_coordinate_override,
                    const std::optional<EdgeDuration> &first_to_segment_end_duration,
                    const std::optional<EdgeDuration> &last_from_segment_start_duration,
                    const std::optional<EdgeWeight> &first_to_segment_end_weight,
                    const std::optional<EdgeWeight> &last_from_segment_start_weight,
                    const MaterializationOptions &options,
                    std::size_t &materialized_geometry_points,
                    std::size_t &output_points,
                    std::vector<WeightedPolyline> &output)
{
    GeometryData geometry;
    const auto read_error =
        readGeometry(facade, label, options.limits, materialized_geometry_points, geometry);
    if (read_error != MaterializationError::None)
        return read_error;

    InternalPolyline polyline;
    const auto geometry_error = buildClippedGeometry(geometry,
                                                     first,
                                                     last,
                                                     first_coordinate_override,
                                                     last_coordinate_override,
                                                     first_to_segment_end_duration,
                                                     last_from_segment_start_duration,
                                                     first_to_segment_end_weight,
                                                     last_from_segment_start_weight,
                                                     label,
                                                     polyline);
    if (geometry_error != MaterializationError::None)
        return geometry_error;

    const auto geometry_id = facade.GetGeometryIndex(label.node).id;
    return appendClippedPolyline(polyline,
                                 from_alias<long double>(options.maximum_duration),
                                 geometry_id,
                                 options.clip_to_maximum_duration,
                                 options.limits,
                                 output,
                                 output_points);
}

MaterializationError materializeApproach(const WeightedApproach &approach,
                                         const MaterializationOptions &options,
                                         std::size_t &materialized_geometry_points,
                                         std::size_t &output_points,
                                         std::vector<WeightedPolyline> &output)
{
    if (approach.empty())
        return MaterializationError::InvalidGeometry;
    if (!hasCapacity(materialized_geometry_points,
                     approach.size(),
                     options.limits.maximum_materialized_geometry_points))
        return MaterializationError::BudgetExceeded;

    InternalPolyline polyline;
    polyline.reserve(approach.size());
    for (const auto &point : approach)
    {
        const auto coordinate_error = validateCoordinate(point.coordinate);
        if (coordinate_error != MaterializationError::None)
            return coordinate_error;
        if (!isValidNonnegativeDuration(point.duration))
            return MaterializationError::InvalidLabel;
        if (!isValidLabelWeight(point.weight))
            return MaterializationError::InvalidLabel;
        polyline.push_back({point.coordinate,
                            from_alias<long double>(point.weight),
                            from_alias<long double>(point.duration)});
    }
    materialized_geometry_points += approach.size();

    for (std::size_t index = 1; index < polyline.size(); ++index)
    {
        const auto error =
            validateSegment(polyline[index - 1].coordinate, polyline[index].coordinate);
        if (error != MaterializationError::None)
            return error;
    }

    return appendClippedPolyline(polyline,
                                 from_alias<long double>(options.maximum_duration),
                                 std::nullopt,
                                 true,
                                 options.limits,
                                 output,
                                 output_points);
}

bool lessLabel(const DirectedLabel &left, const DirectedLabel &right)
{
    if (left.node != right.node)
        return left.node < right.node;
    if (left.anchor != right.anchor)
        return left.anchor < right.anchor;
    return std::tie(left.weight, left.duration) < std::tie(right.weight, right.duration);
}

bool sameLabelGeometry(const DirectedLabel &left, const DirectedLabel &right)
{ return left.node == right.node && left.anchor == right.anchor; }

} // namespace

MaterializationResult
materializeWeightedPolylines(const datafacade::BaseDataFacade &facade,
                             const std::span<const DirectedLabel> labels,
                             const std::span<const GeometryClip> clips,
                             const std::span<const WeightedApproach> approaches,
                             const MaterializationOptions &options)
{
    MaterializationResult result;
    const auto fail = [&](const MaterializationError error)
    {
        result.polylines.clear();
        result.error = error;
        return result;
    };

    if (!hasValidOptions(options))
        return fail(MaterializationError::InvalidOptions);

    const auto maximum_fragments = options.limits.maximum_input_fragments;
    if (!hasCapacity(labels.size(), clips.size(), maximum_fragments) ||
        !hasCapacity(labels.size() + clips.size(), approaches.size(), maximum_fragments))
        return fail(MaterializationError::BudgetExceeded);

    std::vector<DirectedLabel> complete_labels(labels.begin(), labels.end());
    for (const auto &label : complete_labels)
    {
        if (label.node == SPECIAL_NODEID || !isValidLabelDuration(label.duration) ||
            !isValidAnchor(label.anchor))
            return fail(MaterializationError::InvalidLabel);
    }
    std::sort(complete_labels.begin(), complete_labels.end(), lessLabel);
    complete_labels.erase(
        std::unique(complete_labels.begin(), complete_labels.end(), sameLabelGeometry),
        complete_labels.end());

    std::size_t materialized_geometry_points = 0;
    std::size_t output_points = 0;
    const auto segmentCount = [&](const DirectedLabel &label) -> std::size_t
    {
        const auto geometry_index = facade.GetGeometryIndex(label.node);
        if (geometry_index.id == SPECIAL_GEOMETRYID)
            return 0;
        if (geometry_index.forward)
            return facade.GetUncompressedForwardDurations(geometry_index.id).size();
        return facade.GetUncompressedReverseDurations(geometry_index.id).size();
    };

    for (const auto &label : complete_labels)
    {
        const auto segment_count = segmentCount(label);
        if (segment_count == 0)
            return fail(MaterializationError::InvalidGeometry);
        const auto error = materializeGeometry(facade,
                                               label,
                                               {0, 0.},
                                               {segment_count - 1, 1.},
                                               std::nullopt,
                                               std::nullopt,
                                               std::nullopt,
                                               std::nullopt,
                                               std::nullopt,
                                               std::nullopt,
                                               options,
                                               materialized_geometry_points,
                                               output_points,
                                               result.polylines);
        if (error != MaterializationError::None)
            return fail(error);
    }

    for (const auto &clip : clips)
    {
        const auto error = materializeGeometry(facade,
                                               clip.label,
                                               clip.first,
                                               clip.last,
                                               clip.first_coordinate,
                                               clip.last_coordinate,
                                               clip.first_to_segment_end_duration,
                                               clip.last_from_segment_start_duration,
                                               clip.first_to_segment_end_weight,
                                               clip.last_from_segment_start_weight,
                                               options,
                                               materialized_geometry_points,
                                               output_points,
                                               result.polylines);
        if (error != MaterializationError::None)
            return fail(error);
    }

    for (const auto &approach : approaches)
    {
        const auto error = materializeApproach(
            approach, options, materialized_geometry_points, output_points, result.polylines);
        if (error != MaterializationError::None)
            return fail(error);
    }

    return result;
}

} // namespace osrm::engine::isochrone
