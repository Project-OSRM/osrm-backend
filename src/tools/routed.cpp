#include "server/server.hpp"
#include "util/exception.hpp"
#include "util/log.hpp"
#include "util/version.hpp"

#include "osrm/engine_config.hpp"
#include "osrm/osrm.hpp"
#include "osrm/storage_config.hpp"

#include <boost/any.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <cstdlib>

#include <signal.h>

#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <memory>
#include <new>
#include <string>
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

const static unsigned INIT_OK_START_ENGINE = 0;
const static unsigned INIT_OK_DO_NOT_START_ENGINE = 1;
const static unsigned INIT_FAILED = -1;

EngineConfig::Algorithm stringToAlgorithm(const std::string &algorithm)
{
    if (algorithm == "CH")
        return EngineConfig::Algorithm::CH;
    if (algorithm == "CoreCH")
        return EngineConfig::Algorithm::CoreCH;
    if (algorithm == "MLD")
        return EngineConfig::Algorithm::MLD;
    throw util::exception("Invalid algorithm name: " + algorithm);
}

// generate boost::program_options object for the routing part
inline unsigned generateServerProgramOptions(const int argc,
                                             const char *argv[],
                                             boost::filesystem::path &base_path,
                                             std::string &ip_address,
                                             int &ip_port,
                                             int &requested_num_threads,
                                             bool &use_shared_memory,
                                             std::string &algorithm,
                                             bool &trial,
                                             int &max_locations_trip,
                                             int &max_locations_viaroute,
                                             int &max_locations_distance_table,
                                             int &max_locations_map_matching,
                                             int &max_results_nearest)
{
    using boost::program_options::value;
    using boost::filesystem::path;

    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()                                         //
        ("version,v", "Show version")("help,h", "Show this help message") //
        ("trial", value<bool>(&trial)->implicit_value(true), "Quit after initialization");

    // declare a group of options that will be allowed on command line
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options() //
        ("ip,i",
         value<std::string>(&ip_address)->default_value("0.0.0.0"),
         "IP address") //
        ("port,p",
         value<int>(&ip_port)->default_value(5000),
         "TCP/IP port") //
        ("threads,t",
         value<int>(&requested_num_threads)->default_value(8),
         "Number of threads to use") //
        ("shared-memory,s",
         value<bool>(&use_shared_memory)->implicit_value(true)->default_value(false),
         "Load data from shared memory") //
        ("algorithm,a",
         value<std::string>(&algorithm)->default_value("CH"),
         "Algorithm to use for the data. Can be CH, CoreCH, MLD.") //
        ("max-viaroute-size",
         value<int>(&max_locations_viaroute)->default_value(500),
         "Max. locations supported in viaroute query") //
        ("max-trip-size",
         value<int>(&max_locations_trip)->default_value(100),
         "Max. locations supported in trip query") //
        ("max-table-size",
         value<int>(&max_locations_distance_table)->default_value(100),
         "Max. locations supported in distance table query") //
        ("max-matching-size",
         value<int>(&max_locations_map_matching)->default_value(100),
         "Max. locations supported in map matching query") //
        ("max-nearest-size",
         value<int>(&max_results_nearest)->default_value(100),
         "Max. results supported in nearest query");

    // hidden options, will be allowed on command line, but will not be shown to the user
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()(
        "base,b", value<boost::filesystem::path>(&base_path), "base path to .osrm file");

    // positional option
    boost::program_options::positional_options_description positional_options;
    positional_options.add("base", 1);

    // combine above options for parsing
    boost::program_options::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_options).add(hidden_options);

    const auto *executable = argv[0];
    boost::program_options::options_description visible_options(
        boost::filesystem::path(executable).filename().string() + " <base.osrm> [<options>]");
    visible_options.add(generic_options).add(config_options);

    // parse command line options
    boost::program_options::variables_map option_variables;
    try
    {
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
                                          .options(cmdline_options)
                                          .positional(positional_options)
                                          .run(),
                                      option_variables);
    }
    catch (const boost::program_options::error &e)
    {
        util::Log(logERROR) << e.what();
        return INIT_FAILED;
    }

    if (option_variables.count("version"))
    {
        std::cout << OSRM_VERSION << std::endl;
        return INIT_OK_DO_NOT_START_ENGINE;
    }

    if (option_variables.count("help"))
    {
        std::cout << visible_options;
        return INIT_OK_DO_NOT_START_ENGINE;
    }

    boost::program_options::notify(option_variables);

    if (!use_shared_memory && option_variables.count("base"))
    {
        return INIT_OK_START_ENGINE;
    }
    else if (use_shared_memory && !option_variables.count("base"))
    {
        return INIT_OK_START_ENGINE;
    }
    else if (use_shared_memory && option_variables.count("base"))
    {
        util::Log(logWARNING) << "Shared memory settings conflict with path settings.";
    }

    std::cout << visible_options;
    return INIT_OK_DO_NOT_START_ENGINE;
}

