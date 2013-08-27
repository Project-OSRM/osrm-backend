/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef FIXED_POINT_COORDINATE_H_
#define FIXED_POINT_COORDINATE_H_

#include "../DataStructures/MercatorUtil.h"
#include "../Util/StringUtil.h"

#include <cassert>
#include <cmath>
#include <climits>

#include <iostream>

static const double COORDINATE_PRECISION = 1000000.;

struct FixedPointCoordinate {
    int lat;
    int lon;
    FixedPointCoordinate () : lat(INT_MIN), lon(INT_MIN) {}
    explicit FixedPointCoordinate (int lat, int lon) : lat(lat) , lon(lon) {}

    void Reset() {
        lat = INT_MIN;
        lon = INT_MIN;
    }
    bool isSet() const {
        return (INT_MIN != lat) && (INT_MIN != lon);
    }
    inline bool isValid() const {
        if(
            lat >   90*COORDINATE_PRECISION ||
            lat <  -90*COORDINATE_PRECISION ||
            lon >  180*COORDINATE_PRECISION ||
            lon < -180*COORDINATE_PRECISION
        ) {
            return false;
        }
        return true;
    }
    bool operator==(const FixedPointCoordinate & other) const {
        return lat == other.lat && lon == other.lon;
    }
};

inline std::ostream & operator<<(std::ostream & out, const FixedPointCoordinate & c){
    out << "(" << c.lat << "," << c.lon << ")";
    return out;
}

inline double ApproximateDistance( const int lat1, const int lon1, const int lat2, const int lon2 ) {
    assert(lat1 != INT_MIN);
    assert(lon1 != INT_MIN);
    assert(lat2 != INT_MIN);
    assert(lon2 != INT_MIN);
    double RAD = 0.017453292519943295769236907684886;
    double lt1 = lat1/COORDINATE_PRECISION;
    double ln1 = lon1/COORDINATE_PRECISION;
    double lt2 = lat2/COORDINATE_PRECISION;
    double ln2 = lon2/COORDINATE_PRECISION;
    double dlat1=lt1*(RAD);

    double dlong1=ln1*(RAD);
    double dlat2=lt2*(RAD);
    double dlong2=ln2*(RAD);

    double dLong=dlong1-dlong2;
    double dLat=dlat1-dlat2;

    double aHarv= pow(sin(dLat/2.0),2.0)+cos(dlat1)*cos(dlat2)*pow(sin(dLong/2.),2);
    double cHarv=2.*atan2(sqrt(aHarv),sqrt(1.0-aHarv));
    //earth's radius from wikipedia varies between 6,356.750 km — 6,378.135 km (˜3,949.901 — 3,963.189 miles)
    //The IUGG value for the equatorial radius of the Earth is 6378.137 km (3963.19 mile)
    const double earth=6372797.560856;//I am doing miles, just change this to radius in kilometers to get distances in km
    double distance=earth*cHarv;
    return distance;
}

inline double ApproximateDistance(const FixedPointCoordinate &c1, const FixedPointCoordinate &c2) {
    return ApproximateDistance( c1.lat, c1.lon, c2.lat, c2.lon );
}

inline double ApproximateEuclideanDistance(const FixedPointCoordinate &c1, const FixedPointCoordinate &c2) {
    assert(c1.lat != INT_MIN);
    assert(c1.lon != INT_MIN);
    assert(c2.lat != INT_MIN);
    assert(c2.lon != INT_MIN);
    const double RAD = 0.017453292519943295769236907684886;
    const double lat1 = (c1.lat/COORDINATE_PRECISION)*RAD;
    const double lon1 = (c1.lon/COORDINATE_PRECISION)*RAD;
    const double lat2 = (c2.lat/COORDINATE_PRECISION)*RAD;
    const double lon2 = (c2.lon/COORDINATE_PRECISION)*RAD;

    const double x = (lon2-lon1) * cos((lat1+lat2)/2.);
    const double y = (lat2-lat1);
    const double earthRadius = 6372797.560856;
    const double d = sqrt(x*x + y*y) * earthRadius;
    return d;
}

static inline void convertInternalLatLonToString(const int value, std::string & output) {
    char buffer[100];
    buffer[11] = 0; // zero termination
    char* string = printInt< 11, 6 >( buffer, value );
    output = string;
}

static inline void convertInternalCoordinateToString(const FixedPointCoordinate & coord, std::string & output) {
    std::string tmp;
    convertInternalLatLonToString(coord.lon, tmp);
    output = tmp;
    output += ",";
    convertInternalLatLonToString(coord.lat, tmp);
    output += tmp;
    output += " ";
}
static inline void convertInternalReversedCoordinateToString(const FixedPointCoordinate & coord, std::string & output) {
    std::string tmp;
    convertInternalLatLonToString(coord.lat, tmp);
    output = tmp;
    output += ",";
    convertInternalLatLonToString(coord.lon, tmp);
    output += tmp;
    output += " ";
}


/* Get angle of line segment (A,C)->(C,B), atan2 magic, formerly cosine theorem*/
template<class CoordinateT>
static inline double GetAngleBetweenThreeFixedPointCoordinates (
    const CoordinateT & A,
    const CoordinateT & C,
    const CoordinateT & B
) {
    const double v1x = (A.lon - C.lon)/COORDINATE_PRECISION;
    const double v1y = lat2y(A.lat/COORDINATE_PRECISION) - lat2y(C.lat/COORDINATE_PRECISION);
    const double v2x = (B.lon - C.lon)/COORDINATE_PRECISION;
    const double v2y = lat2y(B.lat/COORDINATE_PRECISION) - lat2y(C.lat/COORDINATE_PRECISION);

    double angle = (atan2(v2y,v2x) - atan2(v1y,v1x) )*180/M_PI;
    while(angle < 0)
        angle += 360;
    return angle;
}
#endif /* FIXED_POINT_COORDINATE_H_ */
