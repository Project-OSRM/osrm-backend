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

#ifndef BRESENHAM_H_
#define BRESENHAM_H_

#include <cmath>
#include <vector>
typedef std::pair<unsigned, unsigned> BresenhamPixel;

inline void Bresenham (int x0, int y0, int x1, int y1, std::vector<BresenhamPixel> &resultList) {
    int dx = std::abs(x1-x0);
    int dy = std::abs(y1-y0);
    int sx = (x0 < x1 ? 1 : -1);
    int sy = (y0 < y1 ? 1 : -1);
    int err = dx - dy;
    while(true) {
        resultList.push_back(std::make_pair(x0,y0));
        if(x0 == x1 && y0 == y1) break;
        int e2 = 2* err;
        if ( e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if(e2 < dx) {
            err+= dx;
            y0 += sy;
        }
    }
}

#endif /* BRESENHAM_H_ */
