#include "server/server.hpp"
#include "util/exception_utils.hpp"
#include "util/log.hpp"
#include "util/meminfo.hpp"
#include "util/version.hpp"

#include "osrm/engine_config.hpp"
#include "osrm/exception.hpp"
#include "osrm/osrm.hpp"
#include "osrm/storage_config.hpp"

#include <boost/algorithm/string/case_conv.hpp>
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

namespace osrm
{
namespace engine
{
std::istream &operator>>(std::istream &in, EngineConfig::Algorithm &algorithm)
{
    std::string token;
    in >> token;
    boost::to_lower(token);

    if (token == "ch" || token == "corech")
        algorithm = EngineConfig::Algorithm::CH;
    else if (token == "mld")
        algorithm = EngineConfig::Algorithm::MLD;
    else
        throw util::RuntimeError(token, ErrorCode::UnknownAlgorithm, SOURCE_REF);
    return in;
}
} // namespace engine
} // namespace osrm

// generate boost::program_options object for the routing part
inline unsigned generateServerProgramOptions(const int argc,
                                             const char *argv[],
                                             boost::filesystem::path &base_path,
                                             std::string &ip_address,
                                             int &ip_port,
                                             bool &trial,
                                             EngineConfig &config,
                                             int &requested_thread_num)
{
    using boost::filesystem::path;
    using boost::program_options::value;

    const auto hardware_threads = std::max<int>(1, std::thread::hardware_concurrency());

    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()            //
        ("version,v", "Show version")        //
        ("help,h", "Show this help message") //
        ("verbosity,l",
#ifdef NDEBUG
         boost::program_options::value<std::string>(&config.verbosity)->default_value("INFO"),
#else
         boost::program_options::value<std::string>(&config.verbosity)->default_value("DEBUG"),
#endif
         std::string("Log verbosity level: " + util::LogPolicy::GetLevels()).c_str()) //
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
         value<int>(&requested_thread_num)->default_value(hardware_threads),
         "Number of threads to use") //
        ("shared-memory,s",
         value<bool>(&config.use_shared_memory)->implicit_value(true)->default_value(false),
         "Load data from shared memory") //
        ("memory_file",
         value<boost::filesystem::path>(&config.memory_file),
         "DEPRECATED: Will behave the same as --mmap.")(
            "mmap,m",
            value<bool>(&config.use_mmap)->implicit_value(true)->default_value(false),
            "Map datafiles directly, do not use any additional memory.") //
        ("dataset-name",
         value<std::string>(&config.dataset_name),
         "Name of the shared memory dataset to connect to.") //
        ("algorithm,a",
         value<EngineConfig::Algorithm>(&config.algorithm)
             ->default_value(EngineConfig::Algorithm::CH, "CH"),
         "Algorithm to use for the data. Can be CH, CoreCH, MLD.") //
        ("max-viaroute-size",
         value<int>(&config.max_locations_viaroute)->default_value(500),
         "Max. locations supported in viaroute query") //
        ("max-trip-size",
         value<int>(&config.max_locations_trip)->default_value(100),
         "Max. locations supported in trip query") //
        ("max-table-size",
         value<int>(&config.max_locations_distance_table)->default_value(100),
         "Max. locations supported in distance table query") //
        ("max-matching-size",
         value<int>(&config.max_locations_map_matching)->default_value(100),
         "Max. locations supported in map matching query") //
        ("max-nearest-size",
         value<int>(&config.max_results_nearest)->default_value(100),
         "Max. results supported in nearest query") //
        ("max-alternatives",
         value<int>(&config.max_alternatives)->default_value(3),
         "Max. number of alternatives supported in the MLD route query") //
        ("max-matching-radius",
         value<double>(&config.max_radius_map_matching)->default_value(-1.0),
         "Max. radius size supported in map matching query. Default: unlimited.");

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

    if (!config.use_shared_memory && option_variables.count("base"))
    {
        return INIT_OK_START_ENGINE;
    }
    else if (config.use_shared_memory && !option_variables.count("base"))
    {
        return INIT_OK_START_ENGINE;
    }
    else if (config.use_shared_memory && option_variables.count("base"))
    {
        util::Log(logWARNING) << "Shared memory settings conflict with path settings.";
    }

    // Adjust number of threads to hardware concurrency
    requested_thread_num = std::min(hardware_threads, requested_thread_num);

    std::cout << visible_options;
    return INIT_OK_DO_NOT_START_ENGINE;
}

int main(int argc, const char *argv[]) try
{
    util::LogPolicy::GetInstance().Unmute();

    bool trial_run = false;
    std::string ip_address;
    int ip_port;

    EngineConfig config;
    boost::filesystem::path base_path;

    int requested_thread_num = 1;
    const unsigned init_result = generateServerProgramOptions(
        argc, argv, base_path, ip_address, ip_port, trial_run, config, requested_thread_num);
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
        config.storage_config = storage::StorageConfig(base_path);
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
    auto routing_server = server::Server::CreateServer(ip_address, ip_port, requested_thread_num);

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
