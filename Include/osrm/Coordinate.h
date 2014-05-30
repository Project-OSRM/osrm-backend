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

#ifndef FIXED_POINT_COORDINATE_H_
#define FIXED_POINT_COORDINATE_H_

#include <functional>
#include <iosfwd> //for std::ostream

#include "OSRM_config.h"

constexpr float COORDINATE_PRECISION = 1000000.;


struct _Point2D
{
    int lat;
    int lon;
    constexpr static int MIN = std::numeric_limits<int>::min();
    constexpr static int MAX = std::numeric_limits<int>::max();

    _Point2D() : lat(MIN), lon(MIN) {};
    _Point2D(int lat, int lon, int ele) : lat(lat), lon(lon) {}

    inline int getEle() const { return MIN; }
    inline void setEle(int ele) {}
};

struct _Point3D
{
    int lat;
    int lon;
    static constexpr int MIN = std::numeric_limits<int>::min();
   static  constexpr int MAX = std::numeric_limits<int>::max();

    _Point3D() : lat(MIN), lon(MIN) {};
    _Point3D(int lat, int lon, int ele) : lat(lat), lon(lon), ele(ele) {}

    inline int getEle() const { return ele; }
    inline void setEle(int ele) { this->ele = ele; }
private:
    int ele;
};

#ifdef OSRM_HAS_ELEVATION
typedef _Point3D Coordinate;
#else
typedef _Point2D Coordinate;
#endif

struct FixedPointCoordinate : public Coordinate
{
    FixedPointCoordinate();
    explicit FixedPointCoordinate(int lat, int lon, int ele = MIN);
    void Reset();
    bool isSet() const;
    bool isValid() const;
    bool operator==(const FixedPointCoordinate &other) const;

    static double
    ApproximateDistance(const int lat1, const int lon1, const int lat2, const int lon2);

    static double ApproximateDistance(const FixedPointCoordinate &c1,
                                      const FixedPointCoordinate &c2);

    static float ApproximateEuclideanDistance(const FixedPointCoordinate &c1,
                                              const FixedPointCoordinate &c2);

    static float ApproximateEuclideanDistance(const int lat1, const int lon1, const int lat2, const int lon2);

    static float ApproximateSquaredEuclideanDistance(const FixedPointCoordinate &c1,
                                                     const FixedPointCoordinate &c2);

    static void convertInternalLatLonToString(const int value, std::string &output);

    static void convertInternalCoordinateToString(const FixedPointCoordinate &coord,
                                                  std::string &output);

    static void convertInternalElevationToString(const int value, std::string &output);

    static void convertInternalReversedCoordinateToString(const FixedPointCoordinate &coord,
                                                          std::string &output);

    static float ComputePerpendicularDistance(const FixedPointCoordinate &point,
                                               const FixedPointCoordinate &segA,
                                               const FixedPointCoordinate &segB);

    static float ComputePerpendicularDistance(const FixedPointCoordinate &coord_a,
                                               const FixedPointCoordinate &coord_b,
                                               const FixedPointCoordinate &query_location,
                                               FixedPointCoordinate &nearest_location,
                                               float &r);

    static float GetBearing(const FixedPointCoordinate &A, const FixedPointCoordinate &B);

    float GetBearing(const FixedPointCoordinate &other) const;

    void Output(std::ostream &out) const;

    static float DegreeToRadian(const float degree);
    static float RadianToDegree(const float radian);
};

inline std::ostream &operator<<(std::ostream &o, FixedPointCoordinate const &c)
{
    c.Output(o);
    return o;
}

#endif /* FIXED_POINT_COORDINATE_H_ */
