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


Custom validators for use with boost::program_options.
*/

#ifndef PROGAM_OPTIONS_H
#define PROGAM_OPTIONS_H

#include "OSRMException.h"

#include <boost/any.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>

#include <string>
#include <vector>

namespace boost {
    namespace filesystem {
        // Validator for boost::filesystem::path, that verifies that the file
        // exists. The validate() function must be defined in the same namespace
        // as the target type, (boost::filesystem::path in this case), otherwise
        // it is not be called
        void validate(
            boost::any & v,
            const std::vector<std::string> & values,
            boost::filesystem::path *,
            int
        ) {
            boost::program_options::validators::check_first_occurrence(v);
            const std::string & input_string =
                boost::program_options::validators::get_single_string(values);
            if(boost::filesystem::is_regular_file(input_string)) {
                v = boost::any(boost::filesystem::path(input_string));
            } else {
                throw OSRMException(input_string);
            }
        }
    }
}

//support old capitalized option names by downcasing them with a regex replace
//read from file and store in a stringstream that can be passed to boost::program_options
inline void PrepareConfigFile(const boost::filesystem::path& path, std::string& output ) {
    std::ifstream config_stream(path.c_str());
    std::string input_str( (std::istreambuf_iterator<char>(config_stream)), std::istreambuf_iterator<char>() );
    boost::regex regex( "^([^=]*)" );    //match from start of line to '='
    std::string format( "\\L$1\\E" );    //replace with downcased substring
    output = boost::regex_replace( input_str, regex, format );
}

#endif /* PROGRAM_OPTIONS_H */
