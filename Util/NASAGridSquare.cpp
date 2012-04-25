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
#include <iomanip>

#include <errno.h>
#include <zip.h>

#include "NASAGridSquare.h"

NasaGridSquare::~NasaGridSquare() {
    delete elevationMap;
}

std::string NasaGridSquare::make_filename(const char* ext) const {
    char EW =(longitude>=0 ? 'E' : 'W' );
    char NS =(latitude >=0 ? 'N' : 'S');
    std::stringstream ss;
    ss<<std::setfill('0')
    << ROOT_PATH
    << "/"
    <<NS
    <<std::setw(2)<<std::abs(latitude)<<std::setw(0)
    <<EW
    <<std::setw(3)<<std::abs(longitude)<<std::setw(0)
    <<'.'<<ext;
    return ss.str();
}

void NasaGridSquare::load(const char* filename) {
    const std::string gzipfile =std::string(filename)+".zip";
    std::cout << "testing for zipped file " << gzipfile << std::endl;
    if(exists(gzipfile.c_str())) {
        std::cout << gzipfile << " exists" << std::endl;
    }
    int errorcode = 0;
    int errorcode2 = 0;
    zip * test = zip_open(gzipfile.c_str(), 0, &errorcode);
    if(errorcode) {
        char buffer[1024];
        zip_error_to_str(buffer, 1024, errorcode, errno);
        ERR("Error occured opening zipfile " << filename << ", error: " << buffer);
        //TODO: Load from URL

        return;
    }

    unsigned entries = zip_get_num_files(test);
    INFO("File has " << entries << " entries");
    struct zip_stat stat;
    zip_stat_index(test, 0, 0, &stat);
    INFO("included: " << stat.name);
    INFO("uncompressed: " << stat.size);
    INFO("compressed: " << stat.comp_size);
    num_bytes = stat.size;
    delete elevationMap;
    elevationMap = new char[stat.size];

    zip_file * file = zip_fopen_index(test, 0, 0 );
    zip_file_error_get(file, &errorcode, &errorcode2);
    zip_fread(file, elevationMap, stat.size);
    zip_fclose(file);
    zip_close(test);

}

bool NasaGridSquare::lngLat_to_colRow(float lng_fraction, float lat_fraction, unsigned int& col,  unsigned int&  row) const {
    col = int( 0.5 + lng_fraction * float(numCols-1) );
    row = numRows-1 - int( 0.5 + lat_fraction * float(numRows-1) );
    return( col<numCols && row<numRows );
}

NasaGridSquare::NasaGridSquare(int lng, int lat, std::string & _rp) : ROOT_PATH(_rp), elevationMap(NULL), longitude(lng), latitude(lat), num_bytes(0), numRows(1), numCols(1) {
    std::string filename =make_filename("hgt");
    std::cout << "Loading " << filename << std::endl;
    load(filename.c_str());
    if(hasData())
        numRows=numCols=std::sqrt(num_bytes/2);
}

short NasaGridSquare::getHeight(float lng_fraction, float lat_fraction) const {
    if(!hasData())
        return NO_DATA;
    unsigned int col,row;
    if(!lngLat_to_colRow(lng_fraction,lat_fraction, col,row))
        return NO_DATA;
    if(col>=numCols || row>=numRows)
        return NO_DATA;
    unsigned int i=( col + row * numCols );
    unsigned short datum2=((unsigned short*)elevationMap)[i];
    short result2=(datum2>>8 | datum2<<8);
    return result2;
}
