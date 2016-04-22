#ifndef OSRM_WEB_MERCATOR_HPP
#define OSRM_WEB_MERCATOR_HPP

#include "util/coordinate.hpp"

#include <boost/math/constants/constants.hpp>

namespace osrm
{
namespace util
{
namespace web_mercator
{
namespace detail
{
const constexpr long double DEGREE_TO_RAD = 0.017453292519943295769236907684886;
const constexpr long double RAD_TO_DEGREE = 1. / DEGREE_TO_RAD;
// radius used by WGS84
const constexpr double EARTH_RADIUS_WGS84 = 6378137.0;
// earth circumference devided by 2
const constexpr double MAXEXTENT = EARTH_RADIUS_WGS84 * boost::math::constants::pi<double>();
// ^ math functions are not constexpr since they have side-effects (setting errno) :(
const constexpr double MAX_LATITUDE = 85.;
const constexpr double MAX_LONGITUDE = 180.0;
}

// Converts projected mercator degrees to PX
const constexpr double DEGREE_TO_PX = detail::MAXEXTENT / 180.0;
// This is the global default tile size for all Mapbox Vector Tiles
const constexpr double TILE_SIZE = 256.0;

inline FloatLatitude yToLat(const double y)
{
    const auto clamped_y = std::max(-180., std::min(180., y));
    const double normalized_lat =
        detail::RAD_TO_DEGREE * 2. * std::atan(std::exp(clamped_y * detail::DEGREE_TO_RAD));

    return FloatLatitude(normalized_lat - 90.);
}

inline double latToY(const FloatLatitude latitude)
{
    // apparently this is the (faster) version of the canonical log(tan()) version
    const double f = std::sin(detail::DEGREE_TO_RAD * static_cast<double>(latitude));
    const double y = detail::RAD_TO_DEGREE * 0.5 * std::log((1 + f) / (1 - f));
    const auto clamped_y = std::max(-180., std::min(180., y));
    return clamped_y;
}

inline FloatLatitude clamp(const FloatLatitude lat)
{
    return std::max(std::min(lat, FloatLatitude(detail::MAX_LATITUDE)),
                    FloatLatitude(-detail::MAX_LATITUDE));
}

inline FloatLongitude clamp(const FloatLongitude lon)
{
    return std::max(std::min(lon, FloatLongitude(detail::MAX_LONGITUDE)),
                    FloatLongitude(-detail::MAX_LONGITUDE));
}

inline void pixelToDegree(const double shift, double &x, double &y)
{
    const double b = shift / 2.0;
    x = (x - b) / shift * 360.0;
    // FIXME needs to be simplified
    const double g = (y - b) / -(shift / (2 * M_PI)) / detail::DEGREE_TO_RAD;
    static_assert(detail::DEGREE_TO_RAD / (2 * M_PI) - 1 / 360. < 0.0001, "");
    y = static_cast<double>(yToLat(g));
}

inline double degreeToPixel(FloatLongitude lon, unsigned zoom)
{
    const double shift = (1u << zoom) * TILE_SIZE;
    const double b = shift / 2.0;
    const double x = b * (1 + static_cast<double>(lon) / 180.0);
    return x;
}

inline double degreeToPixel(FloatLatitude lat, unsigned zoom)
{
    const double shift = (1u << zoom) * TILE_SIZE;
    const double b = shift / 2.0;
    const double y = b * (1. - latToY(lat) / 180.);
    return y;
}

inline FloatCoordinate fromWGS84(const FloatCoordinate &wgs84_coordinate)
{
    return {wgs84_coordinate.lon, FloatLatitude{latToY(wgs84_coordinate.lat)}};
}

inline FloatCoordinate toWGS84(const FloatCoordinate &mercator_coordinate)
{
    return {mercator_coordinate.lon, yToLat(static_cast<double>(mercator_coordinate.lat))};
}

// Converts a WMS tile coordinate (z,x,y) into a wgs bounding box
inline void xyzToWGS84(
    const int x, const int y, const int z, double &minx, double &miny, double &maxx, double &maxy)
{
    minx = x * TILE_SIZE;
    miny = (y + 1.0) * TILE_SIZE;
    maxx = (x + 1.0) * TILE_SIZE;
    maxy = y * TILE_SIZE;
    // 2^z * TILE_SIZE
    const double shift = (1u << static_cast<unsigned>(z)) * TILE_SIZE;
    pixelToDegree(shift, minx, miny);
    pixelToDegree(shift, maxx, maxy);
}

// Converts a WMS tile coordinate (z,x,y) into a mercator bounding box
inline void xyzToMercator(
    const int x, const int y, const int z, double &minx, double &miny, double &maxx, double &maxy)
{
    xyzToWGS84(x, y, z, minx, miny, maxx, maxy);

    minx = static_cast<double>(clamp(util::FloatLongitude(minx))) * DEGREE_TO_PX;
    miny = latToY(clamp(util::FloatLatitude(miny))) * DEGREE_TO_PX;
    maxx = static_cast<double>(clamp(util::FloatLongitude(maxx))) * DEGREE_TO_PX;
    maxy = latToY(clamp(util::FloatLatitude(maxy))) * DEGREE_TO_PX;
}
}
}
}

#endif
