#include "util/coordinate.hpp"
#include "util/coordinate_calculation.hpp"
#include "util/trigonometry_table.hpp"
#include "util/web_mercator.hpp"

#include <boost/assert.hpp>

#include <cmath>

#include <limits>
#include <utility>

namespace osrm
{
namespace util
{

namespace coordinate_calculation
{

namespace
{
/// Newton-based Taubin circle algebraic fit in a plain.
/// Chernov, N. Circular and Linear Regression: Fitting Circles and Lines by Least Squares, 2010
/// Chapter 5.10 Implementation of the Taubin fit, p. 126
/// Original code http://people.cas.uab.edu/~mosya/cl/CircleFitByTaubin.cpp
bool circleCenterTaubin(const std::vector<Coordinate> &coords,
                        std::pair<double, double> &center,
                        double &estimated_radius)
{
    // guard against empty coordinates
    if( coords.empty() )
        return false;

    // Compute mean and covariances with the two-pass algorithm
    double meanX = 0., meanY = 0.;
    for (const auto &c : coords)
    {
        // Use the equirectangular projection
        const auto x = static_cast<double>(toFloating(c.lon));
        const auto y = static_cast<double>(toFloating(c.lat));
        meanX += x;
        meanY += y;
    }
    meanX /= coords.size();
    meanY /= coords.size();

    // Compute moments for centered x, y and z = x^2+y^2 coordinates
    double Mxx = 0., Myy = 0., Mxy = 0., Mxz = 0., Myz = 0., Mzz = 0.;
    for (auto &c : coords)
    {
        const auto Xi = static_cast<double>(toFloating(c.lon)) - meanX;
        const auto Yi = static_cast<double>(toFloating(c.lat)) - meanY;
        const auto Zi = Xi * Xi + Yi * Yi;

        Mxx += Xi * Xi;
        Myy += Yi * Yi;
        Mzz += Zi * Zi;
        Mxy += Xi * Yi;
        Mxz += Xi * Zi;
        Myz += Yi * Zi;
    }

    Mxx /= coords.size();
    Myy /= coords.size();
    Mxy /= coords.size();
    Mxz /= coords.size();
    Myz /= coords.size();
    Mzz /= coords.size();

    // Compute coefficients of the characteristic polynomial
    const auto Mz = Mxx + Myy;
    const auto cov_xy = Mxx * Myy - Mxy * Mxy;
    const auto var_z = Mzz - Mz * Mz;
    const auto c3 = 4. * Mz;
    const auto c2 = -3. * Mz * Mz - Mzz;
    const auto c1 = var_z * Mz + 4. * cov_xy * Mz - Mxz * Mxz - Myz * Myz;
    const auto c0 = Mxz * (Mxz * Myy - Myz * Mxy) + Myz * (Myz * Mxx - Mxz * Mxy) - var_z * cov_xy;

    // Find the root of the characteristic polynomial using Newton's method starting at x=0.
    // It is guaranteed to converge to the smallest characteristic value
    double x = 0., y = c0;
    const auto eps = sqrt(std::numeric_limits<double>::epsilon());
    for (int iter = 0; iter < 5; ++iter)
    {
        const auto dy = c1 + x * (2. * c2 + 3. * c3 * x);
        const auto xnew = x - y / dy;
        if (!std::isfinite(xnew))
            return false;
        if (std::abs(xnew - x) < eps)
            break;

        const auto ynew = c0 + xnew * (c1 + xnew * (c2 + xnew * c3));
        if (std::abs(ynew) >= std::abs(y))
            break;

        x = xnew;
        y = ynew;
    }

    // Computing parameters of the fitting circle
    const auto det = x * x - x * Mz + cov_xy;
    // Check if line is fitted
    if (det == 0.)
        return false;

    const auto centerX = (Mxz * (Myy - x) - Myz * Mxy) / det / 2.0; // a = -B/2/A
    const auto centerY = (Myz * (Mxx - x) - Mxz * Mxy) / det / 2.0; // b = -C/2/A

    // Translate the center
    center.first = meanX + centerX;
    center.second = meanY + centerY;

    // R^2 = (B^2 + C^2 - 4AD) / (4A^2) (3.11), D = -A Mz (5.49)
    estimated_radius = std::sqrt(centerX * centerX + centerY * centerY + Mz);

    return true;
}
} // namespace

// Does not project the coordinates!
std::uint64_t squaredEuclideanDistance(const Coordinate lhs, const Coordinate rhs)
{
    const std::uint64_t dx = static_cast<std::int32_t>(lhs.lon - rhs.lon);
    const std::uint64_t dy = static_cast<std::int32_t>(lhs.lat - rhs.lat);

    return dx * dx + dy * dy;
}

double haversineDistance(const Coordinate coordinate_1, const Coordinate coordinate_2)
{
    auto lon1 = static_cast<int>(coordinate_1.lon);
    auto lat1 = static_cast<int>(coordinate_1.lat);
    auto lon2 = static_cast<int>(coordinate_2.lon);
    auto lat2 = static_cast<int>(coordinate_2.lat);
    BOOST_ASSERT(lon1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lat1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon2 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lat2 != std::numeric_limits<int>::min());
    const double lt1 = lat1 / COORDINATE_PRECISION;
    const double ln1 = lon1 / COORDINATE_PRECISION;
    const double lt2 = lat2 / COORDINATE_PRECISION;
    const double ln2 = lon2 / COORDINATE_PRECISION;

    const double dlat1 = lt1 * detail::DEGREE_TO_RAD;
    const double dlong1 = ln1 * detail::DEGREE_TO_RAD;
    const double dlat2 = lt2 * detail::DEGREE_TO_RAD;
    const double dlong2 = ln2 * detail::DEGREE_TO_RAD;

    const double dlong = dlong1 - dlong2;
    const double dlat = dlat1 - dlat2;

    const double aharv = std::pow(std::sin(dlat / 2.0), 2.0) +
                         std::cos(dlat1) * std::cos(dlat2) * std::pow(std::sin(dlong / 2.), 2);
    const double charv = 2. * std::atan2(std::sqrt(aharv), std::sqrt(1.0 - aharv));
    return detail::EARTH_RADIUS * charv;
}

double greatCircleDistance(const Coordinate coordinate_1, const Coordinate coordinate_2)
{
    auto lon1 = static_cast<int>(coordinate_1.lon);
    auto lat1 = static_cast<int>(coordinate_1.lat);
    auto lon2 = static_cast<int>(coordinate_2.lon);
    auto lat2 = static_cast<int>(coordinate_2.lat);
    BOOST_ASSERT(lat1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lat2 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon2 != std::numeric_limits<int>::min());

    const double float_lat1 = (lat1 / COORDINATE_PRECISION) * detail::DEGREE_TO_RAD;
    const double float_lon1 = (lon1 / COORDINATE_PRECISION) * detail::DEGREE_TO_RAD;
    const double float_lat2 = (lat2 / COORDINATE_PRECISION) * detail::DEGREE_TO_RAD;
    const double float_lon2 = (lon2 / COORDINATE_PRECISION) * detail::DEGREE_TO_RAD;

    const double x_value = (float_lon2 - float_lon1) * std::cos((float_lat1 + float_lat2) / 2.0);
    const double y_value = float_lat2 - float_lat1;
    return std::hypot(x_value, y_value) * detail::EARTH_RADIUS;
}

double perpendicularDistance(const Coordinate segment_source,
                             const Coordinate segment_target,
                             const Coordinate query_location,
                             Coordinate &nearest_location,
                             double &ratio)
{
    using namespace coordinate_calculation;

    BOOST_ASSERT(query_location.IsValid());

    FloatCoordinate projected_nearest;
    std::tie(ratio, projected_nearest) =
        projectPointOnSegment(web_mercator::fromWGS84(segment_source),
                              web_mercator::fromWGS84(segment_target),
                              web_mercator::fromWGS84(query_location));
    nearest_location = web_mercator::toWGS84(projected_nearest);

    const double approximate_distance = greatCircleDistance(query_location, nearest_location);
    BOOST_ASSERT(0.0 <= approximate_distance);
    return approximate_distance;
}

double perpendicularDistance(const Coordinate source_coordinate,
                             const Coordinate target_coordinate,
                             const Coordinate query_location)
{
    double ratio;
    Coordinate nearest_location;

    return perpendicularDistance(
        source_coordinate, target_coordinate, query_location, nearest_location, ratio);
}

Coordinate centroid(const Coordinate lhs, const Coordinate rhs)
{
    Coordinate centroid;
    // The coordinates of the midpoints are given by:
    // x = (x1 + x2) /2 and y = (y1 + y2) /2.
    centroid.lon = (lhs.lon + rhs.lon) / FixedLongitude{2};
    centroid.lat = (lhs.lat + rhs.lat) / FixedLatitude{2};
    return centroid;
}

double degToRad(const double degree)
{
    using namespace boost::math::constants;
    return degree * (pi<double>() / 180.0);
}

double radToDeg(const double radian)
{
    using namespace boost::math::constants;
    return radian * (180.0 * (1. / pi<double>()));
}

double bearing(const Coordinate first_coordinate, const Coordinate second_coordinate)
{
    const double lon_diff =
        static_cast<double>(toFloating(second_coordinate.lon - first_coordinate.lon));
    const double lon_delta = degToRad(lon_diff);
    const double lat1 = degToRad(static_cast<double>(toFloating(first_coordinate.lat)));
    const double lat2 = degToRad(static_cast<double>(toFloating(second_coordinate.lat)));
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

double computeAngle(const Coordinate first, const Coordinate second, const Coordinate third)
{
    using namespace boost::math::constants;
    using namespace coordinate_calculation;

    if (first == second || second == third)
        return 180;

    BOOST_ASSERT(first.IsValid());
    BOOST_ASSERT(second.IsValid());
    BOOST_ASSERT(third.IsValid());

    const double v1x = static_cast<double>(toFloating(first.lon - second.lon));
    const double v1y =
        web_mercator::latToY(toFloating(first.lat)) - web_mercator::latToY(toFloating(second.lat));
    const double v2x = static_cast<double>(toFloating(third.lon - second.lon));
    const double v2y =
        web_mercator::latToY(toFloating(third.lat)) - web_mercator::latToY(toFloating(second.lat));

    double angle = (atan2_lookup(v2y, v2x) - atan2_lookup(v1y, v1x)) * 180. / pi<double>();

    while (angle < 0.)
    {
        angle += 360.;
    }

    BOOST_ASSERT(angle >= 0);
    return angle;
}

boost::optional<Coordinate> circleCenter(const std::vector<Coordinate> &coords)
{
    std::pair<double, double> center;
    double radius;

    if (!circleCenterTaubin(coords, center, radius))
        return boost::none;

    const double lon = center.first, lat = center.second;
    if (lon < -180.0 || lon > 180.0 || lat < -90.0 || lat > 90.0)
        return boost::none;
    else
        return Coordinate(FloatLongitude{lon}, FloatLatitude{lat});
}

double circleRadius(const std::vector<Coordinate> &coords)
{
    switch (coords.size())
    {
    case 0:
        return std::numeric_limits<double>::quiet_NaN();
    case 1:
        return 0.;
    case 2:
        return 0.5 * haversineDistance(coords[0], coords[1]);
    default:
        break;
    }

    std::pair<double, double> center;
    double radius;

    if (!circleCenterTaubin(coords, center, radius))
        return std::numeric_limits<double>::infinity();

    const double lon = center.first, lat = center.second;
    if (lon < -180.0 || lon > 180.0 || lat < -90.0 || lat > 90.0)
        return std::numeric_limits<double>::infinity();

    // The estimated radius corresponds to the arc angle in degrees, convert it to meters
    return radius * detail::DEGREE_TO_RAD * detail::EARTH_RADIUS;
}

Coordinate interpolateLinear(double factor, const Coordinate from, const Coordinate to)
{
    BOOST_ASSERT(0 <= factor && factor <= 1.0);

    const auto from_lon = static_cast<std::int32_t>(from.lon);
    const auto from_lat = static_cast<std::int32_t>(from.lat);
    const auto to_lon = static_cast<std::int32_t>(to.lon);
    const auto to_lat = static_cast<std::int32_t>(to.lat);

    FixedLongitude interpolated_lon{
        static_cast<std::int32_t>(from_lon + factor * (to_lon - from_lon))};
    FixedLatitude interpolated_lat{
        static_cast<std::int32_t>(from_lat + factor * (to_lat - from_lat))};

    return {std::move(interpolated_lon), std::move(interpolated_lat)};
}

} // ns coordinate_calculation
} // ns util
} // ns osrm
