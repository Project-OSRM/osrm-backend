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



#ifndef AZIMUTH_H_
#define AZIMUTH_H_

#include <string>

struct Azimuth {
    static std::string Get(const double heading) {
        if(heading <= 202.5) {
            if(heading >= 0 && heading <= 22.5)
                return "N";
            if(heading > 22.5 && heading <= 67.5)
                return "NE";
            if(heading > 67.5 && heading <= 112.5)
                return "E";
            if(heading > 112.5 && heading <= 157.5)
                return "SE";
            return "S";
        }
        if(heading > 202.5 && heading <= 247.5)
            return "SW";
        if(heading > 247.5 && heading <= 292.5)
            return "W";
        if(heading > 292.5 && heading <= 337.5)
            return "NW";
        return "N";
    }
};


#endif /* AZIMUTH_H_ */
