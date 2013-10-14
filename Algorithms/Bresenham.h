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

#ifndef BRESENHAM_H_
#define BRESENHAM_H_

#include <cmath>
#include <vector>

typedef std::pair<unsigned, unsigned> BresenhamPixel;

inline void Bresenham (int x0, int y0, const int x1, int const y1, std::vector<BresenhamPixel> &resultList) {
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
