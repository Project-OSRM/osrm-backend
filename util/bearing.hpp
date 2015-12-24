/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef BEARING_HPP
#define BEARING_HPP

#include <boost/assert.hpp>
#include <string>

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
}

#endif // BEARING_HPP
