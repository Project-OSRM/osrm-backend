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

#include "utils.h"

#include <fstream>
#include <iostream>
#include <cmath>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace FloodUtils {

unsigned short sqrt(unsigned long a) {
  return std::sqrt(a);
}


std::list<std::string> split(const std::string& s, char token) {
  std::list<std::string> result;
  std::string::size_type pos=0;
  while(true)
  {
    pos=s.find_first_not_of(token,pos);
    if(pos==std::string::npos) break;
    std::string::size_type token_pos=s.find_first_of(token,pos);
    result.push_back( s.substr(pos,token_pos-pos) );
    pos=token_pos;
  }
  return result;
}


void googleXY_to_latlng(
  double x, double y, int zoom,
  double& lng, double& lat
)
{
  googleX_to_lng(zoom,x,lng);
  googleY_to_lat(zoom,y,lat);
}
void googleX_to_lng(int zoom, double x, double& lng)
{
  long tilesAtThisZoom =googleMaxTiles(zoom);
  lng = -180.0 + (x * 360.0 / double(tilesAtThisZoom));
}
void googleY_to_lat(int zoom, double y, double& lat)
{
  // This function is the inverse Mercator projection.
  // Look up 'Mercator projection' on Wikipedia for details.
  long   pixels             = googleMaxTiles(zoom) * 256L;
  double pixelsPerLonRadian = pixels / (2.0 * M_PI);
  double bitmapOrigo        = pixels / 2;

  double Y = (bitmapOrigo - y*256) / pixelsPerLonRadian;

  double latRad = 2.0 * ::atan( ::exp(Y) ) - M_PI_2;
  lat = latRad * 180.0 / M_PI;
}


/*
 * Implementation copyright (C) 2004, Alex Tingle.
 */

size_t strlcpy(char* dst, const char* src, size_t bufsize)
{
  size_t srclen =strlen(src);
  size_t result =srclen; /* Result is always the length of the src string */
  if(bufsize>0)
  {
    if(srclen>=bufsize)
       srclen=bufsize-1;
    if(srclen>0)
       memcpy(dst,src,srclen);
    dst[srclen]='\0';
  }
  return result;
}

size_t strlcat(char* dst, const char* src, size_t bufsize)
{
  size_t dstlen =strlen(dst);
  size_t srclen =strlen(src);
  size_t result =dstlen+srclen;
  /* truncate srclen to the buffer */
  if(result>=bufsize)
     srclen=bufsize-dstlen-1;
  if(srclen>0) /* if there is anything left to copy. */
  {
    dst+=dstlen;
    memcpy(dst,src,srclen);
    dst[srclen]='\0';
  }
  return result;
}


int getpid()
{
  return int(::getpid());
}


const char* getenvz(const char* name, const char* dflt)
{
  const char* result =::getenv(name);
  if(!result)
      result=dflt;
  return result;
}


bool exists(const char* path) {
      std::ifstream ifile(path);
      return ifile;
}


} // end namespace FloodUtils
