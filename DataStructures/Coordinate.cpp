/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
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

#include <osrm/Coordinate.h>
#include "../Util/MercatorUtil.h"
#include "../Util/SimpleLogger.h"
#include "../Util/StringUtil.h"
#include "../Util/TrigonometryTables.h"

#include <boost/assert.hpp>

#ifndef NDEBUG
#include <bitset>
#endif
#include <iostream>
#include <limits>

FixedPointCoordinate::FixedPointCoordinate() : Coordinate(MIN, MIN, MIN)
{
}

FixedPointCoordinate::FixedPointCoordinate(int lat, int lon, int ele) : Coordinate(lat, lon, ele)
{
#ifndef NDEBUG
    if (0 != (std::abs(lat) >> 30))
    {
        std::bitset<32> y(lat);
        SimpleLogger().Write(logDEBUG) << "broken lat: " << lat << ", bits: " << y;
    }
    if (0 != (std::abs(lon) >> 30))
    {
        std::bitset<32> x(lon);
        SimpleLogger().Write(logDEBUG) << "broken lon: " << lon << ", bits: " << x;
    }
#endif
}

void FixedPointCoordinate::Reset()
{
    lat = MIN;
    lon = MIN;
    setEle(MIN);
}
bool FixedPointCoordinate::isSet() const
{
    return (MIN != lat) && (MIN != lon);
}
bool FixedPointCoordinate::isValid() const
{
    if (lat > 90 * COORDINATE_PRECISION || lat < -90 * COORDINATE_PRECISION ||
        lon > 180 * COORDINATE_PRECISION || lon < -180 * COORDINATE_PRECISION)
    {
        return false;
    }
    return true;
}
bool FixedPointCoordinate::operator==(const FixedPointCoordinate &other) const
{
    return lat == other.lat && lon == other.lon;
}

double FixedPointCoordinate::ApproximateDistance(const int lat1,
                                                 const int lon1,
                                                 const int lat2,
                                                 const int lon2)
{
    BOOST_ASSERT(lat1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lat2 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon2 != std::numeric_limits<int>::min());
    double RAD = 0.017453292519943295769236907684886;
    double lt1 = lat1 / COORDINATE_PRECISION;
    double ln1 = lon1 / COORDINATE_PRECISION;
    double lt2 = lat2 / COORDINATE_PRECISION;
    double ln2 = lon2 / COORDINATE_PRECISION;
    double dlat1 = lt1 * (RAD);

    double dlong1 = ln1 * (RAD);
    double dlat2 = lt2 * (RAD);
    double dlong2 = ln2 * (RAD);

    double dLong = dlong1 - dlong2;
    double dLat = dlat1 - dlat2;

    double aHarv = pow(sin(dLat / 2.0), 2.0) + cos(dlat1) * cos(dlat2) * pow(sin(dLong / 2.), 2);
    double cHarv = 2. * atan2(sqrt(aHarv), sqrt(1.0 - aHarv));
    // earth radius varies between 6,356.750-6,378.135 km (3,949.901-3,963.189mi)
    // The IUGG value for the equatorial radius is 6378.137 km (3963.19 miles)
    const double earth = 6372797.560856;
    return earth * cHarv;
}

double FixedPointCoordinate::ApproximateDistance(const FixedPointCoordinate &c1,
                                                 const FixedPointCoordinate &c2)
{
    return ApproximateDistance(c1.lat, c1.lon, c2.lat, c2.lon);
}

float FixedPointCoordinate::ApproximateEuclideanDistance(const FixedPointCoordinate &c1,
                                                 const FixedPointCoordinate &c2)
{
    return ApproximateEuclideanDistance(c1.lat, c1.lon, c2.lat, c2.lon);
}

float FixedPointCoordinate::ApproximateEuclideanDistance(const int lat1,
                                                          const int lon1,
                                                          const int lat2,
                                                          const int lon2)
{
    BOOST_ASSERT(lat1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lat2 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon2 != std::numeric_limits<int>::min());

    const float RAD = 0.017453292519943295769236907684886;
    const float float_lat1 = (lat1 / COORDINATE_PRECISION) * RAD;
    const float float_lon1 = (lon1 / COORDINATE_PRECISION) * RAD;
    const float float_lat2 = (lat2 / COORDINATE_PRECISION) * RAD;
    const float float_lon2 = (lon2 / COORDINATE_PRECISION) * RAD;

    const float x = (float_lon2 - float_lon1) * cos((float_lat1 + float_lat2) / 2.);
    const float y = (float_lat2 - float_lat1);
    const float earth_radius = 6372797.560856;
    return sqrt(x * x + y * y) * earth_radius;
}

