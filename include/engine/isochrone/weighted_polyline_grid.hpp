#ifndef OSRM_ENGINE_ISOCHRONE_WEIGHTED_POLYLINE_GRID_HPP
#define OSRM_ENGINE_ISOCHRONE_WEIGHTED_POLYLINE_GRID_HPP

#include "util/coordinate.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace osrm::engine::isochrone
{

struct WeightedPolylinePoint
{
    util::Coordinate coordinate;
    double duration;
};

using WeightedPolyline = std::vector<WeightedPolylinePoint>;
using CoordinateRing = std::vector<util::Coordinate>;

struct CoordinatePolygon
{
    CoordinateRing outer;
    std::vector<CoordinateRing> holes;
};

struct WeightedGridOptions
{
    // Metres in a local tangent-plane projection centered on the request
    // source when one is supplied (otherwise the input bounds center).
    // The implementation rejects sizes that cannot survive OSRM's fixed
    // 1e-6-degree output precision.
    double cell_size = 100.;
    std::size_t maximum_cell_count = 1'000'000;
    // Bounds work for line rasterization independently from grid allocation.
    // A sparse set of long polylines can cross the same bounded grid many times.
    std::size_t maximum_rasterization_steps = 1'000'000;
};

struct WeightedGrid
{
    // Geographic reference point for the local tangent-plane projection.
    // These remain first to preserve existing aggregate initialization.
    util::FloatLongitude reference_lon{0.};
    util::FloatLatitude reference_lat{0.};
    // Metres in the local tangent-plane projection.
    double cell_size = 0.;
    std::size_t width = 0;
    std::size_t height = 0;
    std::vector<double> values;
    // Metre coordinates of grid cell (0, 0) in the local projection.
    double origin_x = 0.;
    double origin_y = 0.;

    std::vector<CoordinatePolygon> buildContours(double cutoff) const;
    std::optional<std::vector<CoordinatePolygon>>
    buildContours(double cutoff, std::size_t maximum_coordinates) const;
};

enum class RasterizationError
{
    None,
    InvalidOptions,
    TooBig,
    TouchesLongitudeBoundary,
    TouchesPole
};

struct RasterizationResult
{
    std::optional<WeightedGrid> grid;
    RasterizationError error = RasterizationError::None;
};

// Rasterizes each finite duration point and each segment between adjacent finite
// duration points. A cell stores the lowest linearly interpolated duration that
// reaches it. Returns a typed error rather than silently reducing resolution
// when the fixed-size grid cannot represent the request. Cell sizes below OSRM
// coordinate precision, non-finite sizes, and nonpositive resource limits
// return InvalidOptions.  Callers that know the request source should supply
// it as projection_center to keep raster-cell alignment stable as the extent
// grows; the default uses the input bounds center.
RasterizationResult
rasterizeWeightedPolylines(std::span<const WeightedPolyline> polylines,
                           const WeightedGridOptions &options = {},
                           std::optional<util::Coordinate> projection_center = std::nullopt);

} // namespace osrm::engine::isochrone

#endif // OSRM_ENGINE_ISOCHRONE_WEIGHTED_POLYLINE_GRID_HPP
