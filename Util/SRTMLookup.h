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

 ** Interface to the GridSquares. Template parameter determines which kind of
 *  raw data is used (NASA, or reduced). GridSquare data is globally cached, so
 *  that many queries can be answered without having to constantly mmap & unmmap
 *  the data files.
 *
 *  Typical usage:
 *
 *  Grid<NasaGridSquare> g();
 *  cout << g.height(lng,lat) << endl;
 *
 */

#ifndef SRTMLOOKUP_H
#define SRTMLOOKUP_H

#include <cassert>
#include <string>

#include "NASAGridSquare.h"
#include "../DataStructures/LRUCache.h"

class SRTMLookup {
public:

    SRTMLookup(std::string & _rp) : cache(MAX_CACHE_SIZE), ROOT_PATH(_rp) {
        // Double check that this compiler truncates towards zero.
        assert(-1 == int(float(-1.9)));
    }

    /** Returns the height above sea level for the given lat/lng. */
    short height(const float longitude, const float latitude) {
        if(0 == ROOT_PATH.size())
            return 0;
        int lng,lat;
        float lng_fraction,lat_fraction;
        split(longitude,lng,lng_fraction);
        split(latitude ,lat,lat_fraction);

        int k = key(lng,lat);
        if(!cache.Holds(k)) {
            cache.Insert(k , new NasaGridSquare(lng,lat, ROOT_PATH));
        }
        NasaGridSquare * result;
        cache.Fetch(k, result);
        return result->getHeight(lng_fraction,lat_fraction);
    }

private:
    /** Split a floating point number (num) into integer (i) & fraction (f)
     *  components. */
    inline void split(float num, int& i, float& f) const {
        if(num>=0.0)
            i=int(num);
        else
            i=int(num)-1;
        f=num-float(i);
    }

    /** Formula for grid squares' unique keys. */
    int key(int lng, int lat) {
        return 1000*lat + lng;
    }
    LRUCache<NasaGridSquare*> cache;
    static const int MAX_CACHE_SIZE = 250;
    std::string ROOT_PATH;
};

#endif // SRTMLOOKUP_H
