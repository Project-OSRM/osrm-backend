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


#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <cmath>

inline double deg2rad(const double d) {
     return d*M_PI/180;
}

inline double rad2deg(const double r) {
    return r*180/M_PI;
}

inline double int2rad(long i) {
    return (i/100000.)*M_PI/180;
}

inline double lat2yrad(double lat) {
	return log(tan( M_PI/4 + lat/2 ));
}

#endif /* GEOMETRY_H_ */
