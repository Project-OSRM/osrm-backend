#include "storage/serialization.hpp"
#include "storage/shared_memory.hpp"
#include "storage/shared_monitor.hpp"
#include "storage/storage.hpp"

#include "osrm/exception.hpp"
#include "util/log.hpp"
#include "util/meminfo.hpp"
#include "util/typedefs.hpp"
#include "util/version.hpp"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <csignal>
#include <cstdlib>

using namespace osrm;

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
    generic_options.add_options() //
        ("contraction-heirarchy,ch", "Dump CH graph")("cells,c", "Dump partition structure");

    try
    {
        boost::program_options::store(
            boost::program_options::command_line_parser(argc, argv, generic_options),
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
        removeLocks();
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

[[noreturn]] void CleanupSharedBarriers(int signum)
{ // Here the lock state of named mutexes is unknown, make a hard cleanup
    removeLocks();
    std::_Exit(128 + signum);
}

int main(const int argc, const char *argv[]) try
{
    int signals[] = {SIGTERM, SIGSEGV, SIGINT, SIGILL, SIGABRT, SIGFPE};
    for (auto sig : signals)
    {
        std::signal(sig, CleanupSharedBarriers);
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

    return storage.Run(max_wait, dataset_name, only_metric);
}
catch (const osrm::RuntimeError &e)
{
    util::Log(logERROR) << e.what();
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
