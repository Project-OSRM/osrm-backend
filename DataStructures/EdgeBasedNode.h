#ifndef EDGE_BASED_NODE_H
#define EDGE_BASED_NODE_H

#include "Coordinate.h"

struct EdgeBasedNode {
    EdgeBasedNode() :
        id(INT_MAX),
        lat1(INT_MAX),
        lat2(INT_MAX),
        lon1(INT_MAX),
        lon2(INT_MAX >> 1),
        belongsToTinyComponent(false),
        nameID(UINT_MAX),
        weight(UINT_MAX >> 1),
        ignoreInGrid(false)
    { }

    bool operator<(const EdgeBasedNode & other) const {
        return other.id < id;
    }

    bool operator==(const EdgeBasedNode & other) const {
        return id == other.id;
    }

    inline FixedPointCoordinate Centroid() const {
        FixedPointCoordinate centroid;
        //The coordinates of the midpoint are given by:
        //x = (x1 + x2) /2 and y = (y1 + y2) /2.
        centroid.lon = (std::min(lon1, lon2) + std::max(lon1, lon2))/2;
        centroid.lat = (std::min(lat1, lat2) + std::max(lat1, lat2))/2;
        return centroid;
    }

    inline bool isIgnored() const {
        return ignoreInGrid;
    }

    NodeID id;
    int lat1;
    int lat2;
    int lon1;
    int lon2:31;
    bool belongsToTinyComponent:1;
    NodeID nameID;
    unsigned weight:31;
    bool ignoreInGrid:1;
};

#endif //EDGE_BASED_NODE_H