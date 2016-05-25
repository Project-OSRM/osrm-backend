#ifndef BEARING_HPP
#define BEARING_HPP

#include <boost/assert.hpp>
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
    if (range <= 0)
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

inline double reverseBearing(const double bearing)
{
    if (bearing >= 180)
        return bearing - 180.;
    return bearing + 180;
}

}
}
}

#endif // BEARING_HPP
