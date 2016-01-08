#include "extractor/extractor_options.hpp"

#include "util/ini_file.hpp"
#include "util/version.hpp"
#include "util/simple_logger.hpp"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <tbb/task_scheduler_init.h>

namespace osrm
{
namespace extractor
{

return_code
ExtractorOptions::ParseArguments(int argc, char *argv[], ExtractorConfig &extractor_config)
{
    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()("version,v", "Show version")("help,h", "Show this help message")(
        /*
         * TODO: re-enable this
        "restrictions,r",
        boost::program_options::value<boost::filesystem::path>(&extractor_config.restrictions_path),
        "Restrictions file in .osrm.restrictions format")(
        */
        "config,c",
        boost::program_options::value<boost::filesystem::path>(&extractor_config.config_file_path)
            ->default_value("extractor.ini"),
        "Path to a configuration file.");

    // declare a group of options that will be allowed both on command line and in config file
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options()(
        "profile,p",
        boost::program_options::value<boost::filesystem::path>(&extractor_config.profile_path)
            ->default_value("profile.lua"),
        "Path to LUA routing profile")(
        "threads,t",
        boost::program_options::value<unsigned int>(&extractor_config.requested_num_threads)
            ->default_value(tbb::task_scheduler_init::default_num_threads()),
        "Number of threads to use")(
        "generate-edge-lookup",
        boost::program_options::value<bool>(&extractor_config.generate_edge_lookup)
            ->implicit_value(true)
            ->default_value(false),
        "Generate a lookup table for internal edge-expanded-edge IDs to OSM node pairs")(
        "small-component-size",
        boost::program_options::value<unsigned int>(&extractor_config.small_component_size)
            ->default_value(1000),
        "Number of nodes required before a strongly-connected-componennt is considered big "
        "(affects nearest neighbor snapping)");

#ifdef DEBUG_GEOMETRY
    config_options.add_options()("debug-turns", boost::program_options::value<std::string>(
                                                    &extractor_config.debug_turns_path),
                                 "Write out GeoJSON with turn penalty data");
#endif // DEBUG_GEOMETRY

    // hidden options, will be allowed both on command line and in config file, but will not be
    // shown to the user
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()("input,i", boost::program_options::value<boost::filesystem::path>(
                                                &extractor_config.input_path),
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
        boost::filesystem::basename(argv[0]) + " <input.osm/.osm.bz2/.osm.pbf> [options]");
    visible_options.add(generic_options).add(config_options);

    // parse command line options
    try
    {
        boost::program_options::variables_map option_variables;
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
                                          .options(cmdline_options)
                                          .positional(positional_options)
                                          .run(),
                                      option_variables);
        if (option_variables.count("version"))
        {
            util::SimpleLogger().Write() << OSRM_VERSION;
            return return_code::exit;
        }

        if (option_variables.count("help"))
        {
            util::SimpleLogger().Write() << visible_options;
            return return_code::exit;
        }

        boost::program_options::notify(option_variables);

        // parse config file
        if (boost::filesystem::is_regular_file(extractor_config.config_file_path))
        {
            util::SimpleLogger().Write() << "Reading options from: "
                                         << extractor_config.config_file_path.string();
            std::string ini_file_contents =
                util::read_file_lower_content(extractor_config.config_file_path);
            std::stringstream config_stream(ini_file_contents);
            boost::program_options::store(parse_config_file(config_stream, config_file_options),
                                          option_variables);
            boost::program_options::notify(option_variables);
        }

        if (!option_variables.count("input"))
        {
            util::SimpleLogger().Write() << visible_options;
            return return_code::exit;
        }
    }
    catch (std::exception &e)
    {
        util::SimpleLogger().Write(logWARNING) << e.what();
        return return_code::fail;
    }

