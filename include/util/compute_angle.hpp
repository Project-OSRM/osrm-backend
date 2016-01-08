#ifndef COMPUTE_ANGLE_HPP
#define COMPUTE_ANGLE_HPP

#include "osrm/coordinate.hpp"
#include "util/trigonometry_table.hpp"
#include "util/mercator.hpp"

namespace osrm
{
namespace util
{

// Get angle of line segment (A,C)->(C,B)
inline double ComputeAngle(const FixedPointCoordinate first,
                           const FixedPointCoordinate second,
                           const FixedPointCoordinate third) noexcept
{
    const double v1x = (first.lon - second.lon) / COORDINATE_PRECISION;
    const double v1y = mercator::lat2y(first.lat / COORDINATE_PRECISION) -
                       mercator::lat2y(second.lat / COORDINATE_PRECISION);
    const double v2x = (third.lon - second.lon) / COORDINATE_PRECISION;
    const double v2y = mercator::lat2y(third.lat / COORDINATE_PRECISION) -
                       mercator::lat2y(second.lat / COORDINATE_PRECISION);

    double angle = (atan2_lookup(v2y, v2x) - atan2_lookup(v1y, v1x)) * 180. / M_PI;

    while (angle < 0.)
    {
        angle += 360.;
    }

    return angle;
}
}
}

#endif // COMPUTE_ANGLE_HPP
