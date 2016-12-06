#ifndef COORDINATE_CALCULATION
#define COORDINATE_CALCULATION

#include "util/coordinate.hpp"

#include <boost/math/constants/constants.hpp>
#include <boost/optional.hpp>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <utility>
#include <vector>

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

inline double degToRad(const double degree)
{
    using namespace boost::math::constants;
    return degree * (pi<double>() / 180.0);
}

inline double radToDeg(const double radian)
{
    using namespace boost::math::constants;
    return radian * (180.0 * (1. / pi<double>()));
}
}

//! Takes the squared euclidean distance of the input coordinates. Does not return meters!
std::uint64_t squaredEuclideanDistance(const Coordinate lhs, const Coordinate rhs);

double haversineDistance(const Coordinate first_coordinate, const Coordinate second_coordinate);

double greatCircleDistance(const Coordinate first_coordinate, const Coordinate second_coordinate);

// get the length of a full coordinate vector, using one of our basic functions to compute distances
template <class BinaryOperation, typename iterator_type>
double getLength(iterator_type begin, const iterator_type end, BinaryOperation op);

// Find the closest distance and location between coordinate and the line connecting source and
// target:
//             coordinate
//                 |
//                 |
// source -------- x -------- target.
// returns x as well as the distance between source and x as ratio ([0,1])
inline std::pair<double, FloatCoordinate> projectPointOnSegment(const FloatCoordinate &source,
                                                                const FloatCoordinate &target,
                                                                const FloatCoordinate &coordinate);

// find the closest distance between a coordinate and a segment
// O(1)
double findClosestDistance(const Coordinate coordinate,
                           const Coordinate segment_begin,
                           const Coordinate segment_end);

// find the closest distance between a coordinate and a set of coordinates
// O(|coordinates|)
template <typename iterator_type>
double findClosestDistance(const Coordinate coordinate,
                           const iterator_type begin,
                           const iterator_type end);

// find the closes distance between two sets of coordinates
// O(|lhs| * |rhs|)
template <typename iterator_type>
double findClosestDistance(const iterator_type lhs_begin,
                           const iterator_type lhs_end,
                           const iterator_type rhs_begin,
                           const iterator_type rhs_end);

// checks if two sets of coordinates describe a parallel set of ways
template <typename iterator_type>
bool areParallel(const iterator_type lhs_begin,
                 const iterator_type lhs_end,
                 const iterator_type rhs_begin,
                 const iterator_type rhs_end);

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

template <typename iterator_type>
std::pair<Coordinate, Coordinate> leastSquareRegression(const iterator_type begin,
                                                        const iterator_type end);

// rotates a coordinate around the point (0,0). This function can be used to normalise a few
// computations around regression vectors
Coordinate rotateCCWAroundZero(Coordinate coordinate, double angle_in_radians);

// compute the difference vector of two coordinates lhs - rhs
Coordinate difference(const Coordinate lhs, const Coordinate rhs);

// TEMPLATE/INLINE DEFINITIONS
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

template <class BinaryOperation, typename iterator_type>
double getLength(iterator_type begin, const iterator_type end, BinaryOperation op)
{
    double result = 0;
    const auto functor = [&result, op](const Coordinate lhs, const Coordinate rhs) {
        result += op(lhs, rhs);
        return false;
    };
    // side-effect find adding up distances
    std::adjacent_find(begin, end, functor);

    return result;
}

template <typename iterator_type>
double
findClosestDistance(const Coordinate coordinate, const iterator_type begin, const iterator_type end)
{
    double current_min = std::numeric_limits<double>::max();

    // comparator updating current_min without ever finding an element
    const auto compute_minimum_distance = [&current_min, coordinate](const Coordinate lhs,
                                                                     const Coordinate rhs) {
        current_min = std::min(current_min, findClosestDistance(coordinate, lhs, rhs));
        return false;
    };

    std::adjacent_find(begin, end, compute_minimum_distance);
    return current_min;
}

template <typename iterator_type>
double findClosestDistance(const iterator_type lhs_begin,
                           const iterator_type lhs_end,
                           const iterator_type rhs_begin,
                           const iterator_type rhs_end)
{
    double current_min = std::numeric_limits<double>::max();

    const auto compute_minimum_distance_in_rhs = [&current_min, rhs_begin, rhs_end](
        const Coordinate coordinate) {
        current_min = std::min(current_min, findClosestDistance(coordinate, rhs_begin, rhs_end));
        return false;
    };

    std::find_if(lhs_begin, lhs_end, compute_minimum_distance_in_rhs);
    return current_min;
}

