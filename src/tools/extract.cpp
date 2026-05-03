#include "osrm/exception.hpp"
#include "osrm/extractor.hpp"
#include "osrm/extractor_config.hpp"
#include "util/exception.hpp"
#include "util/log.hpp"
#include "util/meminfo.hpp"
#include "util/version.hpp"

#include <CLI/CLI.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <new>
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
                           extractor::ExtractorConfig &extractor_config)
{
    const auto executable = std::filesystem::path(argv[0]).filename().string();
    CLI::App app{executable + " <input.osm/.osm.bz2/.osm.pbf> [options]", executable};
    app.set_version_flag("-v,--version", std::string{OSRM_VERSION});

    bool list_inputs = false;
    app.add_flag("--list-inputs", list_inputs, "List required and optional input file extensions");

    app.add_option(
           "-l,--verbosity", verbosity, "Log verbosity level: " + util::LogPolicy::GetLevels())
        ->default_val("INFO");

    app.add_option("-p,--profile", extractor_config.profile_path, "Path to LUA routing profile")
        ->default_val("profiles/car.lua");

    app.add_option("-d,--data_version",
                   extractor_config.data_version,
                   "Data version. Leave blank to avoid. osmosis - to get timestamp from file")
        ->default_val("");

    app.add_option(
           "-t,--threads", extractor_config.requested_num_threads, "Number of threads to use")
        ->default_val(std::thread::hardware_concurrency());

    app.add_option("--small-component-size",
                   extractor_config.small_component_size,
                   "Number of nodes required before a strongly-connected-componennt is considered "
                   "big (affects nearest neighbor snapping)")
        ->default_val(1000);

    app.add_flag("--with-osm-metadata",
                 extractor_config.use_metadata,
                 "Use metadata during osm parsing (This can affect the extraction performance).");

    app.add_flag("--parse-conditional-restrictions",
                 extractor_config.parse_conditionals,
                 "Save conditional restrictions found during extraction to disk for use "
                 "during contraction");

    app.add_option("--location-dependent-data",
                   extractor_config.location_dependent_data_paths,
                   "GeoJSON files with location-dependent data");

    app.add_flag("!--disable-location-cache",
                 extractor_config.use_locations_cache,
                 "Use internal nodes locations cache for location-dependent data lookups");

    app.add_flag("--dump-nbg-graph",
                 extractor_config.dump_nbg_graph,
                 "Dump raw node-based graph to *.osrm file for debug purposes.");

    app.add_option("-o,--output",
                   extractor_config.output_path,
                   "Output base path for generated files (default: derived from input file name)");

    app.add_option(
           "input", extractor_config.input_path, "Input file in .osm, .osm.bz2 or .osm.pbf format")
        ->group("");

    bool dummy = false;
    app.add_flag("--generate-edge-lookup", dummy, "Not used anymore")->group("");

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
        extractor::ExtractorConfig config;
        config.ListInputFiles(std::cout);
        return return_code::exit;
    }

    if (extractor_config.input_path.empty())
    {
        std::cout << app.help();
        return return_code::exit;
    }

    return return_code::ok;
}

int main(int argc, char *argv[])
try
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

    if (!extractor_config.output_path.empty())
    {
        extractor_config.UseDefaultOutputNames(extractor_config.output_path);
    }
    else
    {
        extractor_config.UseDefaultOutputNames(extractor_config.input_path);
    }

    if (1 > extractor_config.requested_num_threads)
    {
        util::Log(logERROR) << "Number of threads must be 1 or larger";
        return EXIT_FAILURE;
    }

    if (!std::filesystem::is_regular_file(extractor_config.input_path))
    {
        util::Log(logERROR) << "Input file " << extractor_config.input_path.string()
                            << " not found!";
        return EXIT_FAILURE;
    }

    if (!std::filesystem::is_regular_file(extractor_config.profile_path))
    {
        util::Log(logERROR) << "Profile " << extractor_config.profile_path.string()
                            << " not found!";
        return EXIT_FAILURE;
    }

    osrm::extract(extractor_config);

    util::DumpMemoryStats();

    return EXIT_SUCCESS;
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
catch (const std::system_error &e)
{
    util::DumpMemoryStats();
    util::Log(logERROR) << e.what();
    return e.code().value();
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
