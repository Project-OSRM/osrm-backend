#ifndef OSRM_ENGINE_ISOCHRONE_WEIGHTED_POLYLINE_MATERIALIZATION_HPP
#define OSRM_ENGINE_ISOCHRONE_WEIGHTED_POLYLINE_MATERIALIZATION_HPP

#include "engine/datafacade/datafacade_base.hpp"
#include "engine/isochrone/weighted_polyline_grid.hpp"

#include "util/typedefs.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace osrm::engine::isochrone
{

// The duration belongs to the corresponding endpoint of the geometry
// fragment.  A first-anchored fragment increases in legal geometry order; a
// last-anchored fragment decreases in that order.
enum class DurationAnchor
{
    FirstCoordinate,
    LastCoordinate
};

struct DirectedLabel
{
    NodeID node;
    EdgeDuration duration;
    DurationAnchor anchor;
};

struct GeometryPosition
{
    std::size_t segment_index;
    double fraction;
};

struct GeometryClip
{
    DirectedLabel label;
    GeometryPosition first;
    GeometryPosition last;
    // When present, an endpoint coordinate comes from snapping rather than
    // interpolating the stored geometry fraction.
    std::optional<util::Coordinate> first_coordinate = std::nullopt;
    std::optional<util::Coordinate> last_coordinate = std::nullopt;
    // Exact legal-travel costs for the partially covered endpoint segments.
    // The first value is first -> end(first.segment_index); the last is
    // start(last.segment_index) -> last.  They must not be inferred from a
    // geometric fraction, because phantom partial durations are integral.
    std::optional<EdgeDuration> first_to_segment_end_duration = std::nullopt;
    std::optional<EdgeDuration> last_from_segment_start_duration = std::nullopt;
};

struct WeightedApproachPoint
{
    util::Coordinate coordinate;
    EdgeDuration duration;
};

using WeightedApproach = std::vector<WeightedApproachPoint>;

struct MaterializationLimits
{
    std::size_t maximum_input_fragments = 200'000;
    std::size_t maximum_materialized_geometry_points = 5'000'000;
    std::size_t maximum_output_polylines = 200'000;
    std::size_t maximum_output_points = 5'000'000;
};

struct MaterializationOptions
{
    EdgeDuration maximum_duration{0};
    MaterializationLimits limits;
};

enum class MaterializationError
{
    None,
    InvalidOptions,
    InvalidLabel,
    InvalidGeometry,
    InvalidSegmentDuration,
    InvalidCoordinate,
    RequiresLongitudeWrap,
    TouchesPole,
    BudgetExceeded
};

struct MaterializationResult
{
    std::vector<WeightedPolyline> polylines;
    MaterializationError error = MaterializationError::None;
};

// Materializes complete edge-based-node labels, explicitly clipped fragments,
// and the input-location-to-phantom approaches.  Arithmetic and clipping use
// internal EdgeDuration units; output durations are converted to seconds.
// Complete labels are deterministically deduplicated by (node, anchor),
// retaining the lowest duration. Explicit clips and
// approaches are intentionally not deduplicated because they can represent
// distinct source or loop-reentry coverage.
MaterializationResult materializeWeightedPolylines(const datafacade::BaseDataFacade &facade,
                                                   std::span<const DirectedLabel> labels,
                                                   std::span<const GeometryClip> clips,
                                                   std::span<const WeightedApproach> approaches,
                                                   const MaterializationOptions &options);

} // namespace osrm::engine::isochrone

#endif // OSRM_ENGINE_ISOCHRONE_WEIGHTED_POLYLINE_MATERIALIZATION_HPP
