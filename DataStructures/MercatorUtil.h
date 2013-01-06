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

#ifndef MERCATORUTIL_H_
#define MERCATORUTIL_H_

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

inline double y2lat(double a) {
	return 180/M_PI * (2 * atan(exp(a*M_PI/180)) - M_PI/2);
}

inline double lat2y(double a) {
	return 180/M_PI * log(tan(M_PI/4+a*(M_PI/180)/2));
}

#endif /* MERCATORUTIL_H_ */