float FixedPointCoordinate::ComputePerpendicularDistance(const FixedPointCoordinate &point,
                                                          const FixedPointCoordinate &segA,
                                                          const FixedPointCoordinate &segB)
{
    const float x = lat2y(point.lat / COORDINATE_PRECISION);
    const float y = point.lon / COORDINATE_PRECISION;
    const float a = lat2y(segA.lat / COORDINATE_PRECISION);
    const float b = segA.lon / COORDINATE_PRECISION;
    const float c = lat2y(segB.lat / COORDINATE_PRECISION);
    const float d = segB.lon / COORDINATE_PRECISION;
    float p, q, nY;
    if (std::abs(a - c) > std::numeric_limits<float>::epsilon())
    {
        const float m = (d - b) / (c - a); // slope
        // Projection of (x,y) on line joining (a,b) and (c,d)
        p = ((x + (m * y)) + (m * m * a - m * b)) / (1. + m * m);
        q = b + m * (p - a);
    }
    else
    {
        p = c;
        q = y;
    }
    nY = (d * p - c * q) / (a * d - b * c);

    // discretize the result to coordinate precision. it's a hack!
    if (std::abs(nY) < (1. / COORDINATE_PRECISION))
    {
        nY = 0.;
    }

    float r = (p - nY * a) / c;
    if (std::isnan(r))
    {
        r = ((segB.lat == point.lat) && (segB.lon == point.lon)) ? 1. : 0.;
    }
    else if (std::abs(r) <= std::numeric_limits<float>::epsilon())
    {
        r = 0.;
    }
    else if (std::abs(r - 1.) <= std::numeric_limits<float>::epsilon())
    {
        r = 1.;
    }
    FixedPointCoordinate nearest_location;
    BOOST_ASSERT(!std::isnan(r));
    if (r <= 0.)
    { // point is "left" of edge
        nearest_location.lat = segA.lat;
        nearest_location.lon = segA.lon;
    }
    else if (r >= 1.)
    { // point is "right" of edge
        nearest_location.lat = segB.lat;
        nearest_location.lon = segB.lon;
    }
    else
    { // point lies in between
        nearest_location.lat = y2lat(p) * COORDINATE_PRECISION;
        nearest_location.lon = q * COORDINATE_PRECISION;
    }
    BOOST_ASSERT(nearest_location.isValid());
    const float approximated_distance =
        FixedPointCoordinate::ApproximateEuclideanDistance(point, nearest_location);
    BOOST_ASSERT(0. <= approximated_distance);
    return approximated_distance;
}

float FixedPointCoordinate::ComputePerpendicularDistance(const FixedPointCoordinate &coord_a,
                                                   const FixedPointCoordinate &coord_b,
                                                   const FixedPointCoordinate &query_location,
                                                   FixedPointCoordinate &nearest_location,
                                                   float &r)
{
    BOOST_ASSERT(query_location.isValid());

    const float x = lat2y(query_location.lat / COORDINATE_PRECISION);
    const float y = query_location.lon / COORDINATE_PRECISION;
    const float a = lat2y(coord_a.lat / COORDINATE_PRECISION);
    const float b = coord_a.lon / COORDINATE_PRECISION;
    const float c = lat2y(coord_b.lat / COORDINATE_PRECISION);
    const float d = coord_b.lon / COORDINATE_PRECISION;
    float p, q /*,mX*/, nY;
    if (std::abs(a - c) > std::numeric_limits<float>::epsilon())
    {
        const float m = (d - b) / (c - a); // slope
        // Projection of (x,y) on line joining (a,b) and (c,d)
        p = ((x + (m * y)) + (m * m * a - m * b)) / (1. + m * m);
        q = b + m * (p - a);
    }
    else
    {
        p = c;
        q = y;
    }
    nY = (d * p - c * q) / (a * d - b * c);

    // discretize the result to coordinate precision. it's a hack!
    if (std::abs(nY) < (1. / COORDINATE_PRECISION))
    {
        nY = 0.;
    }

    r = (p - nY * a) / c; // These values are actually n/m+n and m/m+n , we need
    // not calculate the explicit values of m an n as we
    // are just interested in the ratio
    if (std::isnan(r))
    {
        r = ((coord_b.lat == query_location.lat) && (coord_b.lon == query_location.lon)) ? 1. : 0.;
    }
    else if (std::abs(r) <= std::numeric_limits<float>::epsilon())
    {
        r = 0.;
    }
    else if (std::abs(r - 1.) <= std::numeric_limits<float>::epsilon())
    {
        r = 1.;
    }
    BOOST_ASSERT(!std::isnan(r));
    if (r <= 0.)
    {
        nearest_location = coord_a;
    }
    else if (r >= 1.)
    {
        nearest_location = coord_b;
    }
    else
    {
        // point lies in between
        nearest_location.lat = y2lat(p) * COORDINATE_PRECISION;
        nearest_location.lon = q * COORDINATE_PRECISION;
    }
    BOOST_ASSERT(nearest_location.isValid());

    // TODO: Replace with euclidean approximation when k-NN search is done
    // const float approximated_distance = FixedPointCoordinate::ApproximateEuclideanDistance(
    const float approximated_distance =
        FixedPointCoordinate::ApproximateEuclideanDistance(query_location, nearest_location);
    BOOST_ASSERT(0. <= approximated_distance);
    return approximated_distance;
}

