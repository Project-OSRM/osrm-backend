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

std::pair<double, FloatCoordinate> projectPointOnSegment(const FloatCoordinate &source,
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
                FloatLongitude(1.0 - clamped_ratio) * source.lon +
                    target.lon * FloatLongitude(clamped_ratio),
                FloatLatitude(1.0 - clamped_ratio) * source.lat +
                    target.lat * FloatLatitude(clamped_ratio),
            }};
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
    std::tie(ratio, projected_nearest) = projectPointOnSegment(
        web_mercator::fromWGS84(segment_source), web_mercator::fromWGS84(segment_target),
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

    return perpendicularDistance(source_coordinate, target_coordinate, query_location,
                                 nearest_location, ratio);
}

Coordinate centroid(const Coordinate lhs, const Coordinate rhs)
{
    Coordinate centroid;
    // The coordinates of the midpoints are given by:
    // x = (x1 + x2) /2 and y = (y1 + y2) /2.
    centroid.lon = (lhs.lon + rhs.lon) / FixedLongitude(2);
    centroid.lat = (lhs.lat + rhs.lat) / FixedLatitude(2);
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

boost::optional<Coordinate>
circleCenter(const Coordinate C1, const Coordinate C2, const Coordinate C3)
{
    // free after http://paulbourke.net/geometry/circlesphere/
    // require three distinct points
    if (C1 == C2 || C2 == C3 || C1 == C3)
    {
        return boost::none;
    }

    // define line through c1, c2 and c2,c3
    const double C2C1_lat = static_cast<double>(toFloating(C2.lat - C1.lat)); // yDelta_a
    const double C2C1_lon = static_cast<double>(toFloating(C2.lon - C1.lon)); // xDelta_a
    const double C3C2_lat = static_cast<double>(toFloating(C3.lat - C2.lat)); // yDelta_b
    const double C3C2_lon = static_cast<double>(toFloating(C3.lon - C2.lon)); // xDelta_b

    // check for collinear points in X-Direction / Y-Direction
    if ((std::abs(C2C1_lon) < std::numeric_limits<double>::epsilon() &&
         std::abs(C3C2_lon) < std::numeric_limits<double>::epsilon()) ||
        (std::abs(C2C1_lat) < std::numeric_limits<double>::epsilon() &&
         std::abs(C3C2_lat) < std::numeric_limits<double>::epsilon()))
    {
        return boost::none;
    }
    else if (std::abs(C2C1_lon) < std::numeric_limits<double>::epsilon())
    {
        // vertical line C2C1
        // due to c1.lon == c2.lon && c1.lon != c3.lon we can rearrange this way
        BOOST_ASSERT(std::abs(static_cast<double>(toFloating(C3.lon - C1.lon))) >=
                         std::numeric_limits<double>::epsilon() &&
                     std::abs(static_cast<double>(toFloating(C2.lon - C3.lon))) >=
                         std::numeric_limits<double>::epsilon());
        return circleCenter(C1, C3, C2);
    }
    else if (std::abs(C3C2_lon) < std::numeric_limits<double>::epsilon())
    {
        // vertical line C3C2
        // due to c2.lon == c3.lon && c1.lon != c3.lon we can rearrange this way
        // after rearrangement both deltas will be zero
        BOOST_ASSERT(std::abs(static_cast<double>(toFloating(C1.lon - C2.lon))) >=
                         std::numeric_limits<double>::epsilon() &&
                     std::abs(static_cast<double>(toFloating(C3.lon - C1.lon))) >=
                         std::numeric_limits<double>::epsilon());
        return circleCenter(C2, C1, C3);
    }
    else
    {
        const double C2C1_slope = C2C1_lat / C2C1_lon;
        const double C3C2_slope = C3C2_lat / C3C2_lon;

        if (std::abs(C2C1_slope) < std::numeric_limits<double>::epsilon())
        {
            // Three non-collinear points with C2,C1 on same latitude.
            // Due to the x-values correct, we can swap C3 and C1 to obtain the correct slope value
            return circleCenter(C3, C2, C1);
        }
        // valid slope values for both lines, calculate the center as intersection of the lines

        // can this ever happen?
        if (std::abs(C2C1_slope - C3C2_slope) < std::numeric_limits<double>::epsilon())
            return boost::none;

        const double C1_y = static_cast<double>(toFloating(C1.lat));
        const double C1_x = static_cast<double>(toFloating(C1.lon));
        const double C2_y = static_cast<double>(toFloating(C2.lat));
        const double C2_x = static_cast<double>(toFloating(C2.lon));
        const double C3_y = static_cast<double>(toFloating(C3.lat));
        const double C3_x = static_cast<double>(toFloating(C3.lon));

        const double lon = (C2C1_slope * C3C2_slope * (C1_y - C3_y) + C3C2_slope * (C1_x + C2_x) -
                            C2C1_slope * (C2_x + C3_x)) /
                           (2 * (C3C2_slope - C2C1_slope));
        const double lat = (0.5 * (C1_x + C2_x) - lon) / C2C1_slope + 0.5 * (C1_y + C2_y);
        return Coordinate(FloatLongitude(lon), FloatLatitude(lat));
    }
}

double circleRadius(const Coordinate C1, const Coordinate C2, const Coordinate C3)
{
    // a circle by three points requires thee distinct points
    auto center = circleCenter(C1, C2, C3);
    if (center)
        return haversineDistance(C1, *center);
    else
        return std::numeric_limits<double>::infinity();
}

Coordinate interpolateLinear(double factor, const Coordinate from, const Coordinate to)
{
    BOOST_ASSERT(0 <= factor && factor <= 1.0);

    FixedLongitude interpolated_lon(((1. - factor) * static_cast<std::int32_t>(from.lon)) +
                                    (factor * static_cast<std::int32_t>(to.lon)));
    FixedLatitude interpolated_lat(((1. - factor) * static_cast<std::int32_t>(from.lat)) +
                                   (factor * static_cast<std::int32_t>(to.lat)));

    return {std::move(interpolated_lon), std::move(interpolated_lat)};
}

} // ns coordinate_calculation
} // ns util
} // ns osrm
