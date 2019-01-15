#include "osrm/exception.hpp"
#include "osrm/extractor.hpp"
#include "osrm/extractor_config.hpp"
#include "util/log.hpp"
#include "util/version.hpp"

#include <tbb/task_scheduler_init.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <cstdlib>
#include <exception>
#include <new>

#include "util/meminfo.hpp"

using namespace osrm;

enum class return_code : unsigned
{
    ok,
    fail,
    exit
};

return_code parseArguments(int argc,
                           char *argv[],
                           std::string &verbosity,
                           extractor::ExtractorConfig &extractor_config)
{
    // declare a group of options that will be a llowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()("version,v", "Show version")("help,h", "Show this help message")(
        "verbosity,l",
        boost::program_options::value<std::string>(&verbosity)->default_value("INFO"),
        std::string("Log verbosity level: " + util::LogPolicy::GetLevels()).c_str());

    // declare a group of options that will be allowed both on command line
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options()(
        "profile,p",
        boost::program_options::value<boost::filesystem::path>(&extractor_config.profile_path)
            ->default_value("profiles/car.lua"),
        "Path to LUA routing profile")(
        "data_version,d",
        boost::program_options::value<std::string>(&extractor_config.data_version)
            ->default_value(""),
        "Data version. Leave blank to avoid. osmosis - to get timestamp from file")(
        "threads,t",
        boost::program_options::value<unsigned int>(&extractor_config.requested_num_threads)
            ->default_value(tbb::task_scheduler_init::default_num_threads()),
        "Number of threads to use")(
        "small-component-size",
        boost::program_options::value<unsigned int>(&extractor_config.small_component_size)
            ->default_value(1000),
        "Number of nodes required before a strongly-connected-componennt is considered big "
        "(affects nearest neighbor snapping)")(
        "with-osm-metadata",
        boost::program_options::bool_switch(&extractor_config.use_metadata)
            ->implicit_value(true)
            ->default_value(false),
        "Use metadata during osm parsing (This can affect the extraction performance).")(
        "parse-conditional-restrictions",
        boost::program_options::bool_switch(&extractor_config.parse_conditionals)
            ->implicit_value(true)
            ->default_value(false),
        "Save conditional restrictions found during extraction to disk for use "
        "during contraction")("location-dependent-data",
                              boost::program_options::value<std::vector<boost::filesystem::path>>(
                                  &extractor_config.location_dependent_data_paths)
                                  ->composing(),
                              "GeoJSON files with location-dependent data")(
        "disable-location-cache",
        boost::program_options::bool_switch(&extractor_config.use_locations_cache)
            ->implicit_value(false)
            ->default_value(true),
        "Use internal nodes locations cache for location-dependent data lookups");

    bool dummy;
    // hidden options, will be allowed on command line, but will not be
    // shown to the user
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()(
        "input,i",
        boost::program_options::value<boost::filesystem::path>(&extractor_config.input_path),
        "Input file in .osm, .osm.bz2 or .osm.pbf format")(
        "generate-edge-lookup",
        boost::program_options::bool_switch(&dummy)->implicit_value(true)->default_value(false),
        "Not used anymore");

    // positional option
    boost::program_options::positional_options_description positional_options;
    positional_options.add("input", 1);

    // combine above options for parsing
    boost::program_options::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_options).add(hidden_options);

    const auto *executable = argv[0];
    boost::program_options::options_description visible_options(
        boost::filesystem::path(executable).filename().string() +
        " <input.osm/.osm.bz2/.osm.pbf> [options]");
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
        return return_code::fail;
    }

    if (option_variables.count("version"))
    {
        std::cout << OSRM_VERSION << std::endl;
        return return_code::exit;
    }

    if (option_variables.count("help"))
    {
        std::cout << visible_options;
        return return_code::exit;
    }

    boost::program_options::notify(option_variables);

    if (!option_variables.count("input"))
    {
        std::cout << visible_options;
        return return_code::exit;
    }

    return return_code::ok;
}

int main(int argc, char *argv[]) try
{
    util::LogPolicy::GetInstance().Unmute();
    extractor::ExtractorConfig extractor_config;
    std::string verbosity;

    const auto result = parseArguments(argc, argv, verbosity, extractor_config);

    if (return_code::fail == result)
    {
        return EXIT_FAILURE;
    }

    if (return_code::exit == result)
    {
        return EXIT_SUCCESS;
    }

    util::LogPolicy::GetInstance().SetLevel(verbosity);

    extractor_config.UseDefaultOutputNames(extractor_config.input_path);

    if (1 > extractor_config.requested_num_threads)
    {
        util::Log(logERROR) << "Number of threads must be 1 or larger";
        return EXIT_FAILURE;
    }

    if (!boost::filesystem::is_regular_file(extractor_config.input_path))
    {
        util::Log(logERROR) << "Input file " << extractor_config.input_path.string()
                            << " not found!";
        return EXIT_FAILURE;
    }

    if (!boost::filesystem::is_regular_file(extractor_config.profile_path))
    {
        util::Log(logERROR) << "Profile " << extractor_config.profile_path.string()
                            << " not found!";
        return EXIT_FAILURE;
    }

    osrm::extract(extractor_config);

    util::DumpSTXXLStats();
    util::DumpMemoryStats();

    return EXIT_SUCCESS;
}
catch (const osrm::RuntimeError &e)
{
    util::DumpSTXXLStats();
    util::DumpMemoryStats();
    util::Log(logERROR) << e.what();
    return e.GetCode();
}
catch (const std::system_error &e)
{
    util::DumpSTXXLStats();
    util::DumpMemoryStats();
    util::Log(logERROR) << e.what();
    return e.code().value();
}
catch (const std::bad_alloc &e)
{
    util::DumpSTXXLStats();
    util::DumpMemoryStats();
    util::Log(logERROR) << "[exception] " << e.what();
    util::Log(logERROR) << "Please provide more memory or consider using a larger swapfile";
    return EXIT_FAILURE;
}
#ifdef _WIN32
catch (const std::exception &e)
{
    util::Log(logERROR) << "[exception] " << e.what() << std::endl;
    return EXIT_FAILURE;
}
#endif
