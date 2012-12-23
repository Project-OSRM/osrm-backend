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

 Created on: 26.11.2010
 Author: dennis

 */

#ifndef SERVERFACTORY_H_
#define SERVERFACTORY_H_

#include <zlib.h>

#include "Server.h"
#include "ServerConfiguration.h"

#include "../Util/InputFileUtil.h"
#include "../Util/OpenMPWrapper.h"
#include "../Util/StringUtil.h"

#include "../typedefs.h"

typedef http::Server Server;

struct ServerFactory {
	static Server * CreateServer(ServerConfiguration& serverConfig) {

		if(!testDataFile(serverConfig.GetParameter("nodesData"))) {
			ERR("nodes file not found");
		}

		if(!testDataFile(serverConfig.GetParameter("hsgrData"))) {
		    ERR("hsgr file not found");
		}

		if(!testDataFile(serverConfig.GetParameter("namesData"))) {
		    ERR("names file not found");
		}

		if(!testDataFile(serverConfig.GetParameter("ramIndex"))) {
		    ERR("ram index file not found");
		}

		if(!testDataFile(serverConfig.GetParameter("fileIndex"))) {
		    ERR("file index file not found");
		}

		int threads = omp_get_num_procs();
		if(serverConfig.GetParameter("IP") == "")
			serverConfig.SetParameter("IP", "0.0.0.0");
		if(serverConfig.GetParameter("Port") == "")
			serverConfig.SetParameter("Port", "5000");

		if(stringToInt(serverConfig.GetParameter("Threads")) != 0 && stringToInt(serverConfig.GetParameter("Threads")) <= threads)
			threads = stringToInt( serverConfig.GetParameter("Threads") );

		std::cout << "[server] http 1.1 compression handled by zlib version " << zlibVersion() << std::endl;
		Server * server = new Server(serverConfig.GetParameter("IP"), serverConfig.GetParameter("Port"), threads);
		return server;
	}

	static Server * CreateServer(const char * iniFile) {
		ServerConfiguration serverConfig(iniFile);
		return CreateServer(serverConfig);
	}
};

#endif /* SERVERFACTORY_H_ */
