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

#include "Util/GitDescription.h"
#include "Util/InputFileUtil.h"
#include "Util/OpenMPWrapper.h"
#include "Util/ProgramOptions.h"
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
        if(!mlockall(MCL_CURRENT | MCL_FUTURE)) {
            SimpleLogger().Write(logWARNING) << "Process " << argv[0] << "could not be locked to RAM";
        }
#endif
#ifdef __linux__

    installCrashHandler(argv[0]);
#endif

        boost::unordered_map<const std::string,boost::filesystem::path> paths;
        std::string ip_address;
        int ip_port, requested_num_threads;

        // declare a group of options that will be allowed only on command line
        boost::program_options::options_description generic_options("Options");
        generic_options.add_options()
            ("version,v", "Show version")
            ("help,h", "Show this help message")
            ("config,c", boost::program_options::value<boost::filesystem::path>(&paths["config"])->default_value("server.ini"),
                  "Path to a configuration file.");

        // declare a group of options that will be allowed both on command line and in config file
        boost::program_options::options_description config_options("Configuration");
        config_options.add_options()
            ("hsgr", boost::program_options::value<boost::filesystem::path>(&paths["hsgr"]),
                "HSGR file")
            ("nodes", boost::program_options::value<boost::filesystem::path>(&paths["nodes"]),
                "Nodes file")
            ("edges", boost::program_options::value<boost::filesystem::path>(&paths["edges"]),
                "Edges file")
            ("ram-index", boost::program_options::value<boost::filesystem::path>(&paths["ramIndex"]),
                "RAM index file")
            ("file-index", boost::program_options::value<boost::filesystem::path>(&paths["fileIndex"]),
                "File index file")
            ("names", boost::program_options::value<boost::filesystem::path>(&paths["names"]),
                "Names file")
            ("timestamp", boost::program_options::value<boost::filesystem::path>(&paths["timestamp"]),
                "Timestamp file")
            ("ip,i", boost::program_options::value<std::string>(&ip_address)->default_value("0.0.0.0"),
                "IP address")
            ("port,p", boost::program_options::value<int>(&ip_port)->default_value(5000),
                "IP Port")
            ("threads,t", boost::program_options::value<int>(&requested_num_threads)->default_value(10), 
                "Number of threads to use");

        // hidden options, will be allowed both on command line and in config file, but will not be shown to the user
        boost::program_options::options_description hidden_options("Hidden options");
        hidden_options.add_options()
            ("input,i", boost::program_options::value<boost::filesystem::path>(&paths["input"]),
                "Input file in .osrm format");

        // positional option
        boost::program_options::positional_options_description positional_options;
        positional_options.add("input", 1);

        // combine above options for parsing
        boost::program_options::options_description cmdline_options;
        cmdline_options.add(generic_options).add(config_options).add(hidden_options);

        boost::program_options::options_description config_file_options;
        config_file_options.add(config_options).add(hidden_options);

        boost::program_options::options_description visible_options(boost::filesystem::basename(argv[0]) + " <input.osrm> [<options>]");
        visible_options.add(generic_options).add(config_options);

        // parse command line options
        boost::program_options::variables_map option_variables;
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
            options(cmdline_options).positional(positional_options).run(), option_variables);

        if(option_variables.count("version")) {
            SimpleLogger().Write() << g_GIT_DESCRIPTION;
            return 0;
        }

        if(option_variables.count("help")) {
            SimpleLogger().Write() << visible_options;
            return 0;
        }

        boost::program_options::notify(option_variables);

        // parse config file
        if(boost::filesystem::is_regular_file(paths["config"])) {
            std::ifstream ifs(paths["config"].c_str());
            SimpleLogger().Write() << "Reading options from: " << paths["config"].c_str();
            boost::program_options::store(parse_config_file(ifs, config_file_options), option_variables);
            boost::program_options::notify(option_variables);
        }

        if(!option_variables.count("hsgr")) {
            paths["hsgr"] = std::string( paths["input"].c_str()) + ".hsgr";
        }

        if(!option_variables.count("nodes")) {
            paths["nodes"] = std::string( paths["input"].c_str()) + ".nodes";
        }

        if(!option_variables.count("edges")) {
            paths["edges"] = std::string( paths["input"].c_str()) + ".edges";
        }

        if(!option_variables.count("ram")) {
            paths["ramIndex"] = std::string( paths["input"].c_str()) + ".ramIndex";
        }

        if(!option_variables.count("index")) {
            paths["fileIndex"] = std::string( paths["input"].c_str()) + ".fileIndex";
        }

        if(!option_variables.count("names")) {
            paths["names"] = std::string( paths["input"].c_str()) + ".names";
        }

        if(!option_variables.count("timestamp")) {
            paths["timestamp"] = std::string( paths["input"].c_str()) + ".timestamp";
        }

        if(!option_variables.count("input")) {
            SimpleLogger().Write(logWARNING) << "No input file specified";
            return -1;
        }

        if(1 > requested_num_threads) {
            SimpleLogger().Write(logWARNING) << "Number of threads must be 1 or larger";
            return -1;
        }

        SimpleLogger().Write() <<
            "starting up engines, " << g_GIT_DESCRIPTION << ", compiled at " << __DATE__ << ", " __TIME__;
        
        SimpleLogger().Write() << "Input file:\t" << paths["input"].c_str();
        SimpleLogger().Write() << "HSGR file:\t" << paths["hsgr"].c_str();
        SimpleLogger().Write() << "Nodes file:\t" << paths["nodes"].c_str();
        SimpleLogger().Write() << "Edges file:\t" << paths["edges"].c_str();
        SimpleLogger().Write() << "RAM file:\t" << paths["ramIndex"].c_str();
        SimpleLogger().Write() << "Index file:\t" << paths["fileIndex"].c_str();
        SimpleLogger().Write() << "Names file:\t" << paths["names"].c_str();
        SimpleLogger().Write() << "Timestamp file:\t" << paths["timestamp"].c_str();
        SimpleLogger().Write() << "Threads:\t" << requested_num_threads;
        SimpleLogger().Write() << "IP address:\t" << ip_address;
        SimpleLogger().Write() << "IP port:\t" << ip_port;

#ifndef _WIN32
        int sig = 0;
        sigset_t new_mask;
        sigset_t old_mask;
        sigfillset(&new_mask);
        pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);
#endif

        OSRM routing_machine(paths);
        Server * s = ServerFactory::CreateServer(ip_address,ip_port,requested_num_threads);
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
       	    SimpleLogger().Write(logDEBUG) << "Threads did not finish within 2 seconds. Hard abort!";
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