int main(int argc, const char *argv[]) try
{
    util::LogPolicy::GetInstance().Unmute();

    bool trial_run = false;
    std::string ip_address;
    int ip_port, requested_thread_num;

    EngineConfig config;
    boost::filesystem::path base_path;
    std::string algorithm;
    const unsigned init_result = generateServerProgramOptions(argc,
                                                              argv,
                                                              base_path,
                                                              ip_address,
                                                              ip_port,
                                                              requested_thread_num,
                                                              config.use_shared_memory,
                                                              algorithm,
                                                              trial_run,
                                                              config.max_locations_trip,
                                                              config.max_locations_viaroute,
                                                              config.max_locations_distance_table,
                                                              config.max_locations_map_matching,
                                                              config.max_results_nearest);
    if (init_result == INIT_OK_DO_NOT_START_ENGINE)
    {
        return EXIT_SUCCESS;
    }
    if (init_result == INIT_FAILED)
    {
        return EXIT_FAILURE;
    }
    if (!base_path.empty())
    {
        config.storage_config = storage::StorageConfig(base_path);
    }
    if (!config.IsValid())
    {
        if (base_path.empty() != config.use_shared_memory)
        {
            util::Log(logWARNING) << "Path settings and shared memory conflicts.";
        }
        else
        {
            if (!boost::filesystem::is_regular_file(config.storage_config.ram_index_path))
            {
                util::Log(logWARNING) << config.storage_config.ram_index_path << " is not found";
            }
            if (!boost::filesystem::is_regular_file(config.storage_config.file_index_path))
            {
                util::Log(logWARNING) << config.storage_config.file_index_path << " is not found";
            }
            if (!boost::filesystem::is_regular_file(config.storage_config.hsgr_data_path))
            {
                util::Log(logWARNING) << config.storage_config.hsgr_data_path << " is not found";
            }
            if (!boost::filesystem::is_regular_file(config.storage_config.nodes_data_path))
            {
                util::Log(logWARNING) << config.storage_config.nodes_data_path << " is not found";
            }
            if (!boost::filesystem::is_regular_file(config.storage_config.edges_data_path))
            {
                util::Log(logWARNING) << config.storage_config.edges_data_path << " is not found";
            }
            if (!boost::filesystem::is_regular_file(config.storage_config.core_data_path))
            {
                util::Log(logWARNING) << config.storage_config.core_data_path << " is not found";
            }
            if (!boost::filesystem::is_regular_file(config.storage_config.geometries_path))
            {
                util::Log(logWARNING) << config.storage_config.geometries_path << " is not found";
            }
            if (!boost::filesystem::is_regular_file(config.storage_config.timestamp_path))
            {
                util::Log(logWARNING) << config.storage_config.timestamp_path << " is not found";
            }
            if (!boost::filesystem::is_regular_file(config.storage_config.datasource_names_path))
            {
                util::Log(logWARNING) << config.storage_config.datasource_names_path
                                      << " is not found";
            }
            if (!boost::filesystem::is_regular_file(config.storage_config.datasource_indexes_path))
            {
                util::Log(logWARNING) << config.storage_config.datasource_indexes_path
                                      << " is not found";
            }
            if (!boost::filesystem::is_regular_file(config.storage_config.names_data_path))
            {
                util::Log(logWARNING) << config.storage_config.names_data_path << " is not found";
            }
            if (!boost::filesystem::is_regular_file(config.storage_config.properties_path))
            {
                util::Log(logWARNING) << config.storage_config.properties_path << " is not found";
            }
        }
        return EXIT_FAILURE;
    }
    config.algorithm = stringToAlgorithm(algorithm);

    util::Log() << "starting up engines, " << OSRM_VERSION;

    if (config.use_shared_memory)
    {
        util::Log() << "Loading from shared memory";
    }

    util::Log() << "Threads: " << requested_thread_num;
    util::Log() << "IP address: " << ip_address;
    util::Log() << "IP port: " << ip_port;

#ifndef _WIN32
    int sig = 0;
    sigset_t new_mask;
    sigset_t old_mask;
    sigfillset(&new_mask);
    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);
#endif

    auto routing_server = server::Server::CreateServer(ip_address, ip_port, requested_thread_num);
    auto service_handler = std::make_unique<server::ServiceHandler>(config);

    routing_server->RegisterServiceHandler(std::move(service_handler));

    if (trial_run)
    {
        util::Log() << "trial run, quitting after successful initialization";
    }
    else
    {
        std::packaged_task<int()> server_task([&] {
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
        util::Log() << "running and waiting for requests";
        if (std::getenv("SIGNAL_PARENT_WHEN_READY"))
        {
            kill(getppid(), SIGUSR1);
        }
        sigwait(&wait_mask, &sig);
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
catch (const std::bad_alloc &e)
{
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
