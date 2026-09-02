#ifndef OSRM_EXTRACTOR_AREA_SIMPLIFY_HPP
#define OSRM_EXTRACTOR_AREA_SIMPLIFY_HPP

#include "extractor/area/typedefs.hpp"

namespace osrm::extractor::area
{

/**
 * @brief Drop the vertices of an area that carry no shape, by Visvalingam-Whyatt.
 *
 * An OSM plaza is drawn to be looked at, not to be routed across.  Its outline follows
 * kerbstones and planting beds at a resolution nothing downstream can use: Ile-de-France
 * has a pedestrian area with 2739 nodes around 228 by 209 metres, one every 30 cm.  Three
 * separate costs are paid per vertex, and all of them are paid per query rather than once:
 *
 *  * `SnapInsideOpenArea` asks the r-tree for the phantoms standing on each vertex the
 *    coordinate can see, one nearest-neighbour traversal each;
 *  * the engine's visibility graph is cubic in the vertex count, which is why
 *    `GEODESIC_MAX_VERTICES` has to stop at 256;
 *  * the mesher declines an area over `AreaMesher::max_vertices` obstacle vertices
 *    outright, and 153 of Ile-de-France's 1234 areas hit that.
 *
 * Visvalingam-Whyatt is the right shape of algorithm here because it ranks a vertex by the
 * *area* of the triangle it makes with its neighbours, so it removes what does not change
 * the polygon rather than what is merely close to a line.  Douglas-Peucker, which the
 * repository already has for route geometry, answers a different question -- how far a
 * point strays from a chord -- and on a closed ring it will happily shave a whole shallow
 * bay that a traveller has to walk round.
 *
 * The threshold is an area in square metres, so it says directly how much of the plaza a
 * dropped vertex is allowed to add or remove.  A vertex is dropped when its triangle is
 * smaller than that.
 *
 * @param poly       the polygon, outer ring first
 * @param threshold  effective area, in square metres, below which a vertex is dropped;
 *                   zero or less returns @p poly unchanged
 * @param keep       vertices that must survive whatever their effective area.  Entry
 *                   points go here: an entrance that is simplified away leaves the area
 *                   unreachable from the way that met it, which is not a loss of detail
 *                   but a loss of the graph.
 *
 * A ring is never reduced below three vertices, and a ring that would collapse is left as
 * it is.  Nothing here can turn a valid polygon into one with fewer rings.
 */
OsmiumPolygon simplify(const OsmiumPolygon &poly, double threshold, const NodeRefSet &keep);

/**
 * @brief The effective area of one vertex, in square metres.
 *
 * The triangle @p before, @p at, @p after, measured on an equirectangular projection about
 * the triangle's own latitude.  Exposed for testing.
 */
double effective_area(const osmium::NodeRef &before,
                      const osmium::NodeRef &at,
                      const osmium::NodeRef &after);

} // namespace osrm::extractor::area

#endif // OSRM_EXTRACTOR_AREA_SIMPLIFY_HPP
