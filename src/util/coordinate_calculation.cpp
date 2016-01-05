#include "util/coordinate_calculation.hpp"

#include "util/mercator.hpp"
#include "util/string_util.hpp"

#include <boost/assert.hpp>

#include "osrm/coordinate.hpp"

#include <cmath>

#include <limits>

namespace osrm
{
namespace util
{

namespace
{
constexpr static const double RAD = 0.017453292519943295769236907684886;
// earth radius varies between 6,356.750-6,378.135 km (3,949.901-3,963.189mi)
// The IUGG value for the equatorial radius is 6378.137 km (3963.19 miles)
constexpr static const double earth_radius = 6372797.560856;
}

namespace coordinate_calculation
{

double haversineDistance(const int lat1, const int lon1, const int lat2, const int lon2)
{
    BOOST_ASSERT(lat1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lat2 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon2 != std::numeric_limits<int>::min());
    const double lt1 = lat1 / COORDINATE_PRECISION;
    const double ln1 = lon1 / COORDINATE_PRECISION;
    const double lt2 = lat2 / COORDINATE_PRECISION;
    const double ln2 = lon2 / COORDINATE_PRECISION;
    const double dlat1 = lt1 * (RAD);

    const double dlong1 = ln1 * (RAD);
    const double dlat2 = lt2 * (RAD);
    const double dlong2 = ln2 * (RAD);

    const double dlong = dlong1 - dlong2;
    const double dlat = dlat1 - dlat2;

    const double aharv = std::pow(std::sin(dlat / 2.0), 2.0) +
                         std::cos(dlat1) * std::cos(dlat2) * std::pow(std::sin(dlong / 2.), 2);
    const double charv = 2. * std::atan2(std::sqrt(aharv), std::sqrt(1.0 - aharv));
    return earth_radius * charv;
}

double haversineDistance(const FixedPointCoordinate &coordinate_1,
                         const FixedPointCoordinate &coordinate_2)
{
    return haversineDistance(coordinate_1.lat, coordinate_1.lon, coordinate_2.lat,
                             coordinate_2.lon);
}

double greatCircleDistance(const FixedPointCoordinate &coordinate_1,
                           const FixedPointCoordinate &coordinate_2)
{
    return greatCircleDistance(coordinate_1.lat, coordinate_1.lon, coordinate_2.lat,
                               coordinate_2.lon);
}

double greatCircleDistance(const int lat1, const int lon1, const int lat2, const int lon2)
{
    BOOST_ASSERT(lat1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lat2 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon2 != std::numeric_limits<int>::min());

    const double float_lat1 = (lat1 / COORDINATE_PRECISION) * RAD;
    const double float_lon1 = (lon1 / COORDINATE_PRECISION) * RAD;
    const double float_lat2 = (lat2 / COORDINATE_PRECISION) * RAD;
    const double float_lon2 = (lon2 / COORDINATE_PRECISION) * RAD;

    const double x_value = (float_lon2 - float_lon1) * std::cos((float_lat1 + float_lat2) / 2.0);
    const double y_value = float_lat2 - float_lat1;
    return std::hypot(x_value, y_value) * earth_radius;
}

double perpendicularDistance(const FixedPointCoordinate &source_coordinate,
                             const FixedPointCoordinate &target_coordinate,
                             const FixedPointCoordinate &query_location)
{
    double ratio;
    FixedPointCoordinate nearest_location;

    return perpendicularDistance(source_coordinate, target_coordinate, query_location,
                                 nearest_location, ratio);
}

double perpendicularDistance(const FixedPointCoordinate &segment_source,
                             const FixedPointCoordinate &segment_target,
                             const FixedPointCoordinate &query_location,
                             FixedPointCoordinate &nearest_location,
                             double &ratio)
{
    return perpendicularDistanceFromProjectedCoordinate(
        segment_source, segment_target, query_location,
        {mercator::lat2y(query_location.lat / COORDINATE_PRECISION),
         query_location.lon / COORDINATE_PRECISION},
        nearest_location, ratio);
}

double
perpendicularDistanceFromProjectedCoordinate(const FixedPointCoordinate &source_coordinate,
                                             const FixedPointCoordinate &target_coordinate,
                                             const FixedPointCoordinate &query_location,
                                             const std::pair<double, double> &projected_coordinate)
{
    double ratio;
    FixedPointCoordinate nearest_location;

    return perpendicularDistanceFromProjectedCoordinate(source_coordinate, target_coordinate,
                                                        query_location, projected_coordinate,
                                                        nearest_location, ratio);
}

double
perpendicularDistanceFromProjectedCoordinate(const FixedPointCoordinate &segment_source,
                                             const FixedPointCoordinate &segment_target,
                                             const FixedPointCoordinate &query_location,
                                             const std::pair<double, double> &projected_coordinate,
                                             FixedPointCoordinate &nearest_location,
                                             double &ratio)
{
    BOOST_ASSERT(query_location.IsValid());

    // initialize values
    const double x = projected_coordinate.first;
    const double y = projected_coordinate.second;
    const double a = mercator::lat2y(segment_source.lat / COORDINATE_PRECISION);
    const double b = segment_source.lon / COORDINATE_PRECISION;
    const double c = mercator::lat2y(segment_target.lat / COORDINATE_PRECISION);
    const double d = segment_target.lon / COORDINATE_PRECISION;
    double p, q /*,mX*/, new_y;
    if (std::abs(a - c) > std::numeric_limits<double>::epsilon())
    {
        const double m = (d - b) / (c - a); // slope
        // Projection of (x,y) on line joining (a,b) and (c,d)
        p = ((x + (m * y)) + (m * m * a - m * b)) / (1.0 + m * m);
        q = b + m * (p - a);
    }
    else
    {
        p = c;
        q = y;
    }
    new_y = (d * p - c * q) / (a * d - b * c);

    // discretize the result to coordinate precision. it's a hack!
    if (std::abs(new_y) < (1.0 / COORDINATE_PRECISION))
    {
        new_y = 0.0;
    }

    // compute ratio
    ratio = static_cast<double>((p - new_y * a) /
                                c); // These values are actually n/m+n and m/m+n , we need
    // not calculate the explicit values of m an n as we
    // are just interested in the ratio
    if (std::isnan(ratio))
    {
        ratio = (segment_target == query_location ? 1.0 : 0.0);
    }
    else if (std::abs(ratio) <= std::numeric_limits<double>::epsilon())
    {
        ratio = 0.0;
    }
    else if (std::abs(ratio - 1.0) <= std::numeric_limits<double>::epsilon())
    {
        ratio = 1.0;
    }

    // compute nearest location
    BOOST_ASSERT(!std::isnan(ratio));
    if (ratio <= 0.0)
    {
        nearest_location = segment_source;
    }
    else if (ratio >= 1.0)
    {
        nearest_location = segment_target;
    }
    else
    {
        // point lies in between
        nearest_location.lat = static_cast<int>(mercator::y2lat(p) * COORDINATE_PRECISION);
        nearest_location.lon = static_cast<int>(q * COORDINATE_PRECISION);
    }
    BOOST_ASSERT(nearest_location.IsValid());

    const double approximate_distance = greatCircleDistance(query_location, nearest_location);
    BOOST_ASSERT(0.0 <= approximate_distance);
    return approximate_distance;
}

double degToRad(const double degree) { return degree * (static_cast<double>(M_PI) / 180.0); }

double radToDeg(const double radian) { return radian * (180.0 * static_cast<double>(M_1_PI)); }

double bearing(const FixedPointCoordinate &first_coordinate,
               const FixedPointCoordinate &second_coordinate)
{
    const double lon_diff =
        second_coordinate.lon / COORDINATE_PRECISION - first_coordinate.lon / COORDINATE_PRECISION;
    const double lon_delta = degToRad(lon_diff);
    const double lat1 = degToRad(first_coordinate.lat / COORDINATE_PRECISION);
    const double lat2 = degToRad(second_coordinate.lat / COORDINATE_PRECISION);
    const double y = std::sin(lon_delta) * std::cos(lat2);
    const double x =
        std::cos(lat1) * std::sin(lat2) - std::sin(lat1) * std::cos(lat2) * std::cos(lon_delta);
    double result = radToDeg(std::atan2(y, x));
    while (result < 0.0)
    {
        result += 360.0;
    }

    while (result >= 360.0)
    {
        result -= 360.0;
    }
    return result;
}
}
}
}
