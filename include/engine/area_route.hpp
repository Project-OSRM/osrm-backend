#ifndef OSRM_ENGINE_AREA_ROUTE_HPP
#define OSRM_ENGINE_AREA_ROUTE_HPP

#include "engine/datafacade/datafacade_base.hpp"
#include "engine/internal_route_result.hpp"

#include "util/coordinate.hpp"

#include <vector>

namespace osrm::engine::area
{

/**
 * @brief Replace any leg that runs inside one open area with the geodesic, when shorter.
 *
 * Two coordinates on one plaza are a case the mesh cannot answer well.  It holds shortest
 * paths *out* of the area, so a journey that never leaves is routed by way of whichever
 * vertex both ends happen to share -- and where the two can simply see each other, which
 * on a plaza with nothing in it is every pair, the route leaves by a corner and comes
 * back.  Neither the pruned mesh nor the whole visibility graph fixes that; the fault is
 * that the two endpoints are joined to the graph but never to each other.
 *
 * So the answer is worked out when asked, from the polygon the engine already carries for
 * snapping: see engine/area_geodesic.hpp.  The route it produces is then written into the
 * leg directly, because it runs along lines of sight that are deliberately not edges of
 * the graph and so cannot be unpacked from one.
 *
 * The geodesic is taken whenever it can be computed, without comparing it against the leg
 * the search found: the two do not measure the same journey.  A routed leg runs between
 * the *snapped* points and leaves the walk to each of them out of its distance -- and when
 * both ends snap to one vertex it reports nothing at all.  The geodesic is the shortest
 * path between the two coordinates through the area, and a route that left the area to
 * come back could only beat it on ways faster than the area itself, which is not a case
 * that arises for a profile that meshes plazas.
 */
void useGeodesicWhereShorter(const datafacade::BaseDataFacade &facade,
                             InternalRouteResult &route,
                             const std::vector<PhantomNodeCandidates *> &waypoints);

/**
 * @brief Replace any cell of a table whose two coordinates lie in one open area.
 *
 * The same case as above, and for the same reason, but there is no route to write: a
 * table is numbers.  Durations are in deci-seconds and distances in metres, as the search
 * leaves them.  @p distances may be empty when none were asked for.
 *
 * @p sources and @p destinations map a row and a column onto @p coordinates; either may be
 * empty, which means every coordinate in order.
 */
void useGeodesicInTable(const datafacade::BaseDataFacade &facade,
                        const std::vector<util::Coordinate> &coordinates,
                        const std::vector<std::size_t> &sources,
                        const std::vector<std::size_t> &destinations,
                        std::vector<EdgeDuration> &durations,
                        std::vector<EdgeDistance> &distances);

} // namespace osrm::engine::area

#endif // OSRM_ENGINE_AREA_ROUTE_HPP
