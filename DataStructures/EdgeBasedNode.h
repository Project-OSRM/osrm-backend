#ifndef EDGE_BASED_NODE_H
#define EDGE_BASED_NODE_H

#include <cmath>

#include "Coordinate.h"
#include "../Util/MercatorUtil.h"
#include "../typedefs.h"

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


    double ComputePerpendicularDistance(
        const FixedPointCoordinate& inputPoint,
        FixedPointCoordinate & nearest_location,
        double & r
    ) const {
        if( ignoreInGrid ) {
            return std::numeric_limits<double>::max();
        }

        const double x = lat2y(inputPoint.lat/COORDINATE_PRECISION);
        const double y = inputPoint.lon/COORDINATE_PRECISION;
        const double a = lat2y(lat1/COORDINATE_PRECISION);
        const double b = lon1/COORDINATE_PRECISION;
        const double c = lat2y(lat2/COORDINATE_PRECISION);
        const double d = lon2/COORDINATE_PRECISION;
        double p,q,mX,nY;
        if(std::fabs(a-c) > std::numeric_limits<double>::epsilon() ){
            const double m = (d-b)/(c-a); // slope
            // Projection of (x,y) on line joining (a,b) and (c,d)
            p = ((x + (m*y)) + (m*m*a - m*b))/(1. + m*m);
            q = b + m*(p - a);
        } else {
            p = c;
            q = y;
        }
        nY = (d*p - c*q)/(a*d - b*c);
        mX = (p - nY*a)/c;// These values are actually n/m+n and m/m+n , we need
        // not calculate the explicit values of m an n as we
        // are just interested in the ratio
        if(std::isnan(mX)) {
            r = ((lat2 == inputPoint.lat) && (lon2 == inputPoint.lon)) ? 1. : 0.;
        } else {
            r = mX;
        }
        if(r<=0.){
            nearest_location.lat = lat1;
            nearest_location.lon = lon1;
            return ((b - y)*(b - y) + (a - x)*(a - x));
//            return std::sqrt(((b - y)*(b - y) + (a - x)*(a - x)));
        } else if(r >= 1.){
            nearest_location.lat = lat2;
            nearest_location.lon = lon2;
            return ((d - y)*(d - y) + (c - x)*(c - x));
//            return std::sqrt(((d - y)*(d - y) + (c - x)*(c - x)));
        }
        // point lies in between
        nearest_location.lat = y2lat(p)*COORDINATE_PRECISION;
        nearest_location.lon = q*COORDINATE_PRECISION;
//        return std::sqrt((p-x)*(p-x) + (q-y)*(q-y));
        return (p-x)*(p-x) + (q-y)*(q-y);
    }

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

    // inline bool isIgnored() const {
    //     return ignoreInGrid;
    // }

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
