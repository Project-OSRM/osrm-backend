#include "customizer/customizer.hpp"

#include "osrm/exception.hpp"
#include "util/log.hpp"
#include "util/meminfo.hpp"
#include "util/version.hpp"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <thread>

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
                           customizer::CustomizationConfig &customization_config)
{
    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()("version,v", "Show version")("help,h", "Show this help message")(
        "verbosity,l",
        boost::program_options::value<std::string>(&verbosity)->default_value("INFO"),
        std::string("Log verbosity level: " + util::LogPolicy::GetLevels()).c_str());

    // declare a group of options that will be allowed both on command line
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options()
        //
        ("threads,t",
         boost::program_options::value<unsigned int>(&customization_config.requested_num_threads)
             ->default_value(std::thread::hardware_concurrency()),
         "Number of threads to use")(
            "segment-speed-file",
            boost::program_options::value<std::vector<std::string>>(
                &customization_config.updater_config.segment_speed_lookup_paths)
                ->composing(),
            "Lookup files containing nodeA, nodeB, speed data to adjust edge weights")(
            "turn-penalty-file",
            boost::program_options::value<std::vector<std::string>>(
                &customization_config.updater_config.turn_penalty_lookup_paths)
                ->composing(),
            "Lookup files containing from_, to_, via_nodes, and turn penalties to adjust turn "
            "weights")("edge-weight-updates-over-factor",
                       boost::program_options::value<double>(
                           &customization_config.updater_config.log_edge_updates_factor)
                           ->default_value(0.0),
                       "Use with `--segment-speed-file`. Provide an `x` factor, by which Extractor "
                       "will log edge "
                       "weights updated by more than this factor")(
            "parse-conditionals-from-now",
            boost::program_options::value<std::time_t>(
                &customization_config.updater_config.valid_now)
                ->default_value(0),
            "Optional for conditional turn restriction parsing, provide a UTC time stamp from "
            "which "
            "to evaluate the validity of conditional turn restrictions")(
            "time-zone-file",
            boost::program_options::value<std::string>(
                &customization_config.updater_config.tz_file_path)
                ->default_value(""),
            "Required for conditional turn restriction parsing, provide a geojson file containing "
            "time zone boundaries");

    // hidden options, will be allowed on command line, but will not be
    // shown to the user
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()(
        "input,i",
        boost::program_options::value<boost::filesystem::path>(&customization_config.base_path),
        "Input file in .osrm format");

    // positional option
    boost::program_options::positional_options_description positional_options;
    positional_options.add("input", 1);

    // combine above options for parsing
    boost::program_options::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_options).add(hidden_options);

    const auto *executable = argv[0];
    boost::program_options::options_description visible_options(
        boost::filesystem::path(executable).filename().string() + " <input.osrm> [options]");
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
        return return_code::fail;
    }

    return return_code::ok;
}

int main(int argc, char *argv[])
try
{
    util::LogPolicy::GetInstance().Unmute();
    std::string verbosity;
    customizer::CustomizationConfig customization_config;

    const auto result = parseArguments(argc, argv, verbosity, customization_config);

    if (return_code::fail == result)
    {
        return EXIT_FAILURE;
    }

    if (return_code::exit == result)
    {
        return EXIT_SUCCESS;
    }

    util::LogPolicy::GetInstance().SetLevel(verbosity);

    // set the default in/output names
    customization_config.UseDefaultOutputNames(customization_config.base_path);

    if (1 > customization_config.requested_num_threads)
    {
        util::Log(logERROR) << "Number of threads must be 1 or larger";
        return EXIT_FAILURE;
    }

    if (!customization_config.IsValid())
    {
        return EXIT_FAILURE;
    }

    auto exitcode = customizer::Customizer().Run(customization_config);

    util::DumpMemoryStats();

    return exitcode;
}
catch (const osrm::RuntimeError &e)
{
    util::DumpMemoryStats();
    util::Log(logERROR) << e.what();
    return e.GetCode();
}
catch (const std::bad_alloc &e)
{
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
