#ifndef OSRM_CONCAVE_HULL_HPP
#define OSRM_CONCAVE_HULL_HPP

#include <engine/plugins/isochrone.hpp>
#include <util/coordinate.hpp>
#include <util/coordinate_calculation.hpp>
#include <util/simple_logger.hpp>

#include <algorithm>
#include <iterator>
#include <numeric>
#include <vector>

namespace osrm
{
namespace util
{

// Compute the dot product AB ⋅ BC
double dot(engine::plugins::IsochroneNode &A,
           engine::plugins::IsochroneNode &B,
           engine::plugins::IsochroneNode &C)
{
    double Ax = static_cast<double>(util::toFloating(A.node.lon)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;
    double Ay = static_cast<double>(util::toFloating(A.node.lat)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;
    double Bx = static_cast<double>(util::toFloating(B.node.lon)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;
    double By = static_cast<double>(util::toFloating(B.node.lat)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;
    double Cx = static_cast<double>(util::toFloating(C.node.lon)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;
    double Cy = static_cast<double>(util::toFloating(C.node.lat)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;

    double Axx = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Ax) * std::cos(Ay);
    double Ayy = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Ax) * std::sin(Ay);
    double Bxx = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Bx) * std::cos(By);
    double Byy = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Bx) * std::sin(By);
    double Cxx = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Cx) * std::cos(Cy);
    double Cyy = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Cx) * std::sin(Cy);
    auto AB = new double[2];
    auto BC = new double[2];
    AB[0] = Bxx - Axx;
    AB[1] = Byy - Ayy;
    BC[0] = Cxx - Bxx;
    BC[1] = Cyy - Byy;
    double dot = AB[0] * BC[0] + AB[1] * BC[1];
    return dot;
}
// Compute the cross product AB x AC
double cross(engine::plugins::IsochroneNode &A,
             engine::plugins::IsochroneNode &B,
             engine::plugins::IsochroneNode &C)
{
    //    double Ax = static_cast<double>(util::toFloating(A.node.lon));
    //    double Ay = static_cast<double>(util::toFloating(A.node.lat));
    //    double Bx = static_cast<double>(util::toFloating(B.node.lon));
    //    double By = static_cast<double>(util::toFloating(B.node.lat));
    //    double Cx = static_cast<double>(util::toFloating(C.node.lon));
    //    double Cy = static_cast<double>(util::toFloating(C.node.lat));

    double Ax = static_cast<double>(util::toFloating(A.node.lon)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;
    double Ay = static_cast<double>(util::toFloating(A.node.lat)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;
    double Bx = static_cast<double>(util::toFloating(B.node.lon)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;
    double By = static_cast<double>(util::toFloating(B.node.lat)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;
    double Cx = static_cast<double>(util::toFloating(C.node.lon)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;
    double Cy = static_cast<double>(util::toFloating(C.node.lat)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;

    double Axx = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Ax) * std::cos(Ay);
    double Ayy = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Ax) * std::sin(Ay);
    double Bxx = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Bx) * std::cos(By);
    double Byy = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Bx) * std::sin(By);
    double Cxx = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Cx) * std::cos(Cy);
    double Cyy = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Cx) * std::sin(Cy);
    auto AB = new double[2];
    auto AC = new double[2];
    AB[0] = Bxx - Axx;
    AB[1] = Byy - Ayy;
    AC[0] = Cxx - Axx;
    AC[1] = Cyy - Ayy;
    double cross = AB[0] * AC[1] - AB[1] * AC[0];
    return cross;
}
// Compute the distance from A to B
double distance(engine::plugins::IsochroneNode &A, engine::plugins::IsochroneNode &B)
{
    //    double Ax = static_cast<double>(util::toFloating(A.node.lon));
    //    double Ay = static_cast<double>(util::toFloating(A.node.lat));
    //    double Bx = static_cast<double>(util::toFloating(B.node.lon));
    //    double By = static_cast<double>(util::toFloating(B.node.lat));
    double Ax = static_cast<double>(util::toFloating(A.node.lon)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;
    double Ay = static_cast<double>(util::toFloating(A.node.lat)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;
    double Bx = static_cast<double>(util::toFloating(B.node.lon)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;
    double By = static_cast<double>(util::toFloating(B.node.lat)) *
                util::coordinate_calculation::detail::DEGREE_TO_RAD;

    double Axx = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Ax) * std::cos(Ay);
    double Ayy = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Ax) * std::sin(Ay);
    double Bxx = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Bx) * std::cos(By);
    double Byy = util::coordinate_calculation::detail::EARTH_RADIUS * std::cos(Bx) * std::sin(By);
    double d1 = Axx - Bxx;
    double d2 = Ayy - Byy;
    return std::sqrt(d1 * d1 + d2 * d2);
}

// Compute the distance from AB to C
// if isSegment is true, AB is a segment, not a line.
double linePointDist(engine::plugins::IsochroneNode &A,
                     engine::plugins::IsochroneNode &B,
                     engine::plugins::IsochroneNode &C,
                     bool isSegment)
{
    double dist = cross(A, B, C) / distance(A, B);
    //    util::SimpleLogger().Write() << "dist " << dist;
    if (isSegment)
    {
        double dot1 = dot(A, B, C);
        //        util::SimpleLogger().Write() << "dot " << dot1;
        if (dot1 > 0)
        {
            //            util::SimpleLogger().Write() << "dist " << distance(B, C);
            return distance(B, C);
        }
        double dot2 = dot(B, A, C);
        //        util::SimpleLogger().Write() << "dot2 " << dot2;
        if (dot2 > 0)
        {
            //            util::SimpleLogger().Write() << "dist " << distance(A, C);
            return distance(A, C);
        }
    }
    return std::abs(dist);
}
double distanceToEdge(engine::plugins::IsochroneNode &p,
                      engine::plugins::IsochroneNode &e1,
                      engine::plugins::IsochroneNode &e2)
{
    //    double pX = static_cast<double>(util::toFloating(p.node.lon));
    //    double pY = static_cast<double>(util::toFloating(p.node.lat));
    //    double vX = static_cast<double>(util::toFloating(e1.node.lon));
    //    double vY = static_cast<double>(util::toFloating(e1.node.lat));
    //    double wX = static_cast<double>(util::toFloating(e2.node.lon));
    //    double wY = static_cast<double>(util::toFloating(e2.node.lat));
    const std::uint64_t pX = static_cast<std::int32_t>(p.node.lon);
    const std::uint64_t pY = static_cast<std::int32_t>(p.node.lat);
    const std::uint64_t vX = static_cast<std::int32_t>(e1.node.lon);
    const std::uint64_t vY = static_cast<std::int32_t>(e1.node.lat);
    const std::uint64_t wX = static_cast<std::int32_t>(e2.node.lon);
    const std::uint64_t wY = static_cast<std::int32_t>(e2.node.lat);

    //    double squaredEdgeLength = util::coordinate_calculation::haversineDistance(
    //        Coordinate(e1.node.lon, e1.node.lat), Coordinate(e2.node.lon, e2.node.lat));
    //    if (squaredEdgeLength == 0)
    //    {
    //        // v==w
    //        return util::coordinate_calculation::haversineDistance(
    //            Coordinate(p.node.lon, p.node.lat), Coordinate(e1.node.lon, e1.node.lat));
    //    }
    //    var t = ((p.x - v.x) * (w.x - v.x) + (p.y - v.y) * (w.y - v.y)) / l2;
    //    double t =
    //        (static_cast<std::int32_t>((p.node.lon - e1.node.lon) * (e2.node.lon - e1.node.lon)) +
    //         static_cast<std::int32_t>((p.node.lat - e1.node.lat) * (e2.node.lat - e2.node.lat)))
    //         /
    //        squaredEdgeLength;

    //    double distance = std::abs((wX-vX)*(vY-pY) - (vX-pX)*(wY-vY)) / std::sqrt(std::pow(wX -
    //    vX, 2) + std::pow(wY -vY,2));
    double distance = std::abs((wX - vX) * (vY - pY) - (vX - pX) * (wY - vY)) /
                      std::sqrt(std::pow(wX - vX, 2) + std::pow(wY - vY, 2));
    return distance;
    //    double t = ((pX - vX) * (wX - vX) + (pY - vY) * (wY - vY)) / squaredEdgeLength;
    //    t = std::max(0.0, std::min(1.0, t));

    //    Coordinate tCoord(FloatLongitude(vX + t * (wX - vX)), FloatLatitude(vY + t * (wY - vY)));

    //    return util::coordinate_calculation::haversineDistance(
    //            Coordinate(p.node.lon, p.node.lat), tCoord);
}

std::vector<engine::plugins::IsochroneNode>
concavehull(std::vector<engine::plugins::IsochroneNode> &convexhull,
            unsigned int threshold,
            std::vector<engine::plugins::IsochroneNode> &isochrone)
{

    std::vector<engine::plugins::IsochroneNode> concavehull(convexhull);
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

        for (auto iso : isochrone)
        {
            if (std::find(concavehull.begin(), concavehull.end(), iso) != concavehull.end())
            {
                continue;
            }
            //            auto t = distanceToEdge(iso, n1, n2);
            auto t = linePointDist(n1, n2, iso, true);
            engine::plugins::IsochroneNode n3;

            // Test if point is closer to next Edge -> n2,n3
            if (std::next(it) == concavehull.end())
            {
                n3 = *std::next(concavehull.begin());
            }
            else
            {
                n3 = *std::next(it, 2);
            }
            auto t_next = linePointDist(n2, n3, iso, true);

            engine::plugins::IsochroneNode n0;
            if (it == concavehull.begin()) // Start and End Point are the same (Avoid distance)
            {
                n0 = *std::prev(concavehull.end());
            }
            else
            {
                n0 = *std::prev(it);
            }
            auto t_prev = linePointDist(n0, n1, iso, true);

            if (t_prev < t || t_next < t)
            {
                continue; // Distance of the point is closer to neighbour edge
            }
            if (d > t)
            {
                d = t;
                pk = iso;
            }
        }
        //        double edgelength = util::coordinate_calculation::haversineDistance(
        //            Coordinate(n1.node.lon, n1.node.lat), Coordinate(n2.node.lon, n2.node.lat));
        double edgelength = distance(n1, n2);
        //        auto edgelength =
        //        std::sqrt(util::coordinate_calculation::squaredEuclideanDistance(
        //            Coordinate(n1.node.lon, n1.node.lat), Coordinate(n2.node.lon, n2.node.lat)));

        util::SimpleLogger().Write() << "Kante " << edgelength;
        //        util::SimpleLogger().Write() << "Kante " << edgelength2;
        util::SimpleLogger().Write() << "distanz " << d;
        if ((edgelength / d) > threshold)
        {
            it = concavehull.insert(std::next(it), pk);
            it = std::prev(it);
        }
    }
    return concavehull;
}
}
}
#endif // OSRM_CONCAVE_HULL_HPP
