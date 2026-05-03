#include "server/header_size.hpp"
#include "server/server.hpp"
#include "util/cli_helpers.hpp"
#include "util/exception_utils.hpp"
#include "util/log.hpp"
#include "util/meminfo.hpp"
#include "util/version.hpp"

#include "osrm/datasets.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/exception.hpp"
#include "osrm/osrm.hpp"
#include "osrm/storage_config.hpp"

#include <CLI/CLI.hpp>

#include <cstdlib>

#include <signal.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <future>
#include <iostream>
#include <new>
#include <string>
#include <thread>
#include <vector>

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

const static unsigned INIT_OK_START_ENGINE = 0;
const static unsigned INIT_OK_DO_NOT_START_ENGINE = 1;
const static unsigned INIT_FAILED = -1;

inline unsigned generateServerProgramOptions(const int argc,
                                             const char *argv[],
                                             std::filesystem::path &base_path,
                                             std::string &ip_address,
                                             int &ip_port,
                                             bool &trial,
                                             EngineConfig &config,
                                             int &requested_thread_num,
                                             short &keepalive_timeout,
                                             unsigned &max_header_size)
{
    const auto hardware_threads = std::max<int>(1, std::thread::hardware_concurrency());
    const auto executable = std::filesystem::path(argv[0]).filename().string();

    // EngineConfig defaults these to true for library consumers; reset to
    // false here so the CLI tool only enables them when the flag is passed
    // (matches the original boost::po default_value(false) behavior).
    config.use_shared_memory = false;
    config.use_mmap = false;

    CLI::App app{executable + " <base.osrm> [<options>]", executable};
    app.set_version_flag("-v,--version", std::string{OSRM_VERSION});

    bool list_inputs = false;
    app.add_flag("--list-inputs", list_inputs, "List required and optional input file extensions");

    app.add_option("-l,--verbosity",
                   config.verbosity,
                   "Log verbosity level: " + util::LogPolicy::GetLevels())
#ifdef NDEBUG
        ->default_val("INFO");
#else
        ->default_val("DEBUG");
#endif

    app.add_flag("--trial", trial, "Quit after initialization");

    app.add_option("-i,--ip", ip_address, "IP address")->default_val("0.0.0.0");
    app.add_option("-p,--port", ip_port, "TCP/IP port")->default_val(5000);
    app.add_option("-t,--threads", requested_thread_num, "Number of threads to use")
        ->default_val(hardware_threads);
    app.add_option("-k,--keepalive-timeout",
                   keepalive_timeout,
                   "Default keepalive-timeout. Default: 5 seconds.")
        ->default_val(5);

    app.add_flag("-s,--shared-memory", config.use_shared_memory, "Load data from shared memory");

    app.add_option(
        "--memory_file", config.memory_file, "DEPRECATED: Will behave the same as --mmap.");

    app.add_flag(
        "-m,--mmap", config.use_mmap, "Map datafiles directly, do not use any additional memory.");

    app.add_option(
        "--dataset-name", config.dataset_name, "Name of the shared memory dataset to connect to.");

    app.add_option(
           "-a,--algorithm", config.algorithm, "Algorithm to use for the data. Can be CH, MLD.")
        ->transform(util::cli::algorithm_transform())
        ->default_val("CH");

    app.add_option_function<std::string>(
        "--disable-feature-dataset",
        util::cli::disable_feature_dataset_handler(config.disable_feature_dataset),
        "Disables a feature dataset from being loaded into memory if not needed. "
        "Repeat the flag for multiple values. Options: ROUTE_STEPS, ROUTE_GEOMETRY");

    app.add_option("--max-viaroute-size",
                   config.max_locations_viaroute,
                   "Max. locations supported in viaroute query")
        ->default_val(500);
    app.add_option(
           "--max-trip-size", config.max_locations_trip, "Max. locations supported in trip query")
        ->default_val(100);
    app.add_option("--max-table-size",
                   config.max_locations_distance_table,
                   "Max. locations supported in distance table query")
        ->default_val(100);
    app.add_option("--max-matching-size",
                   config.max_locations_map_matching,
                   "Max. locations supported in map matching query")
        ->default_val(100);
    app.add_option("--max-nearest-size",
                   config.max_results_nearest,
                   "Max. results supported in nearest query")
        ->default_val(100);
    app.add_option("--max-alternatives",
                   config.max_alternatives,
                   "Max. number of alternatives supported in the MLD route query")
        ->default_val(3);

    app.add_option("--max-matching-radius",
                   config.max_radius_map_matching,
                   "Max. radius size supported in map matching query. Default: unlimited.")
        ->transform(util::cli::unlimited_double())
        ->default_val(-1.0);

    app.add_option("--default-radius",
                   config.default_radius,
                   "Default radius size for queries. Default: unlimited.")
        ->transform(util::cli::unlimited_double())
        ->default_val(-1.0);

    app.add_option("--max-header-size",
                   max_header_size,
                   "Maximum size of the HTTP headers (including GET request line). Default: auto "
                   "(based on maximum coordinates).")
        ->default_val(0);

    app.add_option("base", base_path, "base path to .osrm file")->group("");

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::CallForHelp &)
    {
        std::cout << app.help();
        return INIT_OK_DO_NOT_START_ENGINE;
    }
    catch (const CLI::CallForVersion &)
    {
        std::cout << OSRM_VERSION << std::endl;
        return INIT_OK_DO_NOT_START_ENGINE;
    }
    catch (const CLI::ParseError &e)
    {
        util::Log(logERROR) << e.what();
        return INIT_FAILED;
    }

    if (list_inputs)
    {
        storage::StorageConfig storage_config;
        storage_config.ListInputFiles(std::cout);
        return INIT_OK_DO_NOT_START_ENGINE;
    }

    if (max_header_size == 0)
    {
        max_header_size = server::deriveMaxHeaderSize(config);
    }

    const bool has_base = !base_path.empty();
    if (!config.use_shared_memory && has_base)
    {
        return INIT_OK_START_ENGINE;
    }
    else if (config.use_shared_memory && !has_base)
    {
        return INIT_OK_START_ENGINE;
    }
    else if (config.use_shared_memory && has_base)
    {
        util::Log(logWARNING) << "Shared memory settings conflict with path settings.";
    }

    // Adjust number of threads to hardware concurrency
    requested_thread_num = std::min(hardware_threads, requested_thread_num);

    std::cout << app.help();
    return INIT_OK_DO_NOT_START_ENGINE;
}

