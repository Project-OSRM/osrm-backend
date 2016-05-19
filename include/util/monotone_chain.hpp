//
// Created by robin on 5/11/16.
//

#ifndef OSRM_MONOTONE_CHAIN_HPP
#define OSRM_MONOTONE_CHAIN_HPP

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

int ccw(Node &p, Node &q, Node &r)
{
    //Using double values to avoid integer overflows
    double Q_lat = static_cast<double>(util::toFloating(q.node.lat));
    double Q_lon = static_cast<double>(util::toFloating(q.node.lon));
    double R_lat = static_cast<double>(util::toFloating(r.node.lat));
    double R_lon = static_cast<double>(util::toFloating(r.node.lon));
    double P_lat = static_cast<double>(util::toFloating(p.node.lat));
    double P_lon = static_cast<double>(util::toFloating(p.node.lon));

    double val = (Q_lat - P_lat) * (R_lon - P_lon) - (Q_lon - P_lon) * (R_lat - P_lat);
    if (val == 0)
    {
        return 0; // colinear
    }
    return (val < 0) ? -1 : 1; // clock or counterclock wise
}

std::vector<Node> monotoneChain(std::vector<Node> &nodes)
{
    std::vector<Node> P(nodes);
    int n = P.size(), k = 0;
    std::vector<Node> H(2 * n);


    // sort Points by lat and lon
    std::sort(P.begin(), P.end(), [&](const Node &a, const Node &b)
              {
                  return a.node.lat < b.node.lat ||
                         (a.node.lat == b.node.lat && a.node.lon < b.node.lon);
              });

    // Build lower hull
    for (int i = 0; i < n; ++i)
    {
        while (k >= 2 && ccw(H[k - 2], H[k - 1], P[i]) <= 0)
            k--;
        H[k++] = P[i];
    }

    // Build upper hull
    for (int i = n - 2, t = k + 1; i >= 0; i--)
    {
        while (k >= t && ccw(H[k - 2], H[k - 1], P[i]) <= 0)
            k--;
        H[k++] = P[i];
    }

    H.resize(k - 1);

    return H;
}
}
}
#endif // OSRM_MONOTONE_CHAIN_HPP
