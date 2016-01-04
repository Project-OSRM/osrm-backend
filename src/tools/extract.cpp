#include "extractor/extractor.hpp"
#include "extractor/extractor_options.hpp"
#include "util/simple_logger.hpp"

#include <boost/filesystem.hpp>

#include <cstdlib>
#include <exception>
#include <new>

int main(int argc, char *argv[]) try
{
    LogPolicy::GetInstance().Unmute();
    ExtractorConfig extractor_config;

    const return_code result = ExtractorOptions::ParseArguments(argc, argv, extractor_config);

    if (return_code::fail == result)
    {
        return EXIT_FAILURE;
    }

    if (return_code::exit == result)
    {
        return EXIT_SUCCESS;
    }

    ExtractorOptions::GenerateOutputFilesNames(extractor_config);

    if (1 > extractor_config.requested_num_threads)
    {
        SimpleLogger().Write(logWARNING) << "Number of threads must be 1 or larger";
        return EXIT_FAILURE;
    }

    if (!boost::filesystem::is_regular_file(extractor_config.input_path))
    {
        SimpleLogger().Write(logWARNING) << "Input file " << extractor_config.input_path.string()
                                         << " not found!";
        return EXIT_FAILURE;
    }

    if (!boost::filesystem::is_regular_file(extractor_config.profile_path))
    {
        SimpleLogger().Write(logWARNING) << "Profile " << extractor_config.profile_path.string()
                                         << " not found!";
        return EXIT_FAILURE;
    }
    return extractor(extractor_config).run();
}
catch (const std::bad_alloc &e)
{
    SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    SimpleLogger().Write(logWARNING)
        << "Please provide more memory or consider using a larger swapfile";
    return EXIT_FAILURE;
}
catch (const std::exception &e)
{
    SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    return EXIT_FAILURE;
}
