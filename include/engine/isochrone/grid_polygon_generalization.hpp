#ifndef OSRM_ENGINE_ISOCHRONE_GRID_POLYGON_GENERALIZATION_HPP
#define OSRM_ENGINE_ISOCHRONE_GRID_POLYGON_GENERALIZATION_HPP

#include "engine/isochrone/cell_contours.hpp"

#include <vector>

namespace osrm::engine::isochrone
{

// Simplifies closed contour rings in raster-grid units. Shared vertices are retained and the
// complete candidate is checked for ring crossings and changed nesting. Invalid candidates and
// bounded-work fallbacks leave polygons byte-for-byte unchanged and return false.
bool generalizeGridPolygons(std::vector<GridPolygon> &polygons, double tolerance);

} // namespace osrm::engine::isochrone

#endif // OSRM_ENGINE_ISOCHRONE_GRID_POLYGON_GENERALIZATION_HPP
