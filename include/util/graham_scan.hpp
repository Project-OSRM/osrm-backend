//
// Created by robin on 4/27/16.
//

#ifndef OSRM_GRAHAM_SCAN_HPP
#define OSRM_GRAHAM_SCAN_HPP

#include <engine/plugins/isochrone.hpp>
#include <util/coordinate.hpp>
#include <util/coordinate_calculation.hpp>
#include <util/simple_logger.hpp>

#include <stack>
#include <algorithm>
#include <iterator>

namespace osrm
{
namespace util
{
using Node = engine::plugins::IsochroneNode;

bool lon_compare(engine::plugins::IsochroneNode &a, engine::plugins::IsochroneNode &b)
{
    return b.node.lon == a.node.lon ? b.node.lat < a.node.lat : b.node.lon < a.node.lon;
}
std::uint64_t squaredEuclideanDistance(Node n1, Node n2)
{
    return util::coordinate_calculation::squaredEuclideanDistance(
        Coordinate(n1.node.lon, n1.node.lat), Coordinate(n2.node.lon, n2.node.lat));
}

int orientation(Node p, Node q, Node r)
{
    int Q_lat = static_cast<int>(q.node.lat); // b_x -a_x
    int Q_lon= static_cast<int>(q.node.lon); // b_x -a_x
    int R_lat = static_cast<int>(r.node.lat); // c_x - c_a
    int R_lon = static_cast<int>(r.node.lon); // c_x - c_a
    int P_lat = static_cast<int>(p.node.lat); // b_y - a_y
    int P_lon = static_cast<int>(p.node.lon); // c_y - a_y

    int val = (Q_lat - P_lat) * (R_lon - P_lon) - (Q_lon - P_lon) * (R_lat - P_lat);

    if (val == 0)
    {
        return 0; // colinear
    }
    return (val < 0) ? -1 : 1; // clock or counterclock wise
}
struct NodeComparer
{
    const Node p0; // lowest Node

    explicit NodeComparer(Node &p0) : p0(p0){};

    bool operator()(const Node &a, const Node &b)
    {
        int o = orientation(p0, a, b);
        if (o == 0)
        {
            return (squaredEuclideanDistance(p0, a) < squaredEuclideanDistance(p0, b));
        }
        return o == 1;
    }
};

Node nextToTop(std::stack<Node> &S)
{
    Node p = S.top();
    S.pop();
    Node res = S.top();
    S.push(p);
    return res;
}
void swap(Node &p1, Node &p2)
{
    Node temp = p1;
    p1 = p2;
    p2 = temp;
}
void convexHull(engine::plugins::IsochroneSet &set, util::json::Object &response)
{
    std::vector<Node> points(set.begin(), set.end());

    auto result = std::min_element(points.begin(), points.end(), lon_compare);
    int n = points.size();

    swap(points[0], *result);


    std::sort(std::next(points.begin()), points.end(), NodeComparer(points[0]));

//    points.push_back(*result);
//    int m = 1; // Initialize size of modified array
//    for (int i = 1; i < n; i++)
//    {
//        // Keep removing i while angle of i and i+1 is same
//        // with respect to p0
//        while ((i < n - 1) && (orientation(points[0], points[i], points[i + 1]) == 0))
//        {
//            i++;
//        }
//
//        points[m] = points[i];
//        m++; // Update size of modified array
//    }
    // If modified array of points has less than 3 points,
    // convex hull is not possible
//    if (m < 3)
//        return;

    // Create an empty stack and push first three points
    // to it.
    std::stack<Node> S;
    S.push(points[0]);
    S.push(points[1]);
    S.push(points[2]);

    // Process remaining n-3 points
    for (int i = 3; i < n; i++)
    {
        // Keep removing top while the angle formed by
        // points next-to-top, top, and points[i] makes
        // a non-left turn
        Node top = S.top();
        S.pop();
        while (orientation(S.top(), top, points[i]) != 1 )
        {
            top = S.top();
            S.pop();
        }
        S.push(top);
        S.push(points[i]);
    }

    // Now stack has the output points, print contents of stack

    std::vector<Node> vec;
    while (!S.empty())
    {
        Node p = S.top();
        vec.emplace_back(p);
        S.pop();
    }

//    std::sort(vec.begin(), vec.end(), [](Node a, Node b) {
//        return b.node.lon + b.node.lat < a.node.lon + a.node.lat;
//    });
    util::json::Array borderjson;
    for(auto p : vec) {
        util::json::Object point;
        point.values["lat"] = static_cast<double>(util::toFloating(p.node.lat));
        point.values["lon"] = static_cast<double>(util::toFloating(p.node.lon));
        borderjson.values.push_back(point);
    }
    response.values["border"] = std::move(borderjson);
}
}
}

#endif // OSRM_GRAHAM_SCAN_HPP
