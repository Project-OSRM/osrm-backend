#include "customizer/customizer.hpp"

#include "osrm/exception.hpp"
#include "util/log.hpp"
#include "util/meminfo.hpp"
#include "util/version.hpp"

#include <CLI/CLI.hpp>

#include <ctime>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
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
    const auto executable = std::filesystem::path(argv[0]).filename().string();
    CLI::App app{executable + " <input.osrm> [options]", executable};
    app.set_version_flag("-v,--version", std::string{OSRM_VERSION});

    bool list_inputs = false;
    app.add_flag("--list-inputs", list_inputs, "List required and optional input file extensions");

    app.add_option(
           "-l,--verbosity", verbosity, "Log verbosity level: " + util::LogPolicy::GetLevels())
        ->default_val("INFO");

    app.add_option(
           "-t,--threads", customization_config.requested_num_threads, "Number of threads to use")
        ->default_val(std::thread::hardware_concurrency());

    app.add_option("--segment-speed-file",
                   customization_config.updater_config.segment_speed_lookup_paths,
                   "Lookup files containing nodeA, nodeB, speed data to adjust edge weights");

    app.add_option("--turn-penalty-file",
                   customization_config.updater_config.turn_penalty_lookup_paths,
                   "Lookup files containing from_, to_, via_nodes, and turn penalties to adjust "
                   "turn weights");

    app.add_option("--edge-weight-updates-over-factor",
                   customization_config.updater_config.log_edge_updates_factor,
                   "Use with `--segment-speed-file`. Provide an `x` factor, by which Extractor "
                   "will log edge weights updated by more than this factor")
        ->default_val(0.0);

    app.add_option("--parse-conditionals-from-now",
                   customization_config.updater_config.valid_now,
                   "Optional for conditional turn restriction parsing, provide a UTC time stamp "
                   "from which to evaluate the validity of conditional turn restrictions")
        ->default_val(std::time_t{0});

    app.add_option("--time-zone-file",
                   customization_config.updater_config.tz_file_path,
                   "Required for conditional turn restriction parsing, provide a geojson file "
                   "containing time zone boundaries")
        ->default_val("");

    app.add_option("input", customization_config.base_path, "Input base file path")->group("");

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::CallForHelp &)
    {
        std::cout << app.help();
        return return_code::exit;
    }
    catch (const CLI::CallForVersion &)
    {
        std::cout << OSRM_VERSION << std::endl;
        return return_code::exit;
    }
    catch (const CLI::ParseError &e)
    {
        util::Log(logERROR) << e.what();
        return return_code::fail;
    }

    if (list_inputs)
    {
        customizer::CustomizationConfig config;
        std::set<std::string> seen;
        config.ListInputFiles(std::cout, seen);
        config.updater_config.ListInputFiles(std::cout, seen);
        return return_code::exit;
    }

    if (customization_config.base_path.empty())
    {
        std::cout << app.help();
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
catch (const util::exception &e)
{
    util::DumpMemoryStats();
    util::Log(logERROR) << e.what();
    return EXIT_FAILURE;
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
