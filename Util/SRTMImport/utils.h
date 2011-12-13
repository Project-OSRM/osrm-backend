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

#ifndef FLOOD_UTILS_H
#define FLOOD_UTILS_H

#include <string.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <list>

/*
#define ERR(x) { \
  std::stringstream s; \
  s<<FloodUtils::getpid()<<": "<<x<<"\n"; \
  fprintf(stderr,s.str().c_str()); \
}
*/

#define ROOT_PATH "/opt/storage/srtm/Eurasia"

#if DEBUG_OUTPUT
#  define DEBUG_ONLY(x) x
#  define DB(x) ERR("[DEBUG] "<<x)
#else
#  define DEBUG_ONLY(x)
#  define DB(x)
#endif

namespace FloodUtils {

  /** Integer square root.
   *  Algorithm by Jack W. Crenshaw: http://www.embedded.com/98/9802fe2.htm
   */
  unsigned short sqrt(unsigned long a);

  /** Tokenise a string. */
  std::list<std::string> split(const std::string& s, char token=',');

  void googleXY_to_latlng(
    double x, double y, int zoom,
    double& lng, double& lat
  );
  void googleX_to_lng(int zoom, double x, double& lng);
  void googleY_to_lat(int zoom, double y, double& lat);
  
  /** Calculate the number of tiles in x or y dimensions at a zoom level. */
  inline long googleMaxTiles(int zoom) { return( 1L << (17 - zoom) ); }

  /* Specs. & semantics from:
   * http://www.courtesan.com/todd/papers/strlcpy.html
   */
  size_t strlcpy(char* dst, const char* src, size_t bufsize);
  size_t strlcat(char* dst, const char* src, size_t bufsize);
  
  /** Convenience function for debug output */
  int getpid();
  
  /** Convenience function for getting env vars as strings. */
  const char* getenvz(const char* name, const char* dflt ="");
  
  /** Check for the existence of the names file. */
  bool exists(const char* path);
}


#endif // FLOOD_UTILS_H
