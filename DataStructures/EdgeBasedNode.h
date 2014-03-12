#ifndef EDGE_BASED_NODE_H
#define EDGE_BASED_NODE_H

#include <cmath>

#include <boost/assert.hpp>

#include "../Util/MercatorUtil.h"
#include "../typedefs.h"

#include <osrm/Coordinate.h>

// An EdgeBasedNode represents a node in the edge-expanded graph.
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

    // Computes:
    // - the distance from the given query location to nearest point on this edge (and returns it)
    // - the location on this edge which is nearest to the query location
    // - the ratio ps:pq, where p and q are the end points of this edge, and s is the perpendicular foot of
    //   the query location on the line defined by p and q.
    double ComputePerpendicularDistance(
        const FixedPointCoordinate& query_location,
        FixedPointCoordinate & nearest_location,
        double & ratio,
        double precision = COORDINATE_PRECISION
    ) const {
        BOOST_ASSERT( query_location.isValid() );

        const double epsilon = 1.0/precision;

        if( ignoreInGrid ) {
            return std::numeric_limits<double>::max();
        }

        // p, q : the end points of the underlying edge
        const Point p(lat2y(lat1/COORDINATE_PRECISION), lon1/COORDINATE_PRECISION);
        const Point q(lat2y(lat2/COORDINATE_PRECISION), lon2/COORDINATE_PRECISION);

        // r : query location
        const Point r(lat2y(query_location.lat/COORDINATE_PRECISION),
                            query_location.lon/COORDINATE_PRECISION);

        const Point foot = ComputePerpendicularFoot(p, q, r, epsilon);
        ratio            = ComputeRatio(p, q, foot, epsilon);

        BOOST_ASSERT( !std::isnan(ratio) );

        nearest_location = ComputeNearestPointOnSegment(foot, ratio);

        BOOST_ASSERT( nearest_location.isValid() );

        // TODO: Replace with euclidean approximation when k-NN search is done
        // const double approximated_distance = FixedPointCoordinate::ApproximateEuclideanDistance(
        const double approximated_distance = FixedPointCoordinate::ApproximateDistance(query_location, nearest_location);

        BOOST_ASSERT( 0.0 <= approximated_distance );
        return approximated_distance;
    }

    bool operator<(const EdgeBasedNode & other) const {
        return other.id < id;
    }

    bool operator==(const EdgeBasedNode & other) const {
        return id == other.id;
    }

    // Returns the midpoint of the underlying edge.
    inline FixedPointCoordinate Centroid() const {
        return FixedPointCoordinate((lat1+lat2)/2, (lon1+lon2)/2);
    }

    NodeID id;

    // The coordinates of the end-points of the underlying edge.
    int lat1;
    int lat2;
    int lon1;
    int lon2:31;

    bool belongsToTinyComponent:1;
    NodeID nameID;

    // The weight of the underlying edge.
    unsigned weight:31;

    bool ignoreInGrid:1;

private:

    typedef std::pair<double,double> Point;

    // Compute the perpendicular foot of point r on the line defined by p and q.
    Point ComputePerpendicularFoot(const Point &p, const Point &q, const Point &r, double epsilon) const {

        // the projection of r onto the line pq
        double foot_x, foot_y;

        const bool is_parallel_to_y_axis = std::abs(q.first - p.first) < epsilon;

        if( is_parallel_to_y_axis ) {
            foot_x = q.first;
            foot_y = r.second;
        } else {
            // the slope of the line through (a|b) and (c|d)
            const double m = (q.second - p.second) / (q.first - p.first);

            // Projection of (x|y) onto the line joining (a|b) and (c|d).
            foot_x = ((r.first + (m*r.second)) + (m*m*p.first - m*p.second))/(1.0 + m*m);
            foot_y = p.second + m*(foot_x - p.first);
        }

        return Point(foot_x, foot_y);
    }

    // Compute the ratio of the line segment pr to line segment pq.
    double ComputeRatio(const Point & p, const Point & q, const Point & r, double epsilon) const {

        const bool is_parallel_to_x_axis = std::abs(q.second-p.second) < epsilon;
        const bool is_parallel_to_y_axis = std::abs(q.first -p.first ) < epsilon;

        double ratio;

        if( !is_parallel_to_y_axis ) {
            ratio = (r.first - p.first)/(q.first - p.first);
        } else if( !is_parallel_to_x_axis ) {
            ratio = (r.second - p.second)/(q.second - p.second);
        } else {
            // (a|b) and (c|d) are essentially the same point
            // by convention, we set the ratio to 0 in this case
            //ratio = ((lat2 == query_location.lat) && (lon2 == query_location.lon)) ? 1. : 0.;
            ratio = 0.0;
        }

        // Round to integer if the ratio is close to 0 or 1.
        if( std::abs(ratio) <= epsilon ) {
            ratio = 0.0;
        } else if( std::abs(ratio-1.0) <= epsilon ) {
            ratio = 1.0;
        }

        return ratio;
    }

    // Computes the point on the segment pq which is nearest to a point r = p + lambda * (q-p).
    // p and q are the end points of the underlying edge.
    FixedPointCoordinate ComputeNearestPointOnSegment(const Point & r, double lambda) const {

        if( lambda <= 0.0 ) {
            return FixedPointCoordinate(lat1, lon1);
        } else if( lambda >= 1.0 ) {
            return FixedPointCoordinate(lat2, lon2);
        }

        // r lies between p and q
        return  FixedPointCoordinate(
                    y2lat(r.first)*COORDINATE_PRECISION,
                    r.second*COORDINATE_PRECISION
                );
    }

};

#endif //EDGE_BASED_NODE_H
