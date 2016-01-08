#ifndef COMPUTE_ANGLE_HPP
#define COMPUTE_ANGLE_HPP

namespace osrm
{
namespace util
{

struct FixedPointCoordinate;

struct ComputeAngle
{
    // Get angle of line segment (A,C)->(C,B)
    // atan2 magic, formerly cosine theorem
    static double OfThreeFixedPointCoordinates(const FixedPointCoordinate &first,
                                               const FixedPointCoordinate &second,
                                               const FixedPointCoordinate &third) noexcept;
};
}
}

#endif // COMPUTE_ANGLE_HPP
