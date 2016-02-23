/*

Copyright (c) 2016, Project OSRM contributors
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

#ifndef COORDINATE_HPP_
#define COORDINATE_HPP_

#include "util/strong_typedef.hpp"

#include <iosfwd> //for std::ostream
#include <string>
#include <type_traits>
#include <cstddef>

namespace osrm
{

constexpr const double COORDINATE_PRECISION = 1e6;

namespace util
{

OSRM_STRONG_TYPEDEF(int32_t, FixedLatitude)
OSRM_STRONG_TYPEDEF(int32_t, FixedLongitude)
OSRM_STRONG_TYPEDEF(double, FloatLatitude)
OSRM_STRONG_TYPEDEF(double, FloatLongitude)

inline FixedLatitude toFixed(const FloatLatitude floating)
{
    return FixedLatitude(static_cast<double>(floating) * COORDINATE_PRECISION);
}

inline FixedLongitude toFixed(const FloatLongitude floating)
{
    return FixedLongitude(static_cast<double>(floating) * COORDINATE_PRECISION);
}

inline FloatLatitude toFloating(const FixedLatitude fixed)
{
    return FloatLatitude(static_cast<int32_t>(fixed) / COORDINATE_PRECISION);
}

inline FloatLongitude toFloating(const FixedLongitude fixed)
{
    return FloatLongitude(static_cast<int32_t>(fixed) / COORDINATE_PRECISION);
}

// Coordinate encoded as longitude, latitude
struct Coordinate
{
    FixedLongitude lon;
    FixedLatitude lat;

    Coordinate();
    Coordinate(const FixedLongitude lon_, const FixedLatitude lat_);
    Coordinate(const FloatLongitude lon_, const FloatLatitude lat_);

    template <class T> Coordinate(const T &coordinate) : lon(coordinate.lon), lat(coordinate.lat)
    {
        static_assert(!std::is_same<T, Coordinate>::value,
                      "This constructor should not be used for Coordinates");
        static_assert(std::is_same<decltype(lon), decltype(coordinate.lon)>::value,
                      "coordinate types incompatible");
        static_assert(std::is_same<decltype(lat), decltype(coordinate.lat)>::value,
                      "coordinate types incompatible");
    }

    bool IsValid() const;
    friend bool operator==(const Coordinate lhs, const Coordinate rhs);
    friend bool operator!=(const Coordinate lhs, const Coordinate rhs);
    friend std::ostream &operator<<(std::ostream &out, const Coordinate coordinate);
};

bool operator==(const Coordinate lhs, const Coordinate rhs);
std::ostream &operator<<(std::ostream &out, const Coordinate coordinate);
}
}

#endif /* COORDINATE_HPP_ */
