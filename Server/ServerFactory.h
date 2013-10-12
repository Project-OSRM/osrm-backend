/*
    open source routing machine
    Copyright (C) Dennis Luxen, 2010

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

#ifndef SERVERFACTORY_H_
#define SERVERFACTORY_H_

#include "Server.h"
#include "../Util/SimpleLogger.h"
#include "../Util/StringUtil.h"

#include <zlib.h>

#include <boost/noncopyable.hpp>
#include <sstream>

struct ServerFactory : boost::noncopyable {
	static Server * CreateServer(std::string& ip_address, int ip_port, int threads) {

		SimpleLogger().Write() <<
			"http 1.1 compression handled by zlib version " << zlibVersion();

        std::stringstream   port_stream;
        port_stream << ip_port;
        return new Server( ip_address, port_stream.str(), std::min( omp_get_num_procs(), threads) );
	}
};

#endif /* SERVERFACTORY_H_ */
