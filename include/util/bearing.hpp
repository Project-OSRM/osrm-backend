#ifndef BEARING_HPP
#define BEARING_HPP

#include <algorithm>
#include <boost/assert.hpp>
#include <cmath>
#include <string>

namespace osrm
{
namespace util
{
namespace bearing
{

inline std::string get(const double heading)
{
    BOOST_ASSERT(heading >= 0);
    BOOST_ASSERT(heading <= 360);

    if (heading <= 22.5)
    {
        return "N";
    }
    if (heading <= 67.5)
    {
        return "NE";
    }
    if (heading <= 112.5)
    {
        return "E";
    }
    if (heading <= 157.5)
    {
        return "SE";
    }
    if (heading <= 202.5)
    {
        return "S";
    }
    if (heading <= 247.5)
    {
        return "SW";
    }
    if (heading <= 292.5)
    {
        return "W";
    }
    if (heading <= 337.5)
    {
        return "NW";
    }
    return "N";
}

// Checks whether A is between B-range and B+range, all modulo 360
// e.g. A = 5, B = 5, range = 10 == true
//      A = -6, B = 5, range = 10 == false
//      A = -2, B = 355, range = 10 == true
//      A = 6, B = 355, range = 10 == false
//      A = 355, B = -2, range = 10 == true
//
// @param A the bearing to check, in degrees, 0-359, 0=north
// @param B the bearing to check against, in degrees, 0-359, 0=north
// @param range the number of degrees either side of B that A will still match
// @return true if B-range <= A <= B+range, modulo 360
inline bool CheckInBounds(const int A, const int B, const int range)
{

    if (range >= 180)
        return true;
    if (range < 0)
        return false;

    // Map both bearings into positive modulo 360 space
    const int normalized_B = (B < 0) ? (B % 360) + 360 : (B % 360);
    const int normalized_A = (A < 0) ? (A % 360) + 360 : (A % 360);

    if (normalized_B - range < 0)
    {
        return (normalized_B - range + 360 <= normalized_A && normalized_A < 360) ||
               (0 <= normalized_A && normalized_A <= normalized_B + range);
    }
    else if (normalized_B + range > 360)
    {
        return (normalized_B - range <= normalized_A && normalized_A < 360) ||
               (0 <= normalized_A && normalized_A <= normalized_B + range - 360);
    }
    else
    {
        return normalized_B - range <= normalized_A && normalized_A <= normalized_B + range;
    }
}
} // namespace bearing

inline double reverseBearing(const double bearing)
{
    if (bearing >= 180)
        return bearing - 180.;
    return bearing + 180;
}

// Compute the angle between two bearings on a normal turn circle
//
//      Bearings                      Angles
//
//         0                           180
//   315         45               225       135
//
// 270     x       90           270     x      90
//
//   225        135               315        45
//        180                           0
//
// A turn from north to north-east offerst bearing 0 and 45 has to be translated
// into a turn of 135 degrees. The same holdes for 90 - 135 (east to south
// east).
// For north, the transformation works by angle = 540 (360 + 180) - exit_bearing
// % 360;
// All other cases are handled by first rotating both bearings to an
// entry_bearing of 0.
inline double angleBetweenBearings(const double entry_bearing, const double exit_bearing)
{
    const double offset = 360 - entry_bearing;
    const double rotated_exit = [](double bearing, const double offset) {
        bearing += offset;
        return bearing > 360 ? bearing - 360 : bearing;
    }(exit_bearing, offset);

    const auto angle = 540 - rotated_exit;
    return angle >= 360 ? angle - 360 : angle;
}

// minimal difference between two angles/bearings going left or right
inline double angularDeviation(const double angle, const double from)
{
    const double deviation = std::abs(angle - from);
    return std::min(360 - deviation, deviation);
}

} // namespace util
} // namespace osrm

#endif // BEARING_HPP