void FixedPointCoordinate::convertInternalLatLonToString(const int value, std::string &output)
{
    char buffer[12];
    buffer[11] = 0; // zero termination
    output = printInt<11, 6>(buffer, value);
}

void FixedPointCoordinate::convertInternalCoordinateToString(const FixedPointCoordinate &coord,
                                                             std::string &output)
{
    std::string tmp;
    tmp.reserve(23);
    convertInternalLatLonToString(coord.lon, tmp);
    output = tmp;
    output += ",";
    convertInternalLatLonToString(coord.lat, tmp);
    output += tmp;
}

void FixedPointCoordinate::convertInternalElevationToString(const int value, std::string &output)
{
    char buffer[12];
    buffer[sizeof(buffer)-1] = 0; // zero termination
    output = printInt<sizeof(buffer)-1, ELEVATION_NUM_DIGITS>(buffer, value);
}

void
FixedPointCoordinate::convertInternalReversedCoordinateToString(const FixedPointCoordinate &coord,
                                                                std::string &output)
{
    std::string tmp;
    tmp.reserve(23);
    convertInternalLatLonToString(coord.lat, tmp);
    output = tmp;
    output += ",";
    convertInternalLatLonToString(coord.lon, tmp);
    output += tmp;
}

void FixedPointCoordinate::Output(std::ostream &out) const
{
    out << "(" << lat / COORDINATE_PRECISION << "," << lon / COORDINATE_PRECISION << ")";
}

float FixedPointCoordinate::GetBearing(const FixedPointCoordinate &A, const FixedPointCoordinate &B)
{
    const float delta_long = DegreeToRadian(B.lon / COORDINATE_PRECISION - A.lon / COORDINATE_PRECISION);
    const float lat1 = DegreeToRadian(A.lat / COORDINATE_PRECISION);
    const float lat2 = DegreeToRadian(B.lat / COORDINATE_PRECISION);
    const float y = sin(delta_long) * cos(lat2);
    const float x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(delta_long);
    float result = RadianToDegree(std::atan2(y, x));
    while (result < 0.f)
    {
        result += 360.f;
    }

    while (result >= 360.f)
    {
        result -= 360.f;
    }
    return result;
}

float FixedPointCoordinate::GetBearing(const FixedPointCoordinate &other) const
{
    const float delta_long = DegreeToRadian(lon / COORDINATE_PRECISION - other.lon / COORDINATE_PRECISION);
    const float lat1 = DegreeToRadian(other.lat / COORDINATE_PRECISION);
    const float lat2 = DegreeToRadian(lat / COORDINATE_PRECISION);
    const float y = std::sin(delta_long) * std::cos(lat2);
    const float x = std::cos(lat1) * std::sin(lat2) - std::sin(lat1) * std::cos(lat2) * std::cos(delta_long);
    float result = RadianToDegree(std::atan2(y, x));

    while (result < 0.f)
    {
        result += 360.f;
    }

    while (result >= 360.f)
    {
        result -= 360.f;
    }
    return result;
}

float FixedPointCoordinate::DegreeToRadian(const float degree)
{
    return degree * (M_PI / 180.f);
}

float FixedPointCoordinate::RadianToDegree(const float radian) 
{
    return radian * (180.f / M_PI);
}
