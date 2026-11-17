#include "storage/compact.hpp"
#include "storage/storage_config.hpp"

#include "util/log.hpp"
#include "util/program_options_path.hpp"
#include "util/version.hpp"

#include <boost/program_options.hpp>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>

int main(int argc, char *argv[])
try
{
    osrm::util::LogPolicy::GetInstance().Unmute();
    std::filesystem::path input, output;
    std::string verbosity;
    boost::program_options::options_description options(
        "osrm-compact <input.osrm> --output <new-directory/dataset.osrm>");
    options.add_options()("help,h", "Show this help message")("version,v", "Show version")(
        "verbosity,l",
        boost::program_options::value<std::string>(&verbosity)->default_value("INFO"),
        "Log verbosity level")("list-inputs", "List required and optional input file extensions")(
        "input,i",
        boost::program_options::value<std::filesystem::path>(&input),
        "Prepared MLD input dataset")(
        "output,o",
        boost::program_options::value<std::filesystem::path>(&output),
        "Query dataset base path; its parent directory must not exist");
    boost::program_options::positional_options_description positional;
    positional.add("input", 1);
    boost::program_options::variables_map variables;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
                                      .options(options)
                                      .positional(positional)
                                      .run(),
                                  variables);
    boost::program_options::notify(variables);
    if (variables.count("version"))
    {
        std::cout << OSRM_VERSION << '\n';
        return EXIT_SUCCESS;
    }
    if (variables.count("help"))
    {
        std::cout << options << '\n';
        return EXIT_SUCCESS;
    }
    if (variables.count("list-inputs"))
    {
        for (const auto &suffix : osrm::storage::GetRequiredFiles({}))
            std::cout << "required " << suffix.string() << '\n';
        for (const auto &suffix :
             {".osrm.partition", ".osrm.cells", ".osrm.cell_metrics", ".osrm.mldgr"})
            std::cout << "required " << suffix << '\n';
        for (const auto &suffix :
             {".osrm.openareas", ".osrm.openareas.ramIndex", ".osrm.openareas.fileIndex"})
            std::cout << "optional " << suffix << '\n';
        return EXIT_SUCCESS;
    }
    if (input.empty() || output.empty())
    {
        std::cerr << options << '\n';
        return EXIT_FAILURE;
    }
    osrm::util::LogPolicy::GetInstance().SetLevel(verbosity);
    const auto start = std::chrono::steady_clock::now();
    osrm::storage::compactDataset(input, output);
    osrm::util::Log()
        << "Compaction took "
        << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()
        << " seconds";
    return EXIT_SUCCESS;
}
catch (const std::exception &error)
{
    osrm::util::Log(logERROR) << error.what();
    return EXIT_FAILURE;
}
