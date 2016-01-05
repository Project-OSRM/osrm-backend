#ifndef COORDINATE_CALCULATION
#define COORDINATE_CALCULATION

#include <string>
#include <utility>

namespace osrm
{
namespace util
{

struct FixedPointCoordinate;

namespace coordinate_calculation
{
double haversineDistance(const int lat1, const int lon1, const int lat2, const int lon2);

double haversineDistance(const FixedPointCoordinate &first_coordinate,
                         const FixedPointCoordinate &second_coordinate);

double greatCircleDistance(const FixedPointCoordinate &first_coordinate,
                           const FixedPointCoordinate &second_coordinate);

double greatCircleDistance(const int lat1, const int lon1, const int lat2, const int lon2);

double perpendicularDistance(const FixedPointCoordinate &segment_source,
                             const FixedPointCoordinate &segment_target,
                             const FixedPointCoordinate &query_location);

double perpendicularDistance(const FixedPointCoordinate &segment_source,
                             const FixedPointCoordinate &segment_target,
                             const FixedPointCoordinate &query_location,
                             FixedPointCoordinate &nearest_location,
                             double &ratio);

double
perpendicularDistanceFromProjectedCoordinate(const FixedPointCoordinate &segment_source,
                                             const FixedPointCoordinate &segment_target,
                                             const FixedPointCoordinate &query_location,
                                             const std::pair<double, double> &projected_coordinate);

double
perpendicularDistanceFromProjectedCoordinate(const FixedPointCoordinate &segment_source,
                                             const FixedPointCoordinate &segment_target,
                                             const FixedPointCoordinate &query_location,
                                             const std::pair<double, double> &projected_coordinate,
                                             FixedPointCoordinate &nearest_location,
                                             double &ratio);

double degToRad(const double degree);
double radToDeg(const double radian);

double bearing(const FixedPointCoordinate &first_coordinate,
               const FixedPointCoordinate &second_coordinate);
}

}
}

#endif // COORDINATE_CALCULATION
