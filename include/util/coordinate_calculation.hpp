#ifndef COORDINATE_CALCULATION
#define COORDINATE_CALCULATION

#include "util/coordinate.hpp"

#include <boost/math/constants/constants.hpp>

#include <utility>

namespace osrm
{
namespace util
{
namespace coordinate_calculation
{

const constexpr long double DEGREE_TO_RAD = 0.017453292519943295769236907684886;
const constexpr long double RAD_TO_DEGREE = 1. / DEGREE_TO_RAD;
// earth radius varies between 6,356.750-6,378.135 km (3,949.901-3,963.189mi)
// The IUGG value for the equatorial radius is 6378.137 km (3963.19 miles)
const constexpr long double EARTH_RADIUS = 6372797.560856;
// radius used by WGS84
const constexpr double EARTH_RADIUS_WGS84 = 6378137.0;

namespace detail
{
    // earth circumference devided by 2
    const constexpr double MAXEXTENT = EARTH_RADIUS_WGS84 * boost::math::constants::pi<double>();
    // ^ math functions are not constexpr since they have side-effects (setting errno) :(
    const double MAX_LATITUDE = RAD_TO_DEGREE * (2.0 * std::atan(std::exp(180.0 * DEGREE_TO_RAD)) - boost::math::constants::half_pi<double>());
    const constexpr double MAX_LONGITUDE = 180.0;
}


//! Projects both coordinates and takes the euclidean distance of the projected points
// Does not return meters!
double euclideanDistance(const Coordinate first_coordinate, const Coordinate second_coordinate);

double haversineDistance(const Coordinate first_coordinate, const Coordinate second_coordinate);

double greatCircleDistance(const Coordinate first_coordinate, const Coordinate second_coordinate);

double perpendicularDistance(const Coordinate segment_source,
                             const Coordinate segment_target,
                             const Coordinate query_location);

double perpendicularDistance(const Coordinate segment_source,
                             const Coordinate segment_target,
                             const Coordinate query_location,
                             Coordinate &nearest_location,
                             double &ratio);

double perpendicularDistanceFromProjectedCoordinate(
    const Coordinate segment_source,
    const Coordinate segment_target,
    const Coordinate query_location,
    const std::pair<double, double> projected_xy_coordinate);

double perpendicularDistanceFromProjectedCoordinate(
    const Coordinate segment_source,
    const Coordinate segment_target,
    const Coordinate query_location,
    const std::pair<double, double> projected_xy_coordinate,
    Coordinate &nearest_location,
    double &ratio);

double bearing(const Coordinate first_coordinate, const Coordinate second_coordinate);

// Get angle of line segment (A,C)->(C,B)
double computeAngle(const Coordinate first, const Coordinate second, const Coordinate third);

// factor in [0,1]. Returns point along the straight line between from and to. 0 returns from, 1
// returns to
Coordinate interpolateLinear(double factor, const Coordinate from, const Coordinate to);

namespace mercator
{
// This is the global default tile size for all Mapbox Vector Tiles
const constexpr double TILE_SIZE = 256.0;
// Converts projected mercator degrees to PX
const constexpr double DEGREE_TO_PX = detail::MAXEXTENT / 180.0;

double degreeToPixel(FloatLatitude lat, unsigned zoom);
double degreeToPixel(FloatLongitude lon, unsigned zoom);
FloatLatitude yToLat(const double value);
double latToY(const FloatLatitude latitude);
void xyzToMercator(const int x, const int y, const int z, double &minx, double &miny, double &maxx, double &maxy);
void xyzToWSG84(const int x, const int y, const int z, double &minx, double &miny, double &maxx, double &maxy);
} // ns mercator
} // ns coordinate_calculation
} // ns util
} // ns osrm

#endif // COORDINATE_CALCULATION
