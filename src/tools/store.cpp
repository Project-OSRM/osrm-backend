#include "storage/storage.hpp"
#include "util/exception.hpp"
#include "util/simple_logger.hpp"
#include "util/typedefs.hpp"
#include "util/version.hpp"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

using namespace osrm;

// generate boost::program_options object for the routing part
bool generateDataStoreOptions(const int argc, const char *argv[], boost::filesystem::path& base_path)
{
    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()("version,v", "Show version")("help,h", "Show this help message")(
        "springclean,s", "Remove all regions in shared memory");

    // declare a group of options that will be allowed both on command line
    // as well as in a config file
    boost::program_options::options_description config_options("Configuration");

    // hidden options, will be allowed on command line but will not be shown to the user
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()(
        "base,b", boost::program_options::value<boost::filesystem::path>(&base_path),
        "base path to .osrm file");

    // positional option
    boost::program_options::positional_options_description positional_options;
    positional_options.add("base", 1);

    // combine above options for parsing
    boost::program_options::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_options).add(hidden_options);

    const auto* executable = argv[0];
    boost::program_options::options_description visible_options(
        boost::filesystem::path(executable).filename().string() + " [<options>] <configuration>");
    visible_options.add(generic_options).add(config_options);

    // print help options if no infile is specified
    if (argc < 2)
    {
        util::SimpleLogger().Write() << visible_options;
        return false;
    }

    // parse command line options
    boost::program_options::variables_map option_variables;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
                                      .options(cmdline_options)
                                      .positional(positional_options)
                                      .run(),
                                  option_variables);

    if (option_variables.count("version"))
    {
        util::SimpleLogger().Write() << OSRM_VERSION;
        return false;
    }

    if (option_variables.count("help"))
    {
        util::SimpleLogger().Write() << visible_options;
        return false;
    }

    boost::program_options::notify(option_variables);

    return true;
}

int main(const int argc, const char *argv[]) try
{
    util::LogPolicy::GetInstance().Unmute();

    boost::filesystem::path base_path;
    if (!generateDataStoreOptions(argc, argv, base_path))
    {
        return EXIT_SUCCESS;
    }
    storage::StorageConfig config(base_path);
    if (!config.IsValid())
    {
        util::SimpleLogger().Write(logWARNING) << "Config contains invalid file paths. Exiting!";
        return EXIT_FAILURE;
    }
    storage::Storage storage(std::move(config));
    return storage.Run();
}
catch (const std::bad_alloc &e)
{
    util::SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    util::SimpleLogger().Write(logWARNING)
        << "Please provide more memory or disable locking the virtual "
           "address space (note: this makes OSRM swap, i.e. slow)";
    return EXIT_FAILURE;
}
catch (const std::exception &e)
{
    util::SimpleLogger().Write(logWARNING) << "caught exception: " << e.what();
    return EXIT_FAILURE;
}
