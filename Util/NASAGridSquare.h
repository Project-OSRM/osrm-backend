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


#ifndef NASA_GRID_SQUARE_H
#define NASA_GRID_SQUARE_H

#include "ProjectionUtils.h"
#include "../typedefs.h"

#include <cmath>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <zip.h>


class NasaGridSquare {
public:
    const static short NO_DATA =-32768; ///< NASA's magic number for no data.

    NasaGridSquare(int lng, int lat, std::string & _rp);
    ~NasaGridSquare();
    short getHeight(float lng_fraction, float lat_fraction) const;
    bool hasData() const {return bool(elevationMap != NULL);}

private:
    std::string ROOT_PATH;
    char * elevationMap;
    int longitude;
    int latitude;
    unsigned int num_bytes; ///< Length of data.
    unsigned int numRows;
    unsigned int numCols;

    std::string make_filename(const char* ext) const;

    /** Loads the file into memory, pointed to by 'data'. Sets num_bytes. */
    void load(const char* filename);
    /** Converts lng,lat floats to col,row ints. Returns TRUE if result valid. */
    bool lngLat_to_colRow(float lng_fraction, float lat_fraction, unsigned int& col,  unsigned int& row) const;

    inline bool exists(const char* path) {
        std::ifstream ifile(path);
        return ifile;
    }

};

#endif // NASA_GRID_SQUARE_H
