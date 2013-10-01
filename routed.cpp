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


#include "Library/OSRM.h"

#include "Server/ServerFactory.h"

#include "Util/IniFile.h"
#include "Util/InputFileUtil.h"
#include "Util/OpenMPWrapper.h"
#include "Util/SimpleLogger.h"
#include "Util/UUID.h"

#ifdef __linux__
#include "Util/LinuxStackTrace.h"
#include <sys/mman.h>
#endif

#include <signal.h>

#include <boost/bind.hpp>
#include <boost/date_time.hpp>
#include <boost/thread.hpp>

#include <iostream>

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

int main (int argc, char * argv[]) {
    try {
        LogPolicy::GetInstance().Unmute();
#ifdef __linux__
        if( !mlockall(MCL_CURRENT | MCL_FUTURE) ) {
            SimpleLogger().Write(logWARNING) <<
                "Process " << argv[0] << "could not be locked to RAM";
        }
        installCrashHandler(argv[0]);
#endif
        SimpleLogger().Write() <<
            "starting up engines, compiled at " << __DATE__ << ", " __TIME__;

#ifndef _WIN32
        int sig = 0;
        sigset_t new_mask;
        sigset_t old_mask;
        sigfillset(&new_mask);
        pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);
#endif

        IniFile server_config((argc > 1 ? argv[1] : "server.ini"));

        bool use_shared_memory = false;
        if(
            server_config.Holds("SharedMemory") &&
            "yes" == server_config.GetParameter("SharedMemory")
        ) {
            use_shared_memory = true;
            SimpleLogger().Write() << "Using data stored in shared memory";
        }

        OSRM routing_machine(
            (argc > 1 ? argv[1] : "server.ini"),
            use_shared_memory
        );

        Server * s = ServerFactory::CreateServer(server_config);
        s->GetRequestHandlerPtr().RegisterRoutingMachine(&routing_machine);

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

        std::cout << "[server] initiating shutdown" << std::endl;
        s->Stop();
        std::cout << "[server] stopping threads" << std::endl;

        if(!t.timed_join(boost::posix_time::seconds(2))) {
       	    SimpleLogger().Write(logDEBUG) <<
                "Threads did not finish within 2 seconds. Hard abort!";
        }

        std::cout << "[server] freeing objects" << std::endl;
        delete s;
        std::cout << "[server] shutdown completed" << std::endl;
    } catch (std::exception& e) {
        std::cerr << "[fatal error] exception: " << e.what() << std::endl;
    }
#ifdef __linux__
    munlockall();
#endif

    return 0;
}
