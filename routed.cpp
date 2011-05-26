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

#include <iostream>

#ifdef _OPENMP
#include <omp.h>
#endif

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "Server/ServerConfiguration.h"
#include "Server/ServerFactory.h"

#include "Plugins/ObjectForPluginStruct.h"

#include "Plugins/HelloWorldPlugin.h"
#include "Plugins/LocatePlugin.h"
#include "Plugins/NearestPlugin.h"
#include "Plugins/RoutePlugin.h"
#include "Util/InputFileUtil.h"

using namespace std;

typedef http::RequestHandler RequestHandler;

int main (int argc, char *argv[])
{
    if(testDataFiles(argc, argv)==false) {
        std::cerr << "[error] at least one data file name seems to be bogus!" << std::endl;
        exit(-1);
    }

    try {
        std::cout << "[server] starting up engines, compiled at " << __TIMESTAMP__ << std::endl;
        int sig = 0;
        sigset_t new_mask;
        sigset_t old_mask;
        sigfillset(&new_mask);
        pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

        ServerConfiguration serverConfig("server.ini");
        Server * s = ServerFactory::CreateServer(serverConfig);
        RequestHandler & h = s->GetRequestHandlerPtr();

        ObjectsForQueryStruct * objects = new ObjectsForQueryStruct(serverConfig.GetParameter("hsgrData"),
                serverConfig.GetParameter("ramIndex"),
                serverConfig.GetParameter("fileIndex"),
                serverConfig.GetParameter("nodesData"),
                serverConfig.GetParameter("namesData"));

        BasePlugin * helloWorld = new HelloWorldPlugin();
        h.RegisterPlugin(helloWorld);

        BasePlugin * locate = new LocatePlugin(objects);
        h.RegisterPlugin(locate);

        BasePlugin * nearest = new NearestPlugin(objects);
        h.RegisterPlugin(nearest);

        BasePlugin * route = new RoutePlugin(objects);
        h.RegisterPlugin(route);

        boost::thread t(boost::bind(&Server::Run, s));

        sigset_t wait_mask;
        pthread_sigmask(SIG_SETMASK, &old_mask, 0);
        sigemptyset(&wait_mask);
        sigaddset(&wait_mask, SIGINT);
        sigaddset(&wait_mask, SIGQUIT);
        sigaddset(&wait_mask, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
        std::cout << "[server] running and waiting for requests" << std::endl;
        sigwait(&wait_mask, &sig);
        std::cout << std::endl << "[server] shutting down" << std::endl;
        s->Stop();
        t.join();
        delete s;
        delete objects;
    } catch (std::exception& e) {
        std::cerr << "[fatal error] exception: " << e.what() << std::endl;
    }
    return 0;
}
