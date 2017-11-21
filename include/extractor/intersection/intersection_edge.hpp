#ifndef OSRM_EXTRACTOR_INTERSECTION_INTERSECTION_EDGE_HPP
#define OSRM_EXTRACTOR_INTERSECTION_INTERSECTION_EDGE_HPP

#include "util/typedefs.hpp"

#include <vector>

namespace osrm
{
namespace extractor
{
namespace intersection
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

struct IntersectionEdgeGeometry
{
    EdgeID edge;
    double initial_bearing;
    double perceived_bearing;
    double length;

    bool operator<(const IntersectionEdgeGeometry &other) const { return edge < other.edge; }
};

using IntersectionEdgeGeometries = std::vector<IntersectionEdgeGeometry>;
}
}
}

#endif
