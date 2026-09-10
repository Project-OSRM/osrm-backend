#ifndef OSRM_ENGINE_ISOCHRONE_GRID_POLYGON_DENOISING_HPP
#define OSRM_ENGINE_ISOCHRONE_GRID_POLYGON_DENOISING_HPP

#include "engine/isochrone/cell_contours.hpp"

#include <vector>

namespace osrm::engine::isochrone
{

// Removes complete components and holes whose absolute raw-grid area, divided by the largest
// outer-ring area, is below threshold. A zero threshold is an exact no-op. Invalid thresholds and
// unclosed or zero-area rings are left unchanged and return false; callers provide valid topology.
bool denoiseGridPolygons(std::vector<GridPolygon> &polygons, double threshold);

} // namespace osrm::engine::isochrone

#endif // OSRM_ENGINE_ISOCHRONE_GRID_POLYGON_DENOISING_HPP
