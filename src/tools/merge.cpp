#include "osrm/merger.hpp"
#include "osrm/merger_config.hpp"
#include "util/log.hpp"
#include "util/version.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <cstdlib>
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
                           merger::MergerConfig &merger_config)
{
    // declare a group of options that will be a allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()("version,v", "Show version")("help,h", "Show this help message");

    // declare a group of options that will be allowed both on command line
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options()(
        "threads,t",
        boost::program_options::value<unsigned int>(&merger_config.requested_num_threads)
            ->default_value(std::thread::hardware_concurrency()),
        "Number of threads to use")(
        "output,o",
        boost::program_options::value<boost::filesystem::path>(&merger_config.output_path)
            ->default_value(boost::filesystem::path("merged")),
        "Prefix for the output files");

    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()(
        "profiles_to_input_maps",
        boost::program_options::value<std::vector<std::string>>()->multitoken(),
        "list of pairs <profile_file>:<input_map1>,...<input_mapN>");

    // positional option
    boost::program_options::positional_options_description positional_options;
    positional_options.add("profiles_to_input_maps", -1);

    // combine above options for parsing
    boost::program_options::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_options).add(hidden_options);

    const auto *executable = argv[0];
    boost::program_options::options_description visible_options(
        boost::filesystem::path(executable).filename().string() +
        " <.lua>:[<.osm.pbf>,...] <.lua>:[<.osm.pbf>,...] ...");
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

    if (option_variables["profiles_to_input_maps"].empty()) {
        std::cout << visible_options;
        return return_code::exit;
    }
    std::vector<std::string> pairs = option_variables["profiles_to_input_maps"].as<std::vector<std::string>>();
    for (const auto &pair : pairs)
    {
        std::vector<std::string> tokens, input_maps;
        boost::split(tokens, pair, boost::is_any_of(":"));
        if (tokens.size() != 2)
        {
            std::cout << visible_options;
            return return_code::exit;
        }

        boost::filesystem::path profile_path(tokens[0]);
        boost::split(input_maps, tokens[1], boost::is_any_of(","));
        std::vector<boost::filesystem::path> input_maps_path;
        for (const auto &map : input_maps)
        {
            input_maps_path.push_back(boost::filesystem::path(map));
        }
        merger_config.profile_to_input.insert({profile_path, input_maps_path});
    }

    return return_code::ok;
}

int main(int argc, char *argv[])
{
    util::LogPolicy::GetInstance().Unmute();
    merger::MergerConfig merger_config;

    const auto result = parseArguments(argc, argv, merger_config);

    if (return_code::fail == result)
    {
        return EXIT_FAILURE;
    }
    if (return_code::exit == result)
    {
        return EXIT_SUCCESS;
    }

    merger_config.UseDefaultOutputNames(merger_config.output_path);

    for (const auto &pair : merger_config.profile_to_input)
    {
        if (!boost::filesystem::is_regular_file(pair.first))
        {
            util::Log(logERROR) << "Profile " << pair.first.string()
                                << " not found!";
            return EXIT_FAILURE;
        }
        for (const auto &input_map : pair.second)
        {
            if (!boost::filesystem::is_regular_file(input_map))
            {
                util::Log(logERROR) << "Input file " << input_map.string()
                                    << " not found!";
                return EXIT_FAILURE;
            }
        }
    }

    util::Log(logINFO) << "Starting merge...";

    osrm::merge(merger_config);
}
