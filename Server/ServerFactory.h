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

#include <cstdlib>

#include "Server.h"
#include "ServerConfiguration.h"

#include "../Util/InputFileUtil.h"
#include "../Util/OpenMPReplacement.h"

typedef http::Server Server;

struct ServerFactory {
	static Server * CreateServer(ServerConfiguration& serverConfig) {

		if(!testDataFile(serverConfig.GetParameter("nodesData").c_str())) {
			std::cerr << "[error] nodes file not found" << std::endl;
			exit(-1);
		}

		if(!testDataFile(serverConfig.GetParameter("hsgrData").c_str())) {
			std::cerr << "[error] hsgr file not found" << std::endl;
			exit(-1);
		}

		if(!testDataFile(serverConfig.GetParameter("namesData").c_str())) {
			std::cerr << "[error] names file not found" << std::endl;
			exit(-1);
		}

		if(!testDataFile(serverConfig.GetParameter("ramIndex").c_str())) {
			std::cerr << "[error] ram index file not found" << std::endl;
			exit(-1);
		}

		if(!testDataFile(serverConfig.GetParameter("fileIndex").c_str())) {
			std::cerr << "[error] file index file not found" << std::endl;
			exit(-1);
		}

		unsigned threads = omp_get_num_procs();
		if(serverConfig.GetParameter("IP") == "")
			serverConfig.SetParameter("IP", "0.0.0.0");
		if(serverConfig.GetParameter("Port") == "")
			serverConfig.SetParameter("Port", "5000");

		if(atoi(serverConfig.GetParameter("Threads").c_str()) != 0 && (unsigned)atoi(serverConfig.GetParameter("Threads").c_str()) <= threads)
			threads = atoi( serverConfig.GetParameter("Threads").c_str() );

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
