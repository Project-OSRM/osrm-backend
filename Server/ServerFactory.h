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
#include "../Util/IniFile.h"
#include "../Util/SimpleLogger.h"
#include "../Util/StringUtil.h"

#include <zlib.h>

#include <boost/noncopyable.hpp>

struct ServerFactory : boost::noncopyable {
	static Server * CreateServer( IniFile & serverConfig ) {
		int threads = omp_get_num_procs();
		if( serverConfig.GetParameter("IP").empty() ) {
			serverConfig.SetParameter("IP", "0.0.0.0");
		}

		if( serverConfig.GetParameter("Port").empty() ) {
			serverConfig.SetParameter("Port", "5000");
		}

		if(
			stringToInt(serverConfig.GetParameter("Threads")) >= 1 &&
			stringToInt(serverConfig.GetParameter("Threads")) <= threads
		) {
			threads = stringToInt( serverConfig.GetParameter("Threads") );
		}

		SimpleLogger().Write() <<
			"http 1.1 compression handled by zlib version " << zlibVersion();

		Server * server = new Server(
			serverConfig.GetParameter("IP"),
			serverConfig.GetParameter("Port"),
			threads
		);
		return server;
	}

	static Server * CreateServer(const char * iniFile) {
		IniFile serverConfig(iniFile);
		return CreateServer(serverConfig);
	}
};

#endif /* SERVERFACTORY_H_ */
