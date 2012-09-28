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



#ifndef EXTRACTIONHELPERFUNCTIONS_H_
#define EXTRACTIONHELPERFUNCTIONS_H_

#include <climits>

inline bool durationIsValid(const std::string &s) {
    boost::regex e ("((\\d|\\d\\d):)*(\\d|\\d\\d)",boost::regex_constants::icase|boost::regex_constants::perl);

    std::vector< std::string > result;
    boost::algorithm::split_regex( result, s, boost::regex( ":" ) ) ;
    bool matched = regex_match(s, e);
    return matched;
}

inline unsigned parseDuration(const std::string &s) {
    int hours = 0;
    int minutes = 0;
    boost::regex e ("((\\d|\\d\\d):)*(\\d|\\d\\d)",boost::regex_constants::icase|boost::regex_constants::perl);

    std::vector< std::string > result;
    boost::algorithm::split_regex( result, s, boost::regex( ":" ) ) ;
    bool matched = regex_match(s, e);
    if(matched) {
        hours = (result.size()== 2) ?  atoi(result[0].c_str()) : 0;
        minutes = (result.size()== 2) ?  atoi(result[1].c_str()) : atoi(result[0].c_str());
        return 600*(hours*60+minutes);
    }
    return UINT_MAX;
}

inline int parseMaxspeed(std::string input) { //call-by-value on purpose.
    boost::algorithm::to_lower(input);
    int n = atoi(input.c_str());
    if (input.find("mph") != std::string::npos || input.find("mp/h") != std::string::npos) {
        n = (n*1609)/1000;
    }
    return n;
}


#endif /* EXTRACTIONHELPERFUNCTIONS_H_ */
