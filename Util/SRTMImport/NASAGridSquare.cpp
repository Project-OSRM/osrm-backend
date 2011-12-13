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

#include "NASAGridSquare.h"

NasaGridSquare::~NasaGridSquare() {
    if(data)
        unload();
}

std::string NasaGridSquare::make_filename(const char* ext) const
{
    using namespace std;
    char EW =(longitude>=0?'E':'W');
    char NS =(latitude>=0?'N':'S');
    stringstream ss;
#ifdef ROOT_PATH
    static const char* root_path =ROOT_PATH;
#else
    static const char* root_path =FloodUtils::getenvz("ROOT_PATH",".");
#endif
    ss<<setfill('0')
                    << root_path
                    << "/"
                    <<NS
                    <<setw(2)<<abs(latitude)<<setw(0)
                    <<EW
                    <<setw(3)<<abs(longitude)<<setw(0)
                    <<'.'<<ext;
    return ss.str();
}

void NasaGridSquare::parse_filename(const char* filename, int& lng, int& lat)
{
    char fn[12]; // N51E001.xyz
    FloodUtils::strlcpy(fn,filename,sizeof(fn));
    lng=( fn[3]=='E'? 1: -1 );
    lat=( fn[0]=='N'? 1: -1 );
    fn[3]=fn[7]='\0';
    lng *= ::atoi(fn+4);
    lat *= ::atoi(fn+1);
}

void NasaGridSquare::load(const char* filename) {
    const std::string gzipfile =std::string(filename)+".zip";
    bool triedZipped =false;
    std::cout << "testing for zipped file " << gzipfile << std::endl;
    if(FloodUtils::exists(gzipfile.c_str())) {
        std::cout << gzipfile << " exists" << std::endl;
    }
    int errorcode = 0;
    int errorcode2 = 0;
    zip * test = zip_open(gzipfile.c_str(), 0, &errorcode);
    if(errorcode) {
        char buffer[1024];
        zip_error_to_str(buffer, 1024, errorcode, errno);
        ERR("Error occured opening zipfile " << filename << ", error: " << buffer);
    } else {
        unsigned entries = zip_get_num_files(test);
        INFO("File has " << entries << " entries");
        //for(unsigned i = 0; i < entries; ++i) {
        struct zip_stat stat;
        //ZIP_EXTERN int zip_stat_index(struct zip *, int, int, struct zip_stat *);
        zip_stat_index(test, 0, 0, &stat);
        INFO("included: " << stat.name);
        INFO("uncompressed: " << stat.size);
        INFO("compressed: " << stat.comp_size);
        //        }
        DELETE(data2);
        data2 = new char[stat.size];

        zip_file * file = zip_fopen_index(test, 0, 0 );
        INFO("opened");
        zip_file_error_get(file, &errorcode, &errorcode2);
        INFO("opened: " << errorcode << ", " << errorcode2);
        zip_fread(file, data2, stat.size);

        INFO("read");
        zip_fclose(file);
        INFO("closed file");
        unsigned short datum=((unsigned short*)data2)[493093];
        short result=(datum>>8 | datum<<8);
        INFO("493093: " << result);
    }



    zip_close(test);

    while(true) {
        fd=::open(filename,O_RDONLY);
        if(fd>=0)
            break;
        DEBUG_ONLY(int saved_errno =errno;)
        if(ENOENT!=errno || triedZipped || !FloodUtils::exists(gzipfile.c_str())) {
            DB("Failed to open file "<<filename<<": "<<::strerror(saved_errno))
                      return;
        }
        // Unzip and try again
        DB("Unzipping "<<gzipfile)
        std::cout << "unzipping" << std::endl;
        std::stringstream ss;
        ss<<"/bin/gzip -dc "<<gzipfile<<" > "<<filename;
        int status =::system(ss.str().c_str());
        if(-1 == status)
        {
            ERR("Failed to execute shell command ("<<ss.str()<<"): "<<::strerror(errno));
            exit(-1);
        }
        else if(0 != WEXITSTATUS(status))
        {
            ERR("Failed to unzip file "<<gzipfile<<": "<<WEXITSTATUS(status));
            exit(-1);
        }
        triedZipped=true;
    }

    // Get file length
    struct stat file_stat;
    if( ::fstat(fd,&file_stat)<0 )
    {
        ::perror("fstat error");
        ::exit(-1);
    }
    num_bytes=file_stat.st_size;
    if(!num_bytes)
    {
        ::close(fd);
        return;
    }

    data =::mmap(0,num_bytes,PROT_READ,MAP_SHARED,fd,0);
    if(MAP_FAILED==data)
    {
        ::perror("mmap error");
        ::exit(-1);
    }
}

void NasaGridSquare::unload()
{
    if(::munmap(data,num_bytes)<0)
    {
        ::perror("munmap error");
        ::exit(-1);
    }
    ::close(fd);

}

bool NasaGridSquare::lngLat_to_colRow(
        float lng_fraction, float lat_fraction,
        unsigned int& col,  unsigned int&  row
) const
{
    col =             int( 0.5 + lng_fraction * float(num_cols-1) );
    row =num_rows-1 - int( 0.5 + lat_fraction * float(num_rows-1) );
    return( col<num_cols && row<num_rows );
}

NasaGridSquare::NasaGridSquare(const char* filename):
                            data(NULL), data2(NULL), longitude(0), latitude(0),
                            fd(-1), num_bytes(0), num_rows(1), num_cols(1)
{
    parse_filename(filename,longitude,latitude);
    load(filename);
    if(has_data())
        num_rows=num_cols=std::sqrt(num_bytes/2);
}

NasaGridSquare::NasaGridSquare(int lng, int lat) :
                    data(NULL), data2(NULL), longitude(lng), latitude(lat),
                    fd(-1), num_bytes(0), num_rows(1), num_cols(1)
{
    std::string filename =make_filename("hgt");
    std::cout << "Loading " << filename << std::endl;
    load(filename.c_str());
    if(has_data())
        num_rows=num_cols=std::sqrt(num_bytes/2);
}

short NasaGridSquare::height(float lng_fraction, float lat_fraction) const {
    if(!data)
        return NO_DATA;
    unsigned int col,row;
    if(!lngLat_to_colRow(lng_fraction,lat_fraction, col,row))
        return NO_DATA;
    return getHeight(col,row);
}

short NasaGridSquare::getHeight(unsigned int col, unsigned int row) const {
    if(col>=num_cols || row>=num_rows)
        return NO_DATA;
    unsigned int i=( col + row * num_cols );
//    unsigned short datum=((unsigned short*)data)[i];
//    short result=(datum>>8 | datum<<8);
    unsigned short datum2=((unsigned short*)data2)[i];
    short result2=(datum2>>8 | datum2<<8);

//    INFO("[" << i << "] result: " << result << ", result2: " << (int)result2);
    return result2;
}

