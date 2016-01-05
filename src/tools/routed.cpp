#include "server/server.hpp"
#include "util/ini_file.hpp"
#include "util/routed_options.hpp"
#include "util/simple_logger.hpp"

#include "osrm/osrm.hpp"
#include "osrm/libosrm_config.hpp"

#ifdef __linux__
#include <sys/mman.h>
#endif

#include <cstdlib>

#include <signal.h>

#include <chrono>
#include <future>
#include <iostream>
#include <new>
#include <thread>

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

using namespace osrm;

int main(int argc, const char *argv[]) try
{
    util::LogPolicy::GetInstance().Unmute();

    bool trial_run = false;
    std::string ip_address;
    int ip_port, requested_thread_num;

    LibOSRMConfig lib_config;
    const unsigned init_result = util::GenerateServerProgramOptions(
        argc, argv, lib_config.server_paths, ip_address, ip_port, requested_thread_num,
        lib_config.use_shared_memory, trial_run, lib_config.max_locations_trip,
        lib_config.max_locations_viaroute, lib_config.max_locations_distance_table,
        lib_config.max_locations_map_matching);
    if (init_result == util::INIT_OK_DO_NOT_START_ENGINE)
    {
        return EXIT_SUCCESS;
    }
    if (init_result == util::INIT_FAILED)
    {
        return EXIT_FAILURE;
    }

#ifdef __linux__
    struct MemoryLocker final
    {
        explicit MemoryLocker(bool should_lock) : should_lock(should_lock)
        {
            if (should_lock && -1 == mlockall(MCL_CURRENT | MCL_FUTURE))
            {
                could_lock = false;
                util::SimpleLogger().Write(logWARNING) << "memory could not be locked to RAM";
            }
        }
        ~MemoryLocker()
        {
            if (should_lock && could_lock)
                (void)munlockall();
        }
        bool should_lock = false, could_lock = true;
    } memory_locker(lib_config.use_shared_memory);
#endif
    util::SimpleLogger().Write() << "starting up engines, " << OSRM_VERSION;

    if (lib_config.use_shared_memory)
    {
        util::SimpleLogger().Write(logDEBUG) << "Loading from shared memory";
    }

    util::SimpleLogger().Write(logDEBUG) << "Threads:\t" << requested_thread_num;
    util::SimpleLogger().Write(logDEBUG) << "IP address:\t" << ip_address;
    util::SimpleLogger().Write(logDEBUG) << "IP port:\t" << ip_port;

#ifndef _WIN32
    int sig = 0;
    sigset_t new_mask;
    sigset_t old_mask;
    sigfillset(&new_mask);
    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);
#endif

    OSRM osrm_lib(lib_config);
    auto routing_server = server::Server::CreateServer(ip_address, ip_port, requested_thread_num);

    routing_server->GetRequestHandlerPtr().RegisterRoutingMachine(&osrm_lib);

    if (trial_run)
    {
        util::SimpleLogger().Write() << "trial run, quitting after successful initialization";
    }
    else
    {
        std::packaged_task<int()> server_task([&]() -> int
                                              {
                                                  routing_server->Run();
                                                  return 0;
                                              });
        auto future = server_task.get_future();
        std::thread server_thread(std::move(server_task));

#ifndef _WIN32
        sigset_t wait_mask;
        pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
        sigemptyset(&wait_mask);
        sigaddset(&wait_mask, SIGINT);
        sigaddset(&wait_mask, SIGQUIT);
        sigaddset(&wait_mask, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &wait_mask, nullptr);
        util::SimpleLogger().Write() << "running and waiting for requests";
        sigwait(&wait_mask, &sig);
#else
        // Set console control handler to allow server to be stopped.
        console_ctrl_function = std::bind(&server::Server::Stop, routing_server);
        SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
        util::SimpleLogger().Write() << "running and waiting for requests";
        routing_server->Run();
#endif
        util::SimpleLogger().Write() << "initiating shutdown";
        routing_server->Stop();
        util::SimpleLogger().Write() << "stopping threads";

        auto status = future.wait_for(std::chrono::seconds(2));

        if (status == std::future_status::ready)
        {
            server_thread.join();
        }
        else
        {
            util::SimpleLogger().Write(logWARNING) << "Didn't exit within 2 seconds. Hard abort!";
            server_task.reset(); // just kill it
        }
    }

    util::SimpleLogger().Write() << "freeing objects";
    routing_server.reset();
    util::SimpleLogger().Write() << "shutdown completed";
}
catch (const std::bad_alloc &e)
{
    util::SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    util::SimpleLogger().Write(logWARNING)
        << "Please provide more memory or consider using a larger swapfile";
    return EXIT_FAILURE;
}
catch (const std::exception &e)
{
    util::SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    return EXIT_FAILURE;
}
