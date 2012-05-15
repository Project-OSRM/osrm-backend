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
#ifdef __linux__
#include <sys/mman.h>
#endif
#include <iostream>
#include <signal.h>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "Server/DataStructures/QueryObjectsStorage.h"
#include "Server/ServerConfiguration.h"
#include "Server/ServerFactory.h"

#include "Plugins/HelloWorldPlugin.h"
#include "Plugins/LocatePlugin.h"
#include "Plugins/NearestPlugin.h"
#include "Plugins/TimestampPlugin.h"
#include "Plugins/ViaRoutePlugin.h"

#include "Util/InputFileUtil.h"
#include "Util/OpenMPReplacement.h"

#ifndef _WIN32
#include "Util/LinuxStackTrace.h"
#endif

typedef http::RequestHandler RequestHandler;

#ifdef _WIN32
boost::function0<void> console_ctrl_function;

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
  switch (ctrl_type)
  {
  case CTRL_C_EVENT:
  case CTRL_BREAK_EVENT:
  case CTRL_CLOSE_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    console_ctrl_function();
    return TRUE;
  default:
    return FALSE;
  }
}
#endif

int main (int argc, char *argv[]) {
#ifdef __linux__
    if(!mlockall(MCL_CURRENT | MCL_FUTURE))
        WARN("Process " << argv[0] << "could not be locked to RAM");
#endif
#ifndef _WIN32

    installCrashHandler(argv[0]);
#endif

	// Bug - testing not necessary.  testDataFiles also tries to open the first
	// argv, which is the name of exec file
    //if(testDataFiles(argc, argv)==false) {
        //std::cerr << "[error] at least one data file name seems to be bogus!" << std::endl;
        //exit(-1);
    //}

    try {
        std::cout << "[server] starting up engines, saved at " << __TIMESTAMP__ << std::endl;

#ifndef _WIN32
        int sig = 0;
        sigset_t new_mask;
        sigset_t old_mask;
        sigfillset(&new_mask);
        pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);
#endif

        ServerConfiguration serverConfig("server.ini");
        Server * s = ServerFactory::CreateServer(serverConfig);
        RequestHandler & h = s->GetRequestHandlerPtr();

        QueryObjectsStorage * objects = new QueryObjectsStorage(serverConfig.GetParameter("hsgrData"),
                serverConfig.GetParameter("ramIndex"),
                serverConfig.GetParameter("fileIndex"),
                serverConfig.GetParameter("nodesData"),
                serverConfig.GetParameter("edgesData"),
                serverConfig.GetParameter("namesData"),
                serverConfig.GetParameter("timestamp")
                );

        h.RegisterPlugin(new HelloWorldPlugin());

        h.RegisterPlugin(new LocatePlugin(objects));

        h.RegisterPlugin(new NearestPlugin(objects));

        h.RegisterPlugin(new TimestampPlugin(objects));

        h.RegisterPlugin(new ViaRoutePlugin(objects));

        boost::thread t(boost::bind(&Server::Run, s));

#ifndef _WIN32
        sigset_t wait_mask;
        pthread_sigmask(SIG_SETMASK, &old_mask, 0);
        sigemptyset(&wait_mask);
        sigaddset(&wait_mask, SIGINT);
        sigaddset(&wait_mask, SIGQUIT);
        sigaddset(&wait_mask, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
        std::cout << "[server] running and waiting for requests" << std::endl;
        sigwait(&wait_mask, &sig);
#else
        // Set console control handler to allow server to be stopped.
        console_ctrl_function = boost::bind(&Server::Stop, s);
        SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
        std::cout << "[server] running and waiting for requests" << std::endl;
        s->Run();
#endif

        std::cout << std::endl << "[server] shutting down" << std::endl;
        s->Stop();
        t.join();
        delete s;
        delete objects;
    } catch (std::exception& e) {
        std::cerr << "[fatal error] exception: " << e.what() << std::endl;
    }
#ifdef __linux__
    munlockall();
#endif

    return 0;
}
