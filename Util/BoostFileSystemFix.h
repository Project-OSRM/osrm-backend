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

#ifndef BOOST_FILE_SYSTEM_FIX_H
#define BOOST_FILE_SYSTEM_FIX_H

#include <boost/filesystem.hpp>

//This is one big workaround for latest boost renaming woes.

#if BOOST_FILESYSTEM_VERSION < 3
#warning Boost Installation with Filesystem3 missing, activating workaround
#include <cstdio>
namespace boost {
namespace filesystem {
inline path temp_directory_path() {
	char * buffer;
	buffer = tmpnam (NULL);

	return path(buffer);
}

inline path unique_path(const path&) {
	return temp_directory_path();
}

}
}

#endif

#ifndef BOOST_FILESYSTEM_VERSION
#define BOOST_FILESYSTEM_VERSION 3
#endif

#endif /* BOOST_FILE_SYSTEM_FIX_H */
