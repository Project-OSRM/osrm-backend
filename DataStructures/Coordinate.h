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

#ifndef COORDINATE_H_
#define COORDINATE_H_

#include <climits>
#include <iostream>

struct _Coordinate {
    int lat;
    int lon;
    _Coordinate () : lat(INT_MIN), lon(INT_MIN) {}
    _Coordinate (int t, int n) : lat(t) , lon(n) {}
    void Reset() {
        lat = INT_MIN;
        lon = INT_MIN;
    }
    bool isSet() const {
        return (INT_MIN != lat) && (INT_MIN != lon);
    }
    inline bool isValid() const {
        if(lat > 90*100000 || lat < -90*100000 || lon > 180*100000 || lon <-180*100000) {
            return false;
        }
        return true;
    }

};

inline std::ostream & operator<<(std::ostream & out, const _Coordinate & c){
    out << "(" << c.lat << "," << c.lon << ")";
    return out;
}

inline double ApproximateDistance( const int lat1, const int lon1, const int lat2, const int lon2 ) {
    assert(lat1 != INT_MIN);
    assert(lon1 != INT_MIN);
    assert(lat2 != INT_MIN);
    assert(lon2 != INT_MIN);
    static const double DEG_TO_RAD = 0.017453292519943295769236907684886;
    //Earth's quatratic mean radius for WGS-84
    static const double EARTH_RADIUS_IN_METERS = 6372797.560856;
    double latitudeArc  = ( lat1/100000. - lat2/100000. ) * DEG_TO_RAD;
    double longitudeArc = ( lon1/100000. - lon2/100000. ) * DEG_TO_RAD;
    double latitudeH = sin( latitudeArc * 0.5 );
    latitudeH *= latitudeH;
    double lontitudeH = sin( longitudeArc * 0.5 );
    lontitudeH *= lontitudeH;
    double tmp = cos( lat1/100000. * DEG_TO_RAD ) * cos( lat2/100000. * DEG_TO_RAD );
    double distanceArc =  2.0 * asin( sqrt( latitudeH + tmp * lontitudeH ) );
    return EARTH_RADIUS_IN_METERS * distanceArc;
}

inline double ApproximateDistance(const _Coordinate &c1, const _Coordinate &c2) {
    return ApproximateDistance( c1.lat, c1.lon, c2.lat, c2.lon );
}

#endif /* COORDINATE_H_ */
