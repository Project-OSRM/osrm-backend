/*
Copyright (c) 2006  Alex Tingle

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef NASA_GRID_SQUARE_H
#define NASA_GRID_SQUARE_H

#include <string>
#include <zip.h>

#include "utils.h"

#include <cmath>
#include <iostream>
#include <string>
#include "utils.h"

#include <errno.h>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../typedefs.h"

class NasaGridSquare {
public:
    NasaGridSquare(const char* filename);
    NasaGridSquare(int lng, int lat);
    ~NasaGridSquare();
    short height(float lng_fraction, float lat_fraction) const;
    const static short NO_DATA =-32768; ///< NASA's magic number for no data.

    /** Formula for grid squares' unique keys. */
    static int key(int lng, int lat) {
        return 1000*lat + lng;
    }

    /** Calculate unique key for this object. */
    int key() const {return key(longitude,latitude);}
    unsigned int cols() const {return num_cols;}
    unsigned int rows() const {return num_rows;}
    bool has_data() const {return bool(data);}

private:
    void* data; ///< mmap'd data file.
    char * data2;
    int longitude;
    int latitude;
    int fd; ///< File ID for mmap'd data file.
    unsigned int num_bytes; ///< Length of data.
    unsigned int num_rows;
    unsigned int num_cols;

    std::string make_filename(const char* ext) const;
    void parse_filename(const char* filename, int& lng, int& lat);

    /** Loads the file into memory, pointed to by 'data'. Sets num_bytes. */
    void load(const char* filename);
    void unload();
    /** Converts lng,lat floats to col,row ints. Returns TRUE if result valid. */
    bool lngLat_to_colRow(
            float lng_fraction, float lat_fraction,
            unsigned int& col,  unsigned int& row
    ) const;
    short getHeight(unsigned int col, unsigned int row) const;

    friend class ReducedGridSquare;

}; // end class NasaGridSquare

#endif // NASA_GRID_SQUARE_H
