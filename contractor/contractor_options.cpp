/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "contractor_options.hpp"

#include "../util/git_sha.hpp"
#include "../util/simple_logger.hpp"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <tbb/task_scheduler_init.h>

return_code
ContractorOptions::ParseArguments(int argc, char *argv[], ContractorConfig &contractor_config)
{
    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()("version,v", "Show version")("help,h", "Show this help message")(
        "config,c", boost::program_options::value<boost::filesystem::path>(&contractor_config.config_file_path)
                        ->default_value("contractor.ini"),
        "Path to a configuration file.");

    // declare a group of options that will be allowed both on command line and in config file
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options()(
        "restrictions,r",
        boost::program_options::value<boost::filesystem::path>(&contractor_config.restrictions_path),
        "Restrictions file in .osrm.restrictions format")(
        "profile,p", boost::program_options::value<boost::filesystem::path>(&contractor_config.profile_path)
                         ->default_value("profile.lua"),
        "Path to LUA routing profile")(
        "threads,t", boost::program_options::value<unsigned int>(&contractor_config.requested_num_threads)
                         ->default_value(tbb::task_scheduler_init::default_num_threads()),
        "Number of threads to use");

    // hidden options, will be allowed both on command line and in config file, but will not be
    // shown to the user
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()(
        "input,i", boost::program_options::value<boost::filesystem::path>(&contractor_config.osrm_input_path),
        "Input file in .osm, .osm.bz2 or .osm.pbf format");

    // positional option
    boost::program_options::positional_options_description positional_options;
    positional_options.add("input", 1);

    // combine above options for parsing
    boost::program_options::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_options).add(hidden_options);

    boost::program_options::options_description config_file_options;
    config_file_options.add(config_options).add(hidden_options);

    boost::program_options::options_description visible_options(
        "Usage: " + boost::filesystem::basename(argv[0]) + " <input.osrm> [options]");
    visible_options.add(generic_options).add(config_options);

    // parse command line options
    boost::program_options::variables_map option_variables;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
                                      .options(cmdline_options)
                                      .positional(positional_options)
                                      .run(),
                                  option_variables);

    const auto &temp_config_path = option_variables["config"].as<boost::filesystem::path>();
    if (boost::filesystem::is_regular_file(temp_config_path))
    {
        boost::program_options::store(boost::program_options::parse_config_file<char>(
                                          temp_config_path.string().c_str(), cmdline_options, true),
                                      option_variables);
    }

    if (option_variables.count("version"))
    {
        SimpleLogger().Write() << g_GIT_DESCRIPTION;
        return return_code::exit;
    }

    if (option_variables.count("help"))
    {
        SimpleLogger().Write() << "\n" << visible_options;
        return return_code::exit;
    }

    boost::program_options::notify(option_variables);

    if (!option_variables.count("restrictions"))
    {
        contractor_config.restrictions_path = contractor_config.osrm_input_path.string() + ".restrictions";
    }

    if (!option_variables.count("input"))
    {
        SimpleLogger().Write() << "\n" << visible_options;
        return return_code::fail;
    }

    return return_code::ok;
}

void ContractorOptions::GenerateOutputFilesNames(ContractorConfig &contractor_config)
{
    contractor_config.node_output_path = contractor_config.osrm_input_path.string() + ".nodes";
    contractor_config.edge_output_path = contractor_config.osrm_input_path.string() + ".edges";
    contractor_config.geometry_output_path = contractor_config.osrm_input_path.string() + ".geometry";
    contractor_config.graph_output_path = contractor_config.osrm_input_path.string() + ".hsgr";
    contractor_config.rtree_nodes_output_path = contractor_config.osrm_input_path.string() + ".ramIndex";
    contractor_config.rtree_leafs_output_path = contractor_config.osrm_input_path.string() + ".fileIndex";
}
