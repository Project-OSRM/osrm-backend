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

#ifndef GRID_H
#define GRID_H

#include "NASAGridSquare.h"

#include <boost/unordered_map.hpp>
#include <assert.h>

/** Interface to the GridSquares. Template parameter determines which kind of
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
template<class T>
class Grid {
  static          boost::unordered_map<int,T*>            cache;
  static typename boost::unordered_map<int,T*>::size_type cache_size;

public:
  /** The cache_limit defaults to 360. Adjust it to set the number of 
   *  grid squares to store in memory. */
  static typename boost::unordered_map<int,T*>::size_type cache_limit;

  Grid(short sea_level_rise_ = 0 ) : sea_level_rise(sea_level_rise_) {
    // Double check that this compiler truncates towards zero.
    assert(-1 == int(float(-1.9)));
  }

  /** Returns the height above sea level for the given lat/lng. */
  short height(float longitude, float latitude) const  {
    int lng,lat;
    float lng_fraction,lat_fraction;
    split(longitude,lng,lng_fraction);
    split(latitude ,lat,lat_fraction);
    
    int k =T::key(lng,lat);
    typename boost::unordered_map<int,T*>::iterator pos =cache.find(k);
    if(pos==cache.end())
    {
      if(cache_size>=cache_limit)
          clear_cache();
      pos = cache.insert(std::make_pair(k,new T(lng,lat))).first;
      if(pos->second->has_data())
          ++cache_size;
    }
    return pos->second->height(lng_fraction,lat_fraction);
  }

  /** Returns TRUE if the given lng/lat is underwater. */
  bool underwater(float longitude, float latitude) const
  {
    short h =height(longitude,latitude);
    return( h!=NasaGridSquare::NO_DATA && h<=sea_level_rise );
  }
  
private:

  /** Clears the cache of non-null GridSquares. */
  void clear_cache() const {
    cache.clear();
  }

  /** Split a floating point number (num) into integer (i) & fraction (f)
   *  components. */
  void split(float num, int& i, float& f) const {
    if(num>=0.0)
      i=int(num);
    else
      i=int(num)-1;
    f=num-float(i);
  }

  short sea_level_rise;
};

template<class T> boost::unordered_map<int,T*>                     Grid<T>::cache;
template<class T> typename boost::unordered_map<int,T*>::size_type Grid<T>::cache_size =0;
template<class T> typename boost::unordered_map<int,T*>::size_type Grid<T>::cache_limit=360;

#endif // GRID_H
