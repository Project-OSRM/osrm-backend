#include "storage/serialization.hpp"
#include "storage/shared_memory.hpp"
#include "storage/shared_monitor.hpp"
#include "storage/shared_datatype.hpp"
#include "storage/storage.hpp"

#include "osrm/exception.hpp"
#include "util/log.hpp"
#include "util/meminfo.hpp"
#include "util/typedefs.hpp"
#include "util/version.hpp"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/iostreams/tee.hpp>
#include <boost/filesystem.hpp>

#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <locale>

#include <execinfo.h>

using namespace osrm;

void removeLocks(bool alsoDeleteOsrmRegion) { storage::SharedMonitor<storage::SharedRegionRegister>::remove(alsoDeleteOsrmRegion); }

void deleteRegion(const storage::SharedRegionRegister::ShmKey key)
{
    if (storage::SharedMemory::RegionExists(key) && !storage::SharedMemory::Remove(key))
    {
        util::Log(logWARNING) << "could not delete shared memory region " << static_cast<int>(key);
    }
}

void listRegions(bool show_blocks)
{
    osrm::util::Log() << "name\tshm key\ttimestamp\tsize";
    if (!storage::SharedMonitor<storage::SharedRegionRegister>::exists())
    {
        util::Log(logWARNING) << "CTudorache sharedMonitor DOES NOT EXIST: " << (const char *)storage::SharedRegionRegister::name;
        return;
    }
    storage::SharedMonitor<storage::SharedRegionRegister> monitor;
    std::vector<std::string> names;
    const auto &shared_register = monitor.data();
    shared_register.List(std::back_inserter(names));
    for (const auto &name : names)
    {
        auto id = shared_register.Find(name);
        auto region = shared_register.GetRegion(id);
        auto shm = osrm::storage::makeSharedMemory(region.shm_key);
        osrm::util::Log() << "name: " << name
                          << ", shm_key:" << static_cast<int>(region.shm_key)
                          << ", timestamp: " << region.timestamp
                          << ", size: " << shm->Size()
                          << ", shm: " << shm->ToString();

        if (show_blocks)
        {
            using namespace storage;
            auto memory = makeSharedMemory(region.shm_key);
            io::BufferReader reader(reinterpret_cast<char *>(memory->Ptr()), memory->Size());

            std::unique_ptr<BaseDataLayout> layout = std::make_unique<ContiguousDataLayout>();
            serialization::read(reader, *layout);

            std::vector<std::string> block_names;
            layout->List("", std::back_inserter(block_names));
            for (auto &name : block_names)
            {
                osrm::util::Log() << "  " << name << " " << layout->GetBlockSize(name);
            }
        }
    }
}

void springClean()
{
    osrm::util::Log() << "Releasing all locks";
    osrm::util::Log() << "ATTENTION! BE CAREFUL!";
    osrm::util::Log() << "----------------------";
    osrm::util::Log() << "This tool may put osrm-routed into an undefined state!";
    osrm::util::Log() << "Type 'Y' to acknowledge that you know what your are doing.";
    osrm::util::Log() << "\n\nDo you want to purge all shared memory allocated "
                      << "by osrm-datastore? [type 'Y' to confirm]";

    const auto letter = getchar();
    if (letter != 'Y')
    {
        osrm::util::Log() << "aborted.";
    }
    else
    {
        for (auto key : util::irange<storage::SharedRegionRegister::RegionID>(
                 0, storage::SharedRegionRegister::MAX_SHM_KEYS))
        {
            deleteRegion(key);
        }
        removeLocks(true);
    }
}

// generate boost::program_options object for the routing part
bool generateDataStoreOptions(const int argc,
                              const char *argv[],
                              std::string &verbosity,
                              boost::filesystem::path &base_path,
                              int &max_wait,
                              std::string &dataset_name,
                              bool &list_datasets,
                              bool &list_blocks,
                              bool &only_metric)
{
    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()            //
        ("version,v", "Show version")        //
        ("help,h", "Show this help message") //
        ("verbosity,l",
         boost::program_options::value<std::string>(&verbosity)->default_value("INFO"),
         std::string("Log verbosity level: " + util::LogPolicy::GetLevels()).c_str()) //
        ("remove-locks,r", "Remove locks")                                            //
        ("spring-clean,s", "Spring-cleaning all shared memory regions");

    // declare a group of options that will be allowed both on command line
    // as well as in a config file
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options() //
        ("max-wait",
         boost::program_options::value<int>(&max_wait)->default_value(-1),
         "Maximum number of seconds to wait on a running data update "
         "before aquiring the lock by force.") //
        ("dataset-name",
         boost::program_options::value<std::string>(&dataset_name)->default_value(""),
         "Name of the dataset to load into memory. This allows having multiple datasets in memory "
         "at the same time.") //
        ("list",
         boost::program_options::value<bool>(&list_datasets)
             ->default_value(false)
             ->implicit_value(true),
         "List all OSRM datasets currently in memory") //
        ("list-blocks",
         boost::program_options::value<bool>(&list_blocks)
             ->default_value(false)
             ->implicit_value(true),
         "List all OSRM datasets currently in memory")(
            "only-metric",
            boost::program_options::value<bool>(&only_metric)
                ->default_value(false)
                ->implicit_value(true),
            "Only reload the metric data without updating the full dataset. This is an "
            "optimization "
            "for traffic updates.");

    // hidden options, will be allowed on command line but will not be shown to the user
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()("base,b",
                                 boost::program_options::value<boost::filesystem::path>(&base_path),
                                 "base path to .osrm file");

    // positional option
    boost::program_options::positional_options_description positional_options;
    positional_options.add("base", 1);

    // combine above options for parsing
    boost::program_options::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_options).add(hidden_options);

    const auto *executable = argv[0];
    boost::program_options::options_description visible_options(
        boost::filesystem::path(executable).filename().string() + " [<options>] <configuration>");
    visible_options.add(generic_options).add(config_options);

    // print help options if no infile is specified
    if (argc < 2)
    {
        util::Log() << visible_options;
        return false;
    }

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
        return false;
    }

    if (option_variables.count("version"))
    {
        util::Log() << OSRM_VERSION;
        return false;
    }

    if (option_variables.count("help"))
    {
        util::Log() << visible_options;
        return false;
    }

    if (option_variables.count("remove-locks"))
    {
        removeLocks(false);
        return false;
    }

    if (option_variables.count("spring-clean"))
    {
        springClean();
        return false;
    }

    boost::program_options::notify(option_variables);

    return true;
}

