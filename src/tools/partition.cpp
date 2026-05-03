#include "partitioner/partitioner.hpp"
#include "partitioner/partitioner_config.hpp"

#include "osrm/exception.hpp"
#include "util/log.hpp"
#include "util/meminfo.hpp"
#include "util/timing_util.hpp"
#include "util/version.hpp"

#include <CLI/CLI.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
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
                           partitioner::PartitionerConfig &config)
{
    const auto executable = std::filesystem::path(argv[0]).filename().string();
    CLI::App app{executable + " <input.osrm> [options]", executable};
    app.set_version_flag("-v,--version", std::string{OSRM_VERSION});

    bool list_inputs = false;
    app.add_flag("--list-inputs", list_inputs, "List required and optional input file extensions");

    app.add_option(
           "-l,--verbosity", verbosity, "Log verbosity level: " + util::LogPolicy::GetLevels())
        ->default_val("INFO");

    app.add_option("-t,--threads", config.requested_num_threads, "Number of threads to use")
        ->default_val(std::thread::hardware_concurrency());

    app.add_option(
           "--balance", config.balance, "Balance for left and right side in single bisection")
        ->default_val(config.balance);

    app.add_option("--boundary",
                   config.boundary_factor,
                   "Percentage of embedded nodes to contract as sources and sinks")
        ->default_val(config.boundary_factor);

    app.add_option("--optimizing-cuts",
                   config.num_optimizing_cuts,
                   "Number of cuts to use for optimizing a single bisection")
        ->default_val(config.num_optimizing_cuts);

    app.add_option("--small-component-size",
                   config.small_component_size,
                   "Size threshold for small components.")
        ->default_val(config.small_component_size);

    // Bind to a string and split manually after parse. CLI11's delimiter()
    // combined with expected(1) miscounts (post-split values), and without
    // expected(1) the vector binding is greedy across argv tokens — eating
    // the positional input path. Manual split sidesteps both issues.
    std::string max_cell_sizes_str;
    app.add_option("--max-cell-sizes",
                   max_cell_sizes_str,
                   "Maximum cell sizes (comma-separated) starting from the level 1. The first "
                   "cell size value is a bisection termination citerion");

    app.add_option("input", config.base_path, "Input base file path")->group("");

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
        partitioner::PartitionerConfig defaults;
        defaults.ListInputFiles(std::cout);
        return return_code::exit;
    }

    if (config.base_path.empty())
    {
        std::cout << app.help();
        return return_code::fail;
    }

    if (!max_cell_sizes_str.empty())
    {
        config.max_cell_sizes.clear();
        std::stringstream ss(max_cell_sizes_str);
        std::string token;
        while (std::getline(ss, token, ','))
        {
            try
            {
                config.max_cell_sizes.push_back(std::stoull(token));
            }
            catch (const std::exception &)
            {
                util::Log(logERROR) << "--max-cell-sizes: invalid value '" << token << "'";
                return return_code::fail;
            }
        }
    }

    if (!std::is_sorted(config.max_cell_sizes.begin(), config.max_cell_sizes.end()))
    {
        util::Log(logERROR)
            << "The maximum cell sizes array must be sorted in non-descending order.";
        return return_code::fail;
    }

    return return_code::ok;
}

int main(int argc, char *argv[])
try
{
    util::LogPolicy::GetInstance().Unmute();
    std::string verbosity;
    partitioner::PartitionerConfig partition_config;

    const auto result = parseArguments(argc, argv, verbosity, partition_config);

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
    partition_config.UseDefaultOutputNames(partition_config.base_path);

    if (1 > partition_config.requested_num_threads)
    {
        util::Log(logERROR) << "Number of threads must be 1 or larger";
        return EXIT_FAILURE;
    }

    auto check_file = [](const std::filesystem::path &path)
    {
        if (!std::filesystem::is_regular_file(path))
        {
            util::Log(logERROR) << "Input file " << path << " not found!";
            return false;
        }
        else
        {
            return true;
        }
    };

    if (!check_file(partition_config.GetPath(".osrm.ebg")) ||
        !check_file(partition_config.GetPath(".osrm.cnbg_to_ebg")) ||
        !check_file(partition_config.GetPath(".osrm.cnbg")))
    {
        return EXIT_FAILURE;
    }

    util::Log() << "Computing recursive bisection";

    TIMER_START(bisect);
    auto exitcode = partitioner::Partitioner().Run(partition_config);
    TIMER_STOP(bisect);
    util::Log() << "Bisection took " << TIMER_SEC(bisect) << " seconds.";

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
