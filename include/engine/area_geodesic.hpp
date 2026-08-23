#ifndef OSRM_ENGINE_AREA_GEODESIC_HPP
#define OSRM_ENGINE_AREA_GEODESIC_HPP

#include "util/coordinate.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace osrm::engine::area
{

/**
 * @brief The shortest way from one point inside an open area to another.
 *
 * A shortest path inside a polygon is a taut string: it runs straight until something
 * gets in the way, and bends only at a vertex of the area.  So it is fully described by
 * the vertices it turns at, which is usually none at all -- on a plaza with nothing in it
 * every pair of points can see each other and the answer is the straight line.
 */
struct Geodesic
{
    //! Length along the path, in metres.
    double length = 0.0;
    //! The vertices the path turns at, in order.  Empty when the two points are
    //! mutually visible, which is the common case.
    std::vector<util::Coordinate> bends;
};

/**
 * @brief How far the geodesic solver will go before giving up.
 *
 * Solving an area means building the visibility graph among its vertices.  solve() runs
 * visible_vertices() from each vertex in turn and that is itself quadratic, so the build
 * is cubic in practice.  Measured by src/benchmarks/area_geodesic.cpp:
 *
 *     vertices     40     104     200     260     404     580
 *     build      0.8ms   9.0ms    50ms   101ms   356ms   996ms
 *
 * The cost lands on the first request to reach an area and is then cached, so it is a
 * latency spike rather than a throughput cost.
 *
 * Giving up is not free either.  An area the solver declines falls back to the mesh, and
 * the mesh holds only the shortest-path trees rooted at the entry points, so a journey
 * with both ends inside the area walks out towards an entry point and back instead of
 * going straight.  On the Notre-Dame parvis, 120 vertices, 226 of 386 sampled crossings
 * whose straight line was entirely clear came back more than a tenth longer than it, the
 * worst at 2.16 times.  All 386 are within 1.003 once the area is solved.
 *
 * So this is a real constraint, not a guard against the pathological.  In Ile-de-France
 * the median pedestrian area has 22 vertices but the 95th percentile has 119, and 40
 * declines a quarter of them.  256 covers 98.7%, and what it still declines are the
 * genuinely large ones -- theme parks and campuses, up to 2821 vertices -- where a cubic
 * build cannot be paid at any point in a request.
 *
 * Raising this further wants the build to get cheaper first.  The extractor already
 * solves the same problem with a rotational sweep at O(n² log n), which is the obvious
 * next step and would move this number rather than being traded against it.
 */
inline constexpr std::size_t GEODESIC_MAX_VERTICES = 256;

/** How many areas' visibility graphs to keep, per thread. */
inline constexpr std::size_t GEODESIC_CACHE_SIZE = 32;

/**
 * @brief Find the geodesic between two points of one open area.
 *
 * The visibility graph of an area is built on first use and kept, because a plaza that is
 * asked about once tends to be asked about again.  The cache is per thread, following
 * SearchEngineData, so no request waits on another.
 *
 * @param dataset     the facade's checksum, so a reloaded dataset does not reuse a graph
 * @param area        identifies the area within that dataset
 * @param rings       its rings, outer first, as the facade hands them over
 * @param from, to    the two points, which must both lie inside
 *
 * @return the geodesic, or nothing if either point is outside the area, if no path runs
 *         between them, or if the area is larger than GEODESIC_MAX_VERTICES.  Nothing
 *         always means "route this the ordinary way", never "there is no route".
 */
std::optional<Geodesic>
geodesic_between(std::uint32_t dataset,
                 std::uint64_t area,
                 const std::vector<std::span<const util::Coordinate>> &rings,
                 util::Coordinate from,
                 util::Coordinate to);

/** Drop every cached graph on this thread.  For tests, and for a dataset swap. */
void forget_cached_geodesics();

/** How many graphs this thread is holding.  For tests. */
std::size_t cached_geodesic_count();

} // namespace osrm::engine::area

#endif // OSRM_ENGINE_AREA_GEODESIC_HPP