int main(int argc, const char *argv[])
try
{
    util::LogPolicy::GetInstance().Unmute();

    bool trial_run = false;
    std::string ip_address;
    int ip_port;

    EngineConfig config;
    std::filesystem::path base_path;

    int requested_thread_num = 1;
    short keepalive_timeout = 5;
    // Size of 0 means: Determine automatically based on coordinate limits.
    unsigned max_header_size = 0;
    const unsigned init_result = generateServerProgramOptions(argc,
                                                              argv,
                                                              base_path,
                                                              ip_address,
                                                              ip_port,
                                                              trial_run,
                                                              config,
                                                              requested_thread_num,
                                                              keepalive_timeout,
                                                              max_header_size);
    if (init_result == INIT_OK_DO_NOT_START_ENGINE)
    {
        return EXIT_SUCCESS;
    }
    if (init_result == INIT_FAILED)
    {
        return EXIT_FAILURE;
    }

    util::LogPolicy::GetInstance().SetLevel(config.verbosity);

    if (!base_path.empty())
    {
        config.storage_config = storage::StorageConfig(base_path, config.disable_feature_dataset);
    }
    if (!config.use_shared_memory && !config.storage_config.IsValid())
    {
        util::Log(logERROR) << "Required files are missing, cannot continue";
        return EXIT_FAILURE;
    }
    if (!config.IsValid())
    {
        if (base_path.empty() != config.use_shared_memory)
        {
            util::Log(logWARNING) << "Path settings and shared memory conflicts.";
        }
        return EXIT_FAILURE;
    }

    util::Log() << "starting up engines, " << OSRM_VERSION;

    if (config.use_shared_memory)
    {
        util::Log() << "Loading from shared memory";
    }

    util::Log() << "Threads: " << requested_thread_num;
    util::Log() << "IP address: " << ip_address;
    util::Log() << "IP port: " << ip_port;
    util::Log() << "Keepalive timeout: " << keepalive_timeout;
    util::Log() << "Maximum header size: " << max_header_size;

#ifndef _WIN32
    int sig = 0;
    sigset_t wait_mask;
    sigemptyset(&wait_mask);
    sigaddset(&wait_mask, SIGINT);
    sigaddset(&wait_mask, SIGQUIT);
    sigaddset(&wait_mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &wait_mask, nullptr); // only block necessary signals
#endif

    auto service_handler = std::make_unique<server::ServiceHandler>(config);
    auto routing_server = server::Server::CreateServer(
        ip_address, ip_port, requested_thread_num, keepalive_timeout, max_header_size);

    routing_server->RegisterServiceHandler(std::move(service_handler));

    if (trial_run)
    {
        util::Log() << "trial run, quitting after successful initialization";
    }
    else
    {
        std::packaged_task<int()> server_task(
            [&]
            {
                routing_server->Run();
                return 0;
            });
        auto future = server_task.get_future();
        std::thread server_thread(std::move(server_task));

#ifndef _WIN32
        util::Log() << "running and waiting for requests";
        if (std::getenv("SIGNAL_PARENT_WHEN_READY"))
        {
            kill(getppid(), SIGUSR1);
        }
        sigwait(&wait_mask, &sig);
        util::Log() << "received signal " << sig;
#else
        // Set console control handler to allow server to be stopped.
        console_ctrl_function = std::bind(&server::Server::Stop, routing_server);
        SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
        util::Log() << "running and waiting for requests";
        routing_server->Run();
#endif
        util::Log() << "initiating shutdown";
        routing_server->Stop();
        util::Log() << "stopping threads";

        auto status = future.wait_for(std::chrono::seconds(2));

        if (status == std::future_status::ready)
        {
            server_thread.join();
        }
        else
        {
            util::Log(logWARNING) << "Didn't exit within 2 seconds. Hard abort!";
            std::exit(EXIT_FAILURE);
        }
    }

    util::Log() << "freeing objects";
    routing_server.reset();
    util::Log() << "shutdown completed";
}
catch (const osrm::RuntimeError &e)
{
    util::Log(logERROR) << e.what();
    return e.GetCode();
}
catch (const util::exception &e)
{
    util::Log(logERROR) << e.what();
    return EXIT_FAILURE;
}
catch (const std::bad_alloc &e)
{
    util::DumpMemoryStats();
    util::Log(logWARNING) << "[exception] " << e.what();
    util::Log(logWARNING) << "Please provide more memory or consider using a larger swapfile";
    return EXIT_FAILURE;
}
#ifdef _WIN32
catch (const std::exception &e)
{
    util::Log(logERROR) << "[exception] " << e.what();
    return EXIT_FAILURE;
}
#endif
