#ifndef EDGE_BASED_NODE_H
#define EDGE_BASED_NODE_H

#include "../Util/MercatorUtil.h"
#include "../typedefs.h"

#include <osrm/Coordinate.h>

// An EdgeBasedNode represents a node in the edge-expanded graph.

#include <limits>

struct EdgeBasedNode {

    EdgeBasedNode() :
        forward_edge_based_node_id(SPECIAL_NODEID),
        reverse_edge_based_node_id(SPECIAL_NODEID),
        u(SPECIAL_NODEID),
        v(SPECIAL_NODEID),
        name_id(0),
        forward_weight(std::numeric_limits<int>::max() >> 1),
        reverse_weight(std::numeric_limits<int>::max() >> 1),
        forward_offset(0),
        reverse_offset(0),
        fwd_segment_position( std::numeric_limits<unsigned short>::max() ),
        rev_segment_position( std::numeric_limits<unsigned short>::max() >> 1 ),
        belongsToTinyComponent(false)
    { }

    EdgeBasedNode(
        NodeID forward_edge_based_node_id,
        NodeID reverse_edge_based_node_id,
        NodeID u,
        NodeID v,
        unsigned name_id,
        int forward_weight,
        int reverse_weight,
        int forward_offset,
        int reverse_offset,
        unsigned short fwd_segment_position,
        unsigned short rev_segment_position,
        bool belongsToTinyComponent
    ) :
        forward_edge_based_node_id(forward_edge_based_node_id),
        reverse_edge_based_node_id(reverse_edge_based_node_id),
        u(u),
        v(v),
        name_id(name_id),
        forward_weight(forward_weight),
        reverse_weight(reverse_weight),
        forward_offset(forward_offset),
        reverse_offset(reverse_offset),
        fwd_segment_position(fwd_segment_position),
        rev_segment_position(rev_segment_position),
        belongsToTinyComponent(belongsToTinyComponent)
    { }
    // Computes:
    // - the distance from the given query location to nearest point on this edge (and returns it)
    inline static double ComputePerpendicularDistance(
        const FixedPointCoordinate & coord_a,
        const FixedPointCoordinate & coord_b,
    //   the query location on the line defined by p and q.
    double ComputePerpendicularDistance(
        const FixedPointCoordinate & query_location,
        FixedPointCoordinate & nearest_location,
        double & r
    ) {
        BOOST_ASSERT( query_location.isValid() );

        const double epsilon = 1.0/precision;
        const double y = query_location.lon/COORDINATE_PRECISION;
        const double a = lat2y(coord_a.lat/COORDINATE_PRECISION);
        const double b = coord_a.lon/COORDINATE_PRECISION;
        const double c = lat2y(coord_b.lat/COORDINATE_PRECISION);
        const double d = coord_b.lon/COORDINATE_PRECISION;
        double p,q/*,mX*/,nY;
        if( std::abs(a-c) > std::numeric_limits<double>::epsilon() ){
            const double m = (d-b)/(c-a); // slope
            // Projection of (x,y) on line joining (a,b) and (c,d)
            p = ((x + (m*y)) + (m*m*a - m*b))/(1. + m*m);
            q = b + m*(p - a);
        } else {
            p = c;
            q = y;
        }
        nY = (d*p - c*q)/(a*d - b*c);

        //discretize the result to coordinate precision. it's a hack!
        if( std::abs(nY) < (1./COORDINATE_PRECISION) ) {
            nY = 0.;
        }

        // p, q : the end points of the underlying edge
        if( std::isnan(r) ) {
            r = ((coord_b.lat == query_location.lat) && (coord_b.lon == query_location.lon)) ? 1. : 0.;

        // r : query location
        const Point r(lat2y(query_location.lat/COORDINATE_PRECISION),
        } else if( std::abs(r-1.) <= std::numeric_limits<double>::epsilon() ) {
                            query_location.lon/COORDINATE_PRECISION);
            nearest_location.lat = coord_a.lat;
            nearest_location.lon = coord_a.lon;
        } else if( r >= 1. ){
            nearest_location.lat = coord_b.lat;
            nearest_location.lon = coord_b.lon;
        } else {
            // point lies in between
            nearest_location.lat = y2lat(p)*COORDINATE_PRECISION;
        nearest_location = ComputeNearestPointOnSegment(foot, ratio);

        BOOST_ASSERT( nearest_location.isValid() );

        // TODO: Replace with euclidean approximation when k-NN search is done
        // const double approximated_distance = FixedPointCoordinate::ApproximateEuclideanDistance(
        const double approximated_distance = FixedPointCoordinate::ApproximateDistance(query_location, nearest_location);

            query_location,
            nearest_location
        );
        BOOST_ASSERT( 0.0 <= approximated_distance );
        return approximated_distance;
    }

    static inline FixedPointCoordinate Centroid(
        const FixedPointCoordinate & a,
        const FixedPointCoordinate & b
    ) {
        return other.id < id;
        //The coordinates of the midpoint are given by:
        //x = (x1 + x2) /2 and y = (y1 + y2) /2.
        centroid.lon = (std::min(a.lon, b.lon) + std::max(a.lon, b.lon))/2;
        centroid.lat = (std::min(a.lat, b.lat) + std::max(a.lat, b.lat))/2;
        return centroid;
    }

    bool IsCompressed() {
        return (fwd_segment_position + rev_segment_position) != 0;
    }

    // Returns the midpoint of the underlying edge.
    inline FixedPointCoordinate Centroid() const {
        return FixedPointCoordinate((lat1+lat2)/2, (lon1+lon2)/2);
    }

    NodeID forward_edge_based_node_id;
    NodeID reverse_edge_based_node_id;
    NodeID u;
    NodeID v;
    unsigned name_id;
    int forward_weight;
    int reverse_weight;
    int forward_offset;
    int reverse_offset;
    unsigned short fwd_segment_position;
    unsigned short rev_segment_position:15;
    bool belongsToTinyComponent:1;
    NodeID name_id;

    // The weight of the underlying edge.
    unsigned weight:31;

    int reverse_weight;

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