void CleanupSharedBarriers(int signum)
{ // Here the lock state of named mutexes is unknown, make a hard cleanup
    util::Log(logERROR) << "[signal] " << signum << " (" << strsignal(signum) << ")";

    removeLocks(false);

    // The signal handler is installed with SA_RESETHAND,
    // so returning from this function will trigger the signal again
    // but it will be handled by the default handler => core dump.
    //std::_Exit(128 + signum);
}

int main(const int argc, const char *argv[])
try
{
    int signals[] = {SIGTERM, SIGSEGV, SIGINT, SIGILL, SIGABRT, SIGFPE};
    for (auto sig : signals)
    {
        struct sigaction act;
        sigemptyset(&act.sa_mask);
        act.sa_flags = SA_RESETHAND;
        act.sa_handler = &CleanupSharedBarriers;
        sigaction(sig, &act, NULL);
        //std::signal(sig, CleanupSharedBarriers);
    }

    util::LogPolicy::GetInstance().Unmute();

    std::string verbosity;
    boost::filesystem::path base_path;
    int max_wait = -1;
    std::string dataset_name;
    bool list_datasets = false;
    bool list_blocks = false;
    bool only_metric = false;
    if (!generateDataStoreOptions(argc,
                                  argv,
                                  verbosity,
                                  base_path,
                                  max_wait,
                                  dataset_name,
                                  list_datasets,
                                  list_blocks,
                                  only_metric))
    {
        return EXIT_SUCCESS;
    }

    util::LogPolicy::GetInstance().SetLevel(verbosity);

    if (list_datasets || list_blocks)
    {
        listRegions(list_blocks);
        return EXIT_SUCCESS;
    }

    storage::StorageConfig config(base_path);
    if (!config.IsValid())
    {
        util::Log(logERROR) << "Config contains invalid file paths. Exiting!";
        return EXIT_FAILURE;
    }
    storage::Storage storage(std::move(config));

    ////////////////////////////////////
    // tmp directory for logs
    const std::string log_dirpath = "/tmp/osrm-datastore-log";
    if (not boost::filesystem::is_directory(log_dirpath)) {
        boost::filesystem::create_directory(log_dirpath);
    }

    // keep last 200 logs (200 x 15 min => 50h = 2 days)
    const unsigned LOG_COUNT_TO_KEEP = 200;
    std::vector<std::string> log_files;
    for (boost::filesystem::directory_iterator it(log_dirpath), end; it != end; ++it) {
        const std::string filepath = it->path().string();
        if (filepath.rfind(".log") != std::string::npos) {
            log_files.push_back(it->path().string());
        }
    }
    std::sort(log_files.begin(), log_files.end(), std::greater<>());
    for (unsigned i = LOG_COUNT_TO_KEEP - 1; i < log_files.size(); ++i) {
        std::cout << "Removing old log file: " << log_files[i] << std::endl;
        boost::filesystem::remove(log_files[i]);
    }

    // tee log to new file
    char log_filepath[256] = {0};
    std::time_t now = std::time(nullptr);
    std::strftime(log_filepath, sizeof(log_filepath), (log_dirpath + "/%Y-%m-%d_%H-%M-%S_" + dataset_name + ".log").c_str(), std::localtime(&now));
    std::ofstream log_file;
    log_file.open(log_filepath);
    std::ostream tmp(std::cout.rdbuf());
    boost::iostreams::tee_device<std::ostream, std::ofstream> log_output_device(tmp, log_file);
    boost::iostreams::stream<boost::iostreams::tee_device<std::ostream, std::ofstream>> logger(log_output_device);
    const auto original_cout = std::cout.rdbuf();
    const auto original_cerr = std::cerr.rdbuf();
    std::cout.rdbuf(logger.rdbuf());
    std::cerr.rdbuf(logger.rdbuf());
    std::cout << "Redirected log to file: " << log_filepath << std::endl;
    ////////////////////////////////////

    int result = storage.Run(max_wait, dataset_name, only_metric);

    // restore cout/cerr after tee log to file
    std::cout << "Closing log file: " << log_filepath << std::endl;
    std::cout.rdbuf(original_cout);
    std::cerr.rdbuf(original_cerr);
    logger.close();
    log_output_device.close();

    std::cout.flush();
    std::cerr.flush();
    // ::sleep(5);
    return result;
}
catch (const osrm::RuntimeError &e)
{
    util::Log(logERROR) << "[RuntimeError] " << e.what();
    return e.GetCode();
}
catch (const std::bad_alloc &e)
{
    util::DumpMemoryStats();
    util::Log(logERROR) << "[exception] " << e.what();
    util::Log(logERROR) << "Please provide more memory or disable locking the virtual "
                           "address space (note: this makes OSRM swap, i.e. slow)";
    return EXIT_FAILURE;
}