template <typename iterator_type>
std::pair<Coordinate, Coordinate> leastSquareRegression(const iterator_type begin,
                                                        const iterator_type end)
{
    // following the formulas of https://faculty.elgin.edu/dkernler/statistics/ch04/4-2.html
    const auto number_of_coordinates = std::distance(begin, end);
    BOOST_ASSERT(number_of_coordinates >= 2);
    const auto extract_lon = [](const Coordinate coordinate) {
        return static_cast<double>(toFloating(coordinate.lon));
    };

    const auto extract_lat = [](const Coordinate coordinate) {
        return static_cast<double>(toFloating(coordinate.lat));
    };

    double min_lon = extract_lon(*begin);
    double max_lon = extract_lon(*begin);
    double min_lat = extract_lat(*begin);
    double max_lat = extract_lat(*begin);

    for (auto coordinate_iterator = begin; coordinate_iterator != end; ++coordinate_iterator)
    {
        const auto c = *coordinate_iterator;
        const auto lon = extract_lon(c);
        min_lon = std::min(min_lon, lon);
        max_lon = std::max(max_lon, lon);
        const auto lat = extract_lat(c);
        min_lat = std::min(min_lat, lat);
        max_lat = std::max(max_lat, lat);
    }
    // very small difference in longitude -> would result in inaccurate calculation, check if lat is
    // better
    if ((max_lat - min_lat) > 2 * (max_lon - min_lon))
    {
        std::vector<util::Coordinate> rotated_coordinates(number_of_coordinates);
        // rotate all coordinates to the right
        std::transform(begin, end, rotated_coordinates.begin(), [](const auto coordinate) {
            return rotateCCWAroundZero(coordinate, detail::degToRad(-90));
        });
        const auto rotated_regression =
            leastSquareRegression(rotated_coordinates.begin(), rotated_coordinates.end());
        return {rotateCCWAroundZero(rotated_regression.first, detail::degToRad(90)),
                rotateCCWAroundZero(rotated_regression.second, detail::degToRad(90))};
    }

    const auto make_accumulate = [](const auto extraction_function) {
        return [extraction_function](const double sum_so_far, const Coordinate coordinate) {
            return sum_so_far + extraction_function(coordinate);
        };
    };

    const auto accumulated_lon = std::accumulate(begin, end, 0., make_accumulate(extract_lon));

    const auto accumulated_lat = std::accumulate(begin, end, 0., make_accumulate(extract_lat));

    const auto mean_lon = accumulated_lon / number_of_coordinates;
    const auto mean_lat = accumulated_lat / number_of_coordinates;
    const auto make_variance = [](const auto mean, const auto extraction_function) {
        return [extraction_function, mean](const double sum_so_far, const Coordinate coordinate) {
            const auto difference = extraction_function(coordinate) - mean;
            return sum_so_far + difference * difference;
        };
    };

    // using the unbiased version, we divide by num_samples - 1 (see
    // http://mathworld.wolfram.com/SampleVariance.html)
    const auto sample_variance_lon =
        std::sqrt(std::accumulate(begin, end, 0., make_variance(mean_lon, extract_lon)) /
                  (number_of_coordinates - 1));

    // if we don't change longitude, return the vertical line as is
    if (std::abs(sample_variance_lon) <
        std::numeric_limits<decltype(sample_variance_lon)>::epsilon())
        return {*begin, *(end - 1)};

    const auto sample_variance_lat =
        std::sqrt(std::accumulate(begin, end, 0., make_variance(mean_lat, extract_lat)) /
                  (number_of_coordinates - 1));

    if (std::abs(sample_variance_lat) <
        std::numeric_limits<decltype(sample_variance_lat)>::epsilon())
        return {*begin, *(end - 1)};
    const auto linear_correlation =
        std::accumulate(begin,
                        end,
                        0.,
                        [&](const auto sum_so_far, const auto current_coordinate) {
                            return sum_so_far +
                                   (extract_lon(current_coordinate) - mean_lon) *
                                       (extract_lat(current_coordinate) - mean_lat) /
                                       (sample_variance_lon * sample_variance_lat);
                        }) /
        (number_of_coordinates - 1);

    const auto slope = linear_correlation * sample_variance_lat / sample_variance_lon;
    const auto intercept = mean_lat - slope * mean_lon;

    const auto GetLatAtLon = [intercept,
                              slope](const util::FloatLongitude longitude) -> util::FloatLatitude {
        return {intercept + slope * static_cast<double>((longitude))};
    };

    const double offset = 0.00001;
    const Coordinate regression_first = {
        toFixed(util::FloatLongitude{min_lon - offset}),
        toFixed(util::FloatLatitude(GetLatAtLon(util::FloatLongitude{min_lon - offset})))};
    const Coordinate regression_end = {
        toFixed(util::FloatLongitude{max_lon + offset}),
        toFixed(util::FloatLatitude(GetLatAtLon(util::FloatLongitude{max_lon + offset})))};

    return {regression_first, regression_end};
}

template <typename iterator_type>
bool areParallel(const iterator_type lhs_begin,
                 const iterator_type lhs_end,
                 const iterator_type rhs_begin,
                 const iterator_type rhs_end)
{
    const auto regression_lhs = leastSquareRegression(lhs_begin, lhs_end);
    const auto regression_rhs = leastSquareRegression(rhs_begin, rhs_end);

    const auto null_island = Coordinate(FixedLongitude{0}, FixedLatitude{0});
    const auto difference_lhs = difference(regression_lhs.first, regression_lhs.second);
    const auto difference_rhs = difference(regression_rhs.first, regression_rhs.second);

    // we normalise the left slope to be zero, so we rotate the coordinates around 0,0 to match 90
    // degrees
    const auto bearing_lhs = bearing(null_island, difference_lhs);

    // we rotate to have one of the lines facing horizontally to the right (bearing 90 degree)
    const auto rotation_angle_radians = detail::degToRad(bearing_lhs - 90);
    const auto rotated_difference_rhs = rotateCCWAroundZero(difference_rhs, rotation_angle_radians);

    const auto get_slope = [](const Coordinate from, const Coordinate to) {
        const auto diff_lat = static_cast<int>(from.lat) - static_cast<int>(to.lat);
        const auto diff_lon = static_cast<int>(from.lon) - static_cast<int>(to.lon);
        if (diff_lon == 0)
            return std::numeric_limits<double>::max();
        return static_cast<double>(diff_lat) / static_cast<double>(diff_lon);
    };

    const auto slope_rhs = get_slope(null_island, rotated_difference_rhs);
    // the left hand side has a slope of `0` after the rotation. We can check the slope of the right
    // hand side to ensure we only considering slight slopes
    return std::abs(slope_rhs) < 0.20; // twenty percent incline at the most
}

} // ns coordinate_calculation
} // ns util
} // ns osrm

#endif // COORDINATE_CALCULATION
