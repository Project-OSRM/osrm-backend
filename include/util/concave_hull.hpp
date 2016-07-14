#ifndef OSRM_CONCAVE_HULL_HPP
#define OSRM_CONCAVE_HULL_HPP

#include <engine/plugins/isochrone.hpp>
#include <util/coordinate.hpp>
#include <util/coordinate_calculation.hpp>
#include <util/simple_logger.hpp>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numeric>
#include <vector>

namespace osrm
{
namespace util
{

// Compute the dot product AB ⋅ BC
double dot(const engine::plugins::IsochroneNode &A,
           const engine::plugins::IsochroneNode &B,
           const engine::plugins::IsochroneNode &C)
{
    const double Ax = static_cast<double>(util::toFloating(A.node.lon)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;
    const double Ay = static_cast<double>(util::toFloating(A.node.lat)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;
    const double Bx = static_cast<double>(util::toFloating(B.node.lon)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;
    const double By = static_cast<double>(util::toFloating(B.node.lat)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;
    const double Cx = static_cast<double>(util::toFloating(C.node.lon)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;
    const double Cy = static_cast<double>(util::toFloating(C.node.lat)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;

    const double Axx =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Ax) * std::cos(Ay);
    const double Ayy =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Ax) * std::sin(Ay);
    const double Bxx =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Bx) * std::cos(By);
    const double Byy =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Bx) * std::sin(By);
    const double Cxx =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Cx) * std::cos(Cy);
    const double Cyy =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Cx) * std::sin(Cy);
    double AB[2];
    double BC[2];
    AB[0] = Bxx - Axx;
    AB[1] = Byy - Ayy;
    BC[0] = Cxx - Bxx;
    BC[1] = Cyy - Byy;
    const double dot = AB[0] * BC[0] + AB[1] * BC[1];
    return dot;
}
// Compute the cross product AB x AC
double cross(const engine::plugins::IsochroneNode &A,
             const engine::plugins::IsochroneNode &B,
             const engine::plugins::IsochroneNode &C)
{
    const double Ax = static_cast<double>(util::toFloating(A.node.lon)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;
    const double Ay = static_cast<double>(util::toFloating(A.node.lat)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;
    const double Bx = static_cast<double>(util::toFloating(B.node.lon)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;
    const double By = static_cast<double>(util::toFloating(B.node.lat)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;
    const double Cx = static_cast<double>(util::toFloating(C.node.lon)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;
    const double Cy = static_cast<double>(util::toFloating(C.node.lat)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;

    const double Axx =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Ax) * std::cos(Ay);
    const double Ayy =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Ax) * std::sin(Ay);
    const double Bxx =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Bx) * std::cos(By);
    const double Byy =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Bx) * std::sin(By);
    const double Cxx =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Cx) * std::cos(Cy);
    const double Cyy =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Cx) * std::sin(Cy);
    double AB[2];
    double AC[2];
    AB[0] = Bxx - Axx;
    AB[1] = Byy - Ayy;
    AC[0] = Cxx - Axx;
    AC[1] = Cyy - Ayy;
    const double cross = AB[0] * AC[1] - AB[1] * AC[0];
    return cross;
}
// Compute the distance from A to B
double distance(const engine::plugins::IsochroneNode &A, const engine::plugins::IsochroneNode &B)
{
    const double Ax = static_cast<double>(util::toFloating(A.node.lon)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;
    const double Ay = static_cast<double>(util::toFloating(A.node.lat)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;
    const double Bx = static_cast<double>(util::toFloating(B.node.lon)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;
    const double By = static_cast<double>(util::toFloating(B.node.lat)) *
                      util::coordinate_calculation::detail::DEGREE_TO_RAD;

    const double Axx =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Ax) * std::cos(Ay);
    const double Ayy =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Ax) * std::sin(Ay);
    const double Bxx =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Bx) * std::cos(By);
    const double Byy =
        util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Bx) * std::sin(By);
    const double d1 = Axx - Bxx;
    const double d2 = Ayy - Byy;
    return std::sqrt(d1 * d1 + d2 * d2);
}

// Compute the distance from AB to C
// if isSegment is true, AB is a segment, not a line.
double linePointDist(const engine::plugins::IsochroneNode &A,
                     const engine::plugins::IsochroneNode &B,
                     const engine::plugins::IsochroneNode &C,
                     bool isSegment)
{
    const double dist = cross(A, B, C) / distance(A, B);
    if (isSegment)
    {
        const double dot1 = dot(A, B, C);
        if (dot1 > 0)
        {
            return distance(B, C);
        }
        const double dot2 = dot(B, A, C);
        if (dot2 > 0)
        {
            return distance(A, C);
        }
    }
    return std::abs(dist);
}
double distanceToEdge(const engine::plugins::IsochroneNode &p,
                      const engine::plugins::IsochroneNode &e1,
                      const engine::plugins::IsochroneNode &e2)
{
    const std::uint64_t pX = static_cast<std::int32_t>(p.node.lon);
    const std::uint64_t pY = static_cast<std::int32_t>(p.node.lat);
    const std::uint64_t vX = static_cast<std::int32_t>(e1.node.lon);
    const std::uint64_t vY = static_cast<std::int32_t>(e1.node.lat);
    const std::uint64_t wX = static_cast<std::int32_t>(e2.node.lon);
    const std::uint64_t wY = static_cast<std::int32_t>(e2.node.lat);

    const double distance = std::abs((wX - vX) * (vY - pY) - (vX - pX) * (wY - vY)) /
                            std::sqrt(std::pow(wX - vX, 2) + std::pow(wY - vY, 2));
    return distance;
}


std::vector<engine::plugins::IsochroneNode>
concavehull(std::vector<engine::plugins::IsochroneNode> &convexhull,
            unsigned int threshold,
            std::vector<engine::plugins::IsochroneNode> &isochrone)
{

    //    std::vector<engine::plugins::IsochroneNode> concavehull(convexhull);
    std::vector<engine::plugins::IsochroneNode> concavehull = convexhull;
    concavehull.push_back(concavehull[0]);

    for (std::vector<engine::plugins::IsochroneNode>::iterator it = concavehull.begin();
         it != concavehull.end() - 1;
         ++it)
    {
        // find the nearest inner point pk ∈ G from the edge (ci1, ci2);
        // pk should not closer to neighbor edges of (ci1, ci2) than (ci1, ci2)
        engine::plugins::IsochroneNode n1 = *it;
        engine::plugins::IsochroneNode n2 = *std::next(it);
        engine::plugins::IsochroneNode pk;
        double d = std::numeric_limits<double>::max();
        double edgelength = distance(n1, n2);

        for (auto iso : isochrone)
        {
            if (std::find(concavehull.begin(), concavehull.end(), iso) != concavehull.end())
            {
                continue;
            }
            auto temp = linePointDist(n1, n2, iso, true);

            if (temp > edgelength)
            {
                continue;
            }
            if (d > temp)
            {
                d = temp;
                pk = iso;
            }
        }
        if ((edgelength / d) > threshold)
        {
            it = concavehull.insert(std::next(it), pk);
            it = std::prev(it);
            //            util::SimpleLogger().Write() << "List-Size: " << concavehull.size();
        }
    }
    util::SimpleLogger().Write() << "Convex-Size: " << convexhull.size();
    return concavehull;
}
}
}
#endif // OSRM_CONCAVE_HULL_HPP
