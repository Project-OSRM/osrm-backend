#ifndef COORDINATE_CALCULATION
#define COORDINATE_CALCULATION

struct FixedPointCoordinate;

#include <string>
#include <utility>

namespace coordinate_calculation
{
    double
    haversine_distance(const int lat1, const int lon1, const int lat2, const int lon2);

    double haversine_distance(const FixedPointCoordinate &first_coordinate,
                                 const FixedPointCoordinate &second_coordinate);

    double great_circle_distance(const FixedPointCoordinate &first_coordinate,
                             const FixedPointCoordinate &second_coordinate);

    double great_circle_distance(const int lat1, const int lon1, const int lat2, const int lon2);

    void lat_or_lon_to_string(const int value, std::string &output);

    double perpendicular_distance(const FixedPointCoordinate &segment_source,
                                 const FixedPointCoordinate &segment_target,
                                 const FixedPointCoordinate &query_location);

    double perpendicular_distance(const FixedPointCoordinate &segment_source,
                                 const FixedPointCoordinate &segment_target,
                                 const FixedPointCoordinate &query_location,
                                 FixedPointCoordinate &nearest_location,
                                 double &ratio);

    double perpendicular_distance_from_projected_coordinate(
        const FixedPointCoordinate &segment_source,
        const FixedPointCoordinate &segment_target,
        const FixedPointCoordinate &query_location,
        const std::pair<double, double> &projected_coordinate);

    double perpendicular_distance_from_projected_coordinate(
        const FixedPointCoordinate &segment_source,
        const FixedPointCoordinate &segment_target,
        const FixedPointCoordinate &query_location,
        const std::pair<double, double> &projected_coordinate,
        FixedPointCoordinate &nearest_location,
        double &ratio);

    double deg_to_rad(const double degree);
    double rad_to_deg(const double radian);

    double bearing(const FixedPointCoordinate &first_coordinate,
                  const FixedPointCoordinate &second_coordinate);
}

#endif // COORDINATE_CALCULATION
