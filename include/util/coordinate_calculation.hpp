#ifndef COORDINATE_CALCULATION
#define COORDINATE_CALCULATION

#include "osrm/coordinate.hpp"

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
double haversineDistance(const int lat1, const int lon1, const int lat2, const int lon2);

double haversineDistance(const FixedPointCoordinate first_coordinate,
                         const FixedPointCoordinate second_coordinate);

double greatCircleDistance(const FixedPointCoordinate first_coordinate,
                           const FixedPointCoordinate second_coordinate);

double greatCircleDistance(const int lat1, const int lon1, const int lat2, const int lon2);

double perpendicularDistance(const FixedPointCoordinate segment_source,
                             const FixedPointCoordinate segment_target,
                             const FixedPointCoordinate query_location);

double perpendicularDistance(const FixedPointCoordinate segment_source,
                             const FixedPointCoordinate segment_target,
                             const FixedPointCoordinate query_location,
                             FixedPointCoordinate &nearest_location,
                             double &ratio);

double
perpendicularDistanceFromProjectedCoordinate(const FixedPointCoordinate segment_source,
                                             const FixedPointCoordinate segment_target,
                                             const FixedPointCoordinate query_location,
                                             const std::pair<double, double> projected_coordinate);

double
perpendicularDistanceFromProjectedCoordinate(const FixedPointCoordinate segment_source,
                                             const FixedPointCoordinate segment_target,
                                             const FixedPointCoordinate query_location,
                                             const std::pair<double, double> projected_coordinate,
                                             FixedPointCoordinate &nearest_location,
                                             double &ratio);

double degToRad(const double degree);
double radToDeg(const double radian);

double bearing(const FixedPointCoordinate first_coordinate,
               const FixedPointCoordinate second_coordinate);

// Get angle of line segment (A,C)->(C,B)
double computeAngle(const FixedPointCoordinate first,
                    const FixedPointCoordinate second,
                    const FixedPointCoordinate third);

inline FixedPointCoordinate interpolateLinear( double factor, const FixedPointCoordinate from, const FixedPointCoordinate to )
{
  BOOST_ASSERT( 0 <= factor && factor <= 1.0 );
  return {from.lat + static_cast<int>(factor * (to.lat - from.lat)), from.lon + static_cast<int>(factor * (to.lon - from.lon))};
}

namespace mercator
{
double yToLat(const double value);
double latToY(const double latitude);
} // ns mercator
} // ns coordinate_calculation
} // ns util
} // ns osrm

#endif // COORDINATE_CALCULATION
