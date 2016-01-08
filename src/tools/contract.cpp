#include "contractor/processing_chain.hpp"
#include "contractor/contractor_options.hpp"
#include "util/simple_logger.hpp"

#include <boost/program_options/errors.hpp>

#include <tbb/task_scheduler_init.h>

#include <cstdlib>
#include <exception>
#include <new>
#include <ostream>

using namespace osrm;

int main(int argc, char *argv[]) try
{
    util::LogPolicy::GetInstance().Unmute();
    contractor::ContractorConfig contractor_config;

    const contractor::return_code result =
        contractor::ContractorOptions::ParseArguments(argc, argv, contractor_config);

    if (contractor::return_code::fail == result)
    {
        return EXIT_FAILURE;
    }

    if (contractor::return_code::exit == result)
    {
        return EXIT_SUCCESS;
    }

    contractor::ContractorOptions::GenerateOutputFilesNames(contractor_config);

    if (1 > contractor_config.requested_num_threads)
    {
        util::SimpleLogger().Write(logWARNING) << "Number of threads must be 1 or larger";
        return EXIT_FAILURE;
    }

    const unsigned recommended_num_threads = tbb::task_scheduler_init::default_num_threads();

    if (recommended_num_threads != contractor_config.requested_num_threads)
    {
        util::SimpleLogger().Write(logWARNING)
            << "The recommended number of threads is " << recommended_num_threads
            << "! This setting may have performance side-effects.";
    }

    if (!boost::filesystem::is_regular_file(contractor_config.osrm_input_path))
    {
        util::SimpleLogger().Write(logWARNING)
            << "Input file " << contractor_config.osrm_input_path.string() << " not found!";
        return EXIT_FAILURE;
    }

    if (!boost::filesystem::is_regular_file(contractor_config.profile_path))
    {
        util::SimpleLogger().Write(logWARNING)
            << "Profile " << contractor_config.profile_path.string() << " not found!";
        return EXIT_FAILURE;
    }

    util::SimpleLogger().Write() << "Input file: "
                                 << contractor_config.osrm_input_path.filename().string();
    util::SimpleLogger().Write() << "Profile: "
                                 << contractor_config.profile_path.filename().string();
    util::SimpleLogger().Write() << "Threads: " << contractor_config.requested_num_threads;

    tbb::task_scheduler_init init(contractor_config.requested_num_threads);

    return contractor::Prepare(contractor_config).Run();
}
catch (const std::bad_alloc &e)
{
    util::SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    util::SimpleLogger().Write(logWARNING)
        << "Please provide more memory or consider using a larger swapfile";
    return EXIT_FAILURE;
}
catch (const std::exception &e)
{
    util::SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    return EXIT_FAILURE;
}
