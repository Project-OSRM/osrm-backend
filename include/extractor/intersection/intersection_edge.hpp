#ifndef OSRM_EXTRACTOR_INTERSECTION_INTERSECTION_EDGE_HPP
#define OSRM_EXTRACTOR_INTERSECTION_INTERSECTION_EDGE_HPP

#include "util/typedefs.hpp"

#include <vector>

namespace osrm::extractor::intersection
{

// IntersectionEdge is an alias for incoming and outgoing node-based graph edges of an intersection
struct IntersectionEdge
{
    NodeID node;
    EdgeID edge;

    bool operator<(const IntersectionEdge &other) const
    {
        return std::tie(node, edge) < std::tie(other.node, other.edge);
    }
};

using IntersectionEdges = std::vector<IntersectionEdge>;

// The extracted geometry of an intersection edge only knows about edge IDs and bearings
// Bearing is the direction in clockwise angle from true north after taking the turn:
//    0 = heading north, 90 = east, 180 = south, 270 = west
// `initial_bearing` is the original OSM edge bearing
// `perceived_bearing` is the edge bearing after line fitting and edges merging
struct IntersectionEdgeGeometry
{
    EdgeID eid;
    double initial_bearing;
    double perceived_bearing;
    double segment_length;

    bool operator<(const IntersectionEdgeGeometry &other) const { return eid < other.eid; }
};

using IntersectionEdgeGeometries = std::vector<IntersectionEdgeGeometry>;
} // namespace osrm::extractor::intersection

#endif
