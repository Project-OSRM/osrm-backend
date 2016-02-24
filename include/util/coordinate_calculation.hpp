#ifndef COORDINATE_CALCULATION
#define COORDINATE_CALCULATION

#include "util/coordinate.hpp"

#include <iostream>
#include <utility>

#include <boost/assert.hpp>

namespace osrm
{
namespace util
{

const constexpr long double RAD = 0.017453292519943295769236907684886;
// earth radius varies between 6,356.750-6,378.135 km (3,949.901-3,963.189mi)
// The IUGG value for the equatorial radius is 6378.137 km (3963.19 miles)
const constexpr long double EARTH_RADIUS = 6372797.560856;

namespace coordinate_calculation
{

//! Projects both coordinates and takes the euclidean distance of the projected points
// Does not return meters!
double euclideanDistance(const Coordinate first_coordinate,
                         const Coordinate second_coordinate);

double haversineDistance(const Coordinate first_coordinate,
                         const Coordinate second_coordinate);

double greatCircleDistance(const Coordinate first_coordinate,
                           const Coordinate second_coordinate);


double perpendicularDistance(const Coordinate segment_source,
                             const Coordinate segment_target,
                             const Coordinate query_location);

double perpendicularDistance(const Coordinate segment_source,
                             const Coordinate segment_target,
                             const Coordinate query_location,
                             Coordinate &nearest_location,
                             double &ratio);

double
perpendicularDistanceFromProjectedCoordinate(const Coordinate segment_source,
                                             const Coordinate segment_target,
                                             const Coordinate query_location,
                                             const std::pair<double, double> projected_xy_coordinate);

double
perpendicularDistanceFromProjectedCoordinate(const Coordinate segment_source,
                                             const Coordinate segment_target,
                                             const Coordinate query_location,
                                             const std::pair<double, double> projected_xy_coordinate,
                                             Coordinate &nearest_location,
                                             double &ratio);

double degToRad(const double degree);
double radToDeg(const double radian);

double bearing(const Coordinate first_coordinate,
               const Coordinate second_coordinate);

// Get angle of line segment (A,C)->(C,B)
double computeAngle(const Coordinate first,
                    const Coordinate second,
                    const Coordinate third);

Coordinate interpolateLinear( double factor, const Coordinate from, const Coordinate to );

namespace mercator
{
FloatLatitude yToLat(const double value);
double latToY(const FloatLatitude latitude);
} // ns mercator
} // ns coordinate_calculation
} // ns util
} // ns osrm

#endif // COORDINATE_CALCULATION