    return return_code::ok;
}

void ExtractorOptions::GenerateOutputFilesNames(ExtractorConfig &extractor_config)
{
    boost::filesystem::path &input_path = extractor_config.input_path;
    extractor_config.output_file_name = input_path.string();
    extractor_config.restriction_file_name = input_path.string();
    extractor_config.names_file_name = input_path.string();
    extractor_config.timestamp_file_name = input_path.string();
    extractor_config.geometry_output_path = input_path.string();
    extractor_config.edge_output_path = input_path.string();
    extractor_config.edge_graph_output_path = input_path.string();
    extractor_config.node_output_path = input_path.string();
    extractor_config.rtree_nodes_output_path = input_path.string();
    extractor_config.rtree_leafs_output_path = input_path.string();
    extractor_config.edge_segment_lookup_path = input_path.string();
    extractor_config.edge_penalty_path = input_path.string();
    std::string::size_type pos = extractor_config.output_file_name.find(".osm.bz2");
    if (pos == std::string::npos)
    {
        pos = extractor_config.output_file_name.find(".osm.pbf");
        if (pos == std::string::npos)
        {
            pos = extractor_config.output_file_name.find(".osm.xml");
        }
    }
    if (pos == std::string::npos)
    {
        pos = extractor_config.output_file_name.find(".pbf");
    }
    if (pos == std::string::npos)
    {
        pos = extractor_config.output_file_name.find(".osm");
        if (pos == std::string::npos)
        {
            extractor_config.output_file_name.append(".osrm");
            extractor_config.restriction_file_name.append(".osrm.restrictions");
            extractor_config.names_file_name.append(".osrm.names");
            extractor_config.timestamp_file_name.append(".osrm.timestamp");
            extractor_config.geometry_output_path.append(".osrm.geometry");
            extractor_config.node_output_path.append(".osrm.nodes");
            extractor_config.edge_output_path.append(".osrm.edges");
            extractor_config.edge_graph_output_path.append(".osrm.ebg");
            extractor_config.rtree_nodes_output_path.append(".osrm.ramIndex");
            extractor_config.rtree_leafs_output_path.append(".osrm.fileIndex");
            extractor_config.edge_segment_lookup_path.append(".osrm.edge_segment_lookup");
            extractor_config.edge_penalty_path.append(".osrm.edge_penalties");
        }
        else
        {
            extractor_config.output_file_name.replace(pos, 5, ".osrm");
            extractor_config.restriction_file_name.replace(pos, 5, ".osrm.restrictions");
            extractor_config.names_file_name.replace(pos, 5, ".osrm.names");
            extractor_config.timestamp_file_name.replace(pos, 5, ".osrm.timestamp");
            extractor_config.geometry_output_path.replace(pos, 5, ".osrm.geometry");
            extractor_config.node_output_path.replace(pos, 5, ".osrm.nodes");
            extractor_config.edge_output_path.replace(pos, 5, ".osrm.edges");
            extractor_config.edge_graph_output_path.replace(pos, 5, ".osrm.ebg");
            extractor_config.rtree_nodes_output_path.replace(pos, 5, ".osrm.ramIndex");
            extractor_config.rtree_leafs_output_path.replace(pos, 5, ".osrm.fileIndex");
            extractor_config.edge_segment_lookup_path.replace(pos, 5, ".osrm.edge_segment_lookup");
            extractor_config.edge_penalty_path.replace(pos, 5, ".osrm.edge_penalties");
        }
    }
    else
    {
        extractor_config.output_file_name.replace(pos, 8, ".osrm");
        extractor_config.restriction_file_name.replace(pos, 8, ".osrm.restrictions");
        extractor_config.names_file_name.replace(pos, 8, ".osrm.names");
        extractor_config.timestamp_file_name.replace(pos, 8, ".osrm.timestamp");
        extractor_config.geometry_output_path.replace(pos, 8, ".osrm.geometry");
        extractor_config.node_output_path.replace(pos, 8, ".osrm.nodes");
        extractor_config.edge_output_path.replace(pos, 8, ".osrm.edges");
        extractor_config.edge_graph_output_path.replace(pos, 8, ".osrm.ebg");
        extractor_config.rtree_nodes_output_path.replace(pos, 8, ".osrm.ramIndex");
        extractor_config.rtree_leafs_output_path.replace(pos, 8, ".osrm.fileIndex");
        extractor_config.edge_segment_lookup_path.replace(pos, 8, ".osrm.edge_segment_lookup");
        extractor_config.edge_penalty_path.replace(pos, 8, ".osrm.edge_penalties");
    }
}
}
}
