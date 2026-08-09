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
 * Solving an area means building the visibility graph among its vertices, which costs
 * O(n² log n).  Measured by src/benchmarks/area_geodesic.cpp, an area of 40 vertices
 * takes about half a millisecond against a budget of one, and one of 68 takes two
 * milliseconds.  Areas that occur are far smaller than either -- Monaco's largest has 24
 * -- so the limit is a guard against the pathological rather than a real constraint.
 */
inline constexpr std::size_t GEODESIC_MAX_VERTICES = 40;

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
