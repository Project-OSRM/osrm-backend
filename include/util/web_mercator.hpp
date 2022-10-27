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
const constexpr double DEGREE_TO_RAD = 0.017453292519943295769236907684886;
const constexpr double RAD_TO_DEGREE = 1. / DEGREE_TO_RAD;
// radius used by WGS84
const constexpr double EARTH_RADIUS_WGS84 = 6378137.0;
// earth circumference devided by 2
const constexpr double MAXEXTENT = EARTH_RADIUS_WGS84 * boost::math::constants::pi<double>();
// ^ math functions are not constexpr since they have side-effects (setting errno) :(
const constexpr double EPSG3857_MAX_LATITUDE = 85.051128779806592378; // 90(4*atan(exp(pi))/pi-1)
const constexpr double MAX_LONGITUDE = 180.0;
} // namespace detail

// Converts projected mercator degrees to PX
const constexpr double DEGREE_TO_PX = detail::MAXEXTENT / 180.0;
// This is the global default tile size for all Mapbox Vector Tiles
const constexpr double TILE_SIZE = 256.0;

inline FloatLatitude clamp(const FloatLatitude lat)
{
    return std::max(std::min(lat, FloatLatitude{detail::EPSG3857_MAX_LATITUDE}),
                    FloatLatitude{-detail::EPSG3857_MAX_LATITUDE});
}

inline FloatLongitude clamp(const FloatLongitude lon)
{
    return std::max(std::min(lon, FloatLongitude{detail::MAX_LONGITUDE}),
                    FloatLongitude{-detail::MAX_LONGITUDE});
}

inline FloatLatitude yToLat(const double y)
{
    const auto clamped_y = std::max(-180., std::min(180., y));
    const double normalized_lat =
        detail::RAD_TO_DEGREE * 2. * std::atan(std::exp(clamped_y * detail::DEGREE_TO_RAD));

    return FloatLatitude{normalized_lat - 90.};
}

inline double latToY(const FloatLatitude latitude)
{
    // apparently this is the (faster) version of the canonical log(tan()) version
    const auto clamped_latitude = clamp(latitude);
    const double f = std::sin(detail::DEGREE_TO_RAD * static_cast<double>(clamped_latitude));
    return detail::RAD_TO_DEGREE * 0.5 * std::log((1 + f) / (1 - f));
}

template <typename T> constexpr double horner(double, T an) { return an; }

template <typename T, typename... U> constexpr double horner(double x, T an, U... a)
{
    return horner(x, a...) * x + an;
}

inline double latToYapprox(const FloatLatitude latitude)
{
    if (latitude < FloatLatitude{-70.} || latitude > FloatLatitude{70.})
        return latToY(latitude);

    // Approximate the inverse Gudermannian function with the Padé approximant [11/11]: deg → deg
    // Coefficients are computed for the argument range [-70°,70°] by Remez algorithm
    // |err|_∞=3.387e-12
    const auto x = static_cast<double>(latitude);
    return horner(x,
                  0.00000000000000000000000000e+00,
                  1.00000000000089108431373566e+00,
                  2.34439410386997223035693483e-06,
                  -3.21291701673364717170998957e-04,
                  -6.62778508496089940141103135e-10,
                  3.68188055470304769936079078e-08,
                  6.31192702320492485752941578e-14,
                  -1.77274453235716299127325443e-12,
                  -2.24563810831776747318521450e-18,
                  3.13524754818073129982475171e-17,
                  2.09014225025314211415458228e-23,
                  -9.82938075991732185095509716e-23) /
           horner(x,
                  1.00000000000000000000000000e+00,
                  2.34439410398970701719081061e-06,
                  -3.72061271627251952928813333e-04,
                  -7.81802389685429267252612620e-10,
                  5.18418724186576447072888605e-08,
                  9.37468561198098681003717477e-14,
                  -3.30833288607921773936702558e-12,
                  -4.78446279888774903983338274e-18,
                  9.32999229169156878168234191e-17,
                  9.17695141954265959600965170e-23,
                  -8.72130728982012387640166055e-22,
                  -3.23083224835967391884404730e-28);
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
    return {wgs84_coordinate.lon, FloatLatitude{latToYapprox(wgs84_coordinate.lat)}};
}

inline FloatCoordinate toWGS84(const FloatCoordinate &mercator_coordinate)
{
    return {mercator_coordinate.lon, yToLat(static_cast<double>(mercator_coordinate.lat))};
}

// Converts a WMS tile coordinate (z,x,y) into a wgs bounding box
inline void xyzToWGS84(const int x,
                       const int y,
                       const int z,
                       double &minx,
                       double &miny,
                       double &maxx,
                       double &maxy,
                       int mercator_buffer = 0)
{
    minx = x * TILE_SIZE - mercator_buffer;
    miny = (y + 1.0) * TILE_SIZE + mercator_buffer;
    maxx = (x + 1.0) * TILE_SIZE + mercator_buffer;
    maxy = y * TILE_SIZE - mercator_buffer;
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

    minx = static_cast<double>(clamp(util::FloatLongitude{minx})) * DEGREE_TO_PX;
    miny = latToY(util::FloatLatitude{miny}) * DEGREE_TO_PX;
    maxx = static_cast<double>(clamp(util::FloatLongitude{maxx})) * DEGREE_TO_PX;
    maxy = latToY(util::FloatLatitude{maxy}) * DEGREE_TO_PX;
}
} // namespace web_mercator
} // namespace util
} // namespace osrm

#endif
