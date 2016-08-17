#ifndef COORDINATE_CALCULATION
#define COORDINATE_CALCULATION

#include "util/coordinate.hpp"

#include <boost/optional.hpp>

#include <utility>

namespace osrm
{
namespace util
{
namespace coordinate_calculation
{

namespace detail
{
const constexpr long double DEGREE_TO_RAD = 0.017453292519943295769236907684886;
const constexpr long double RAD_TO_DEGREE = 1. / DEGREE_TO_RAD;
// earth radius varies between 6,356.750-6,378.135 km (3,949.901-3,963.189mi)
// The IUGG value for the equatorial radius is 6378.137 km (3963.19 miles)
const constexpr long double EARTH_RADIUS = 6372797.560856;
}

//! Takes the squared euclidean distance of the input coordinates. Does not return meters!
std::uint64_t squaredEuclideanDistance(const Coordinate lhs, const Coordinate rhs);

double haversineDistance(const Coordinate first_coordinate, const Coordinate second_coordinate);

double greatCircleDistance(const Coordinate first_coordinate, const Coordinate second_coordinate);

// Find the closest distance and location between coordinate and the line connecting source and
// target:
//             coordinate
//                 |
//                 |
// source -------- x -------- target.
// returns x as well as the distance between source and x as ratio ([0,1])
inline std::pair<double, FloatCoordinate> projectPointOnSegment(const FloatCoordinate &source,
                                                                const FloatCoordinate &target,
                                                                const FloatCoordinate &coordinate)
{
    const FloatCoordinate slope_vector{target.lon - source.lon, target.lat - source.lat};
    const FloatCoordinate rel_coordinate{coordinate.lon - source.lon, coordinate.lat - source.lat};
    // dot product of two un-normed vectors
    const auto unnormed_ratio = static_cast<double>(slope_vector.lon * rel_coordinate.lon) +
                                static_cast<double>(slope_vector.lat * rel_coordinate.lat);
    // squared length of the slope vector
    const auto squared_length = static_cast<double>(slope_vector.lon * slope_vector.lon) +
                                static_cast<double>(slope_vector.lat * slope_vector.lat);

    if (squared_length < std::numeric_limits<double>::epsilon())
    {
        return {0, source};
    }

    const double normed_ratio = unnormed_ratio / squared_length;
    double clamped_ratio = normed_ratio;
    if (clamped_ratio > 1.)
    {
        clamped_ratio = 1.;
    }
    else if (clamped_ratio < 0.)
    {
        clamped_ratio = 0.;
    }

    return {clamped_ratio,
            {
                FloatLongitude{1.0 - clamped_ratio} * source.lon +
                    target.lon * FloatLongitude{clamped_ratio},
                FloatLatitude{1.0 - clamped_ratio} * source.lat +
                    target.lat * FloatLatitude{clamped_ratio},
            }};
}

double perpendicularDistance(const Coordinate segment_source,
                             const Coordinate segment_target,
                             const Coordinate query_location);

double perpendicularDistance(const Coordinate segment_source,
                             const Coordinate segment_target,
                             const Coordinate query_location,
                             Coordinate &nearest_location,
                             double &ratio);

Coordinate centroid(const Coordinate lhs, const Coordinate rhs);

double bearing(const Coordinate first_coordinate, const Coordinate second_coordinate);

// Get angle of line segment (A,C)->(C,B)
double computeAngle(const Coordinate first, const Coordinate second, const Coordinate third);

// find the center of a circle through three coordinates
boost::optional<Coordinate> circleCenter(const Coordinate first_coordinate,
                                         const Coordinate second_coordinate,
                                         const Coordinate third_coordinate);

// find the radius of a circle through three coordinates
double circleRadius(const Coordinate first_coordinate,
                    const Coordinate second_coordinate,
                    const Coordinate third_coordinate);

// factor in [0,1]. Returns point along the straight line between from and to. 0 returns from, 1
// returns to
Coordinate interpolateLinear(double factor, const Coordinate from, const Coordinate to);

// compute the signed area of a triangle
double signedArea(const Coordinate first_coordinate,
                  const Coordinate second_coordinate,
                  const Coordinate third_coordinate);

// check if a set of three coordinates is given in CCW order
bool isCCW(const Coordinate first_coordinate,
           const Coordinate second_coordinate,
           const Coordinate third_coordinate);

} // ns coordinate_calculation
} // ns util
} // ns osrm

#endif // COORDINATE_CALCULATION
