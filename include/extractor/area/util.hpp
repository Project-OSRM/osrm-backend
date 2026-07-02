#ifndef OSRM_EXTRACTOR_AREA_UTIL_HPP
#define OSRM_EXTRACTOR_AREA_UTIL_HPP

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <cmath>
#include <functional>
#include <ranges>

namespace osrm::extractor::area
{
namespace bg = boost::geometry;

/**
 * @brief Return twice the area enclosed by the three given points.
 *
 * The result is:
 *
 * - positive if a, b, c form a counter-clockwise circuit,
 * - negative if they form a clockwise circuit, and
 * - 0 if they are collinear.
 */
template <class TPoint> double area2(const TPoint *a, const TPoint *b, const TPoint *c)
{
    return ((bg::get<0>(*b) - bg::get<0>(*a)) * (bg::get<1>(*c) - bg::get<1>(*a))) -
           ((bg::get<0>(*c) - bg::get<0>(*a)) * (bg::get<1>(*b) - bg::get<1>(*a)));
}
/**
 * @brief Return true if the point c is strictly to the left of the line formed by
 * points a and b.
 */
template <class TPoint> bool left(const TPoint *a, const TPoint *b, const TPoint *c)
{
    return area2(a, b, c) > 0;
}
/**
 * @brief Return true if the point c is to the left of the line formed by points a and b
 * or on that line.
 */
template <class TPoint> bool leftOrOn(const TPoint *a, const TPoint *b, const TPoint *c)
{
    return area2(a, b, c) >= 0;
}
/**
 * @brief Return true if the point c is strictly to the right of the line formed by
 * points a and b.
 */
template <class TPoint> bool right(const TPoint *a, const TPoint *b, const TPoint *c)
{
    return area2(a, b, c) < 0;
}
/**
 * @brief Return true if the point c is to the right of the line formed by points a and
 * b or on that line.
 */
template <class TPoint> bool rightOrOn(const TPoint *a, const TPoint *b, const TPoint *c)
{
    return area2(a, b, c) <= 0;
}
/**
 * @brief Return true if points a, b, and c are collinear.
 */
template <class TPoint> bool collinear(const TPoint *a, const TPoint *b, const TPoint *c)
{
    return area2(a, b, c) == 0;
}

/**
 * @brief Calculate segment-segment or ray-segment intersection.
 *
 * If ray_segment is false, the function returns true if the segment between a and
 * b intersects the segment between c and d.
 *
 * If ray_segment is true, the function returns true if a ray from a through b intersects
 * the segment between c and d.
 *
 * The intersection point is returned in i if i is not null.
 *
 * The intersection test does not include endpoints. To include endpoints, use @ref
 * intersect_closed().
 *
 * Adapted from: *O'Rourke. Computational Geometry in C. 2nd ed. Cambridge 1998* Code 7.2
 *
 * @tparam TPoint The point type. See: boost::geometry.
 * @tparam TComp The comparison function to use.
 * @param a The first point of the first segment or the origin of the ray.
 * @param b The second point of the first segment or the ray.
 * @param c The first point of the second segment.
 * @param d The second point of the second segment.
 * @param i Returns the intersection point if not `nullptr`.
 * @param ray_segment If true calculate the ray-segment intersection.
 * @return True if an intersection was found.
 */
template <class TPoint, typename TComp = std::less_equal<>>
bool intersect(const TPoint *a,
               const TPoint *b,
               const TPoint *c,
               const TPoint *d,
               TPoint *i = nullptr,
               bool ray_segment = false)
{
    using Coord = double;
    Coord num, denom;
    auto comp = TComp{};

    // clang-format off

    denom = bg::get<0>(*a) * (Coord)(bg::get<1>(*d) - bg::get<1>(*c)) +
            bg::get<0>(*b) * (Coord)(bg::get<1>(*c) - bg::get<1>(*d)) +
            bg::get<0>(*d) * (Coord)(bg::get<1>(*b) - bg::get<1>(*a)) +
            bg::get<0>(*c) * (Coord)(bg::get<1>(*a) - bg::get<1>(*b));

    // if denom is zero the ray and the segment are parallel
    if (denom == 0) return false;

    num   = bg::get<0>(*a) * (Coord)(bg::get<1>(*d) - bg::get<1>(*c)) +
            bg::get<0>(*c) * (Coord)(bg::get<1>(*a) - bg::get<1>(*d)) +
            bg::get<0>(*d) * (Coord)(bg::get<1>(*c) - bg::get<1>(*a));

    // the parameter of the intersection point: a + s * (b - a)
    double s = num / denom;

    if (comp(s, 0.0))
        return false;

    if (!ray_segment && comp(1.0, s))
        return false;

    num = -((bg::get<0>(*a) * (Coord)(bg::get<1>(*c) - bg::get<1>(*b))) +
            (bg::get<0>(*b) * (Coord)(bg::get<1>(*a) - bg::get<1>(*c))) +
            (bg::get<0>(*c) * (Coord)(bg::get<1>(*b) - bg::get<1>(*a))));

    // clang-format on

    // the parameter of the intersection point: c + t * (d - c)
    double t = num / denom;

    if (comp(t, 0.0) || comp(1.0, t))
        return false;

    if (i)
    {
        bg::set<0>(*i, bg::get<0>(*a) + (s * (bg::get<0>(*b) - bg::get<0>(*a))));
        bg::set<1>(*i, bg::get<1>(*a) + (s * (bg::get<1>(*b) - bg::get<1>(*a))));
    }
    return true;
};

/**
 * @brief Same as @ref intersect() but includes segment endpoint.
 */
template <class TPoint, typename comp = std::less_equal<double>>
bool intersect_closed(const TPoint *a,
                      const TPoint *b,
                      const TPoint *c,
                      const TPoint *d,
                      TPoint *i = nullptr,
                      bool ray_segment = false)
{
    return intersect<TPoint, std::less<>>(a, b, c, d, i, ray_segment);
}

/**
 * @brief Return true if a -> b lies in the closed cone clockwise determined by a -> a0
 * and a -> a1.
 *
 * Adapted from: *O'Rourke. Computational Geometry in C. 2nd ed. Cambridge 1998* Code 1.11
 */
template <class TPoint>
bool in_closed_cone(const TPoint *a0, const TPoint *a, const TPoint *a1, const TPoint *b)
{
    if (leftOrOn(a, a1, a0))
        // angle <= 180°
        return leftOrOn(a, b, a0) && leftOrOn(b, a, a1);
    return !(leftOrOn(a, b, a1) && leftOrOn(b, a, a0));
}

/**
 * @brief Return true if a -> b lies in the open cone clockwise determined by a -> a0
 * and a -> a1.
 *
 * Adapted from: *O'Rourke. Computational Geometry in C. 2nd ed. Cambridge 1998* Code 1.11
 */
template <class TPoint>
bool in_open_cone(const TPoint *a0, const TPoint *a, const TPoint *a1, const TPoint *b)
{
    if (leftOrOn(a, a1, a0))
        // angle <= 180°
        return left(a, b, a0) && left(b, a, a1);
    return !(left(a, b, a1) && left(b, a, a0));
}

/**
 * @brief Call `function` for each pair in a ring.
 *
 * Note: `function` will also be called on the pair (last, first).
 *
 * @tparam range_t The ring type
 * @tparam fun_t   The function type, void(range_t, range_t)
 * @param range    The ring
 * @param function The function to call
 */
template <std::ranges::sized_range range_t, typename fun_t>
void for_each_pair_in_ring(range_t &range, fun_t function)
{
    size_t size = range.size();
    for (size_t i = 0; i < size; ++i)
    {
        function(range[i], range[(i + 1) % size]);
    }
}

/**
 * @brief Call `function` for each triplet in a ring.
 *
 * Note: `function` will also be called on the triplets (n-2, n-1, 0) and (n-1, 0, 1).
 *
 * @tparam range_t The ring type
 * @tparam fun_t   The function type, void(range_t, range_t, range_t)
 * @param range    The ring
 * @param function The function to call
 */
template <std::ranges::sized_range range_t, typename fun_t>
void for_each_triplet_in_ring(range_t &range, fun_t function)
{
    size_t size = range.size();
    auto mod = [size](size_t i) { return (i + size) % size; };
    for (size_t i = 0; i < size; ++i)
    {
        function(range[mod(i - 1)], range[i], range[mod(i + 1)]);
    }
}

/**
 * @brief Call `function` for each ring in the polygon.
 *
 * @tparam polygon_t The polygon type
 * @tparam fun_t     The function type, void(polygon_t::ring_t)
 * @param poly       The polygon
 * @param function   The function
 */
template <class polygon_t, typename fun_t> void for_each_ring(polygon_t &poly, fun_t function)
{
    function(poly.outer());
    for (auto &inner : poly.inners())
    {
        function(inner);
    }
}

} // namespace osrm::extractor::area

#endif
