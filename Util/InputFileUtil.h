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

#ifndef INPUTFILEUTIL_H_
#define INPUTFILEUTIL_H_

#include "../typedefs.h"

#include <boost/filesystem.hpp>

// Check if file exists and if it can be opened for reading with ifstream an object
inline bool testDataFile(const std::string & filename){
    boost::filesystem::path fileToTest(filename);
	if(!boost::filesystem::exists(fileToTest))       {
		SimpleLogger().Write(logWARNING) <<
			"Failed to open file " << filename << " for reading.";

		return false;
	}
	return true;
}

inline bool testDataFiles(int argc, char *argv[]){
	for(int i = 0; i < argc; ++i) {
		if(!testDataFile(argv[i]))
			return false;
	}
	return true;
}

#endif /* INPUTFILEUTIL_H_ */
