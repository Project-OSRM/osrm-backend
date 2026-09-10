#ifndef OSRM_ENGINE_ISOCHRONE_CELL_CONTOURS_HPP
#define OSRM_ENGINE_ISOCHRONE_CELL_CONTOURS_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace osrm::engine::isochrone
{

struct GridPoint
{
    // Coordinates are in raster-cell units.  Building boundaries from cell
    // vertices keeps the polygonization exact without floating-point topology
    // predicates.
    std::int64_t x;
    std::int64_t y;

    bool operator==(const GridPoint &) const = default;
};

using GridRing = std::vector<GridPoint>;

struct GridPolygon
{
    GridRing outer;
    std::vector<GridRing> holes;
};

// Returns the boundary of the cells whose value is at most cutoff. Rings are
// closed. Outer rings are counter-clockwise and holes are clockwise.
std::vector<GridPolygon> buildCellContours(std::span<const double> values,
                                           std::size_t width,
                                           std::size_t height,
                                           double cutoff);

// As above, but returns std::nullopt when the exact number of coordinates in
// the simplified closed rings would exceed maximum_coordinates.
std::optional<std::vector<GridPolygon>> buildCellContours(std::span<const double> values,
                                                          std::size_t width,
                                                          std::size_t height,
                                                          double cutoff,
                                                          std::size_t maximum_coordinates);

} // namespace osrm::engine::isochrone

#endif // OSRM_ENGINE_ISOCHRONE_CELL_CONTOURS_HPP
