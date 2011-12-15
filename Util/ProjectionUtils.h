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

#ifndef PROJECTIONUTILS_H_
#define PROJECTIONUTILS_H_

#include <cmath>

namespace ProjectionUtils {

inline long googleMaxTiles(int zoom) { return( 1L << (17 - zoom) ); }

inline void googleX_to_lng(int zoom, double x, double& lng) {
    long tilesAtThisZoom =googleMaxTiles(zoom);
    lng = -180.0 + (x * 360.0 / double(tilesAtThisZoom));
}

inline void googleY_to_lat(int zoom, double y, double& lat) {
    // This function is the inverse Mercator projection.
    // Look up 'Mercator projection' on Wikipedia for details.
    long   pixels             = googleMaxTiles(zoom) * 256L;
    double pixelsPerLonRadian = pixels / (2.0 * M_PI);
    double bitmapOrigo        = pixels / 2;

    double Y = (bitmapOrigo - y*256) / pixelsPerLonRadian;

    double latRad = 2.0 * ::atan( ::exp(Y) ) - M_PI_2;
    lat = latRad * 180.0 / M_PI;
}

inline void googleXY_to_latlng( double x, double y, int zoom, double& lng, double& lat ) {
    googleX_to_lng(zoom,x,lng);
    googleY_to_lat(zoom,y,lat);
}

}

#endif /* PROJECTIONUTILS_H_ */
