/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.


Custom validators for use with boost::program_options.
*/

#ifndef PROGAM_OPTIONS_H
#define PROGAM_OPTIONS_H

#include "GitDescription.h"
#include "OSRMException.h"
#include "SimpleLogger.h"

#include <boost/any.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/unordered_map.hpp>

#include <fstream>
#include <string>
#include <vector>

typedef boost::unordered_map<
            const std::string,
            boost::filesystem::path
        > ServerPaths;

namespace boost {
    namespace filesystem {
        // Validator for boost::filesystem::path, that verifies that the file
        // exists. The validate() function must be defined in the same namespace
        // as the target type, (boost::filesystem::path in this case), otherwise
        // it is not be called
        inline void validate(
            boost::any & v,
            const std::vector<std::string> & values,
            boost::filesystem::path *,
            int
        ) {
            boost::program_options::validators::check_first_occurrence(v);
            const std::string & input_string =
                boost::program_options::validators::get_single_string(values);
            if(boost::filesystem::is_regular_file(input_string)) {
                v = boost::any(boost::filesystem::path(input_string));
            } else {
                throw OSRMException(input_string);
            }
        }
    }
}

// support old capitalized option names by downcasing them with a regex replace
// read from file and store in a stringstream that can be passed to
// boost::program_options
inline void PrepareConfigFile(
    const boost::filesystem::path& path,
    std::string& output
) {
    std::ifstream config_stream(path.string().c_str());
    std::string input_str(
        (std::istreambuf_iterator<char>(config_stream)),
        std::istreambuf_iterator<char>()
    );
    boost::regex regex( "^([^=]*)" );    //match from start of line to '='
    std::string format( "\\L$1\\E" );    //replace with downcased substring
    output = boost::regex_replace( input_str, regex, format );
}


// generate boost::program_options object for the routing part
inline bool GenerateServerProgramOptions(
    const int argc,
    const char * argv[],
    ServerPaths & paths,
    std::string & ip_address,
    int & ip_port,
    int & requested_num_threads
) {

    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()
        ("version,v", "Show version")
        ("help,h", "Show this help message")
        (
            "config,c",
            boost::program_options::value<boost::filesystem::path>(
                &paths["config"]
            )->default_value("server.ini"),
            "Path to a configuration file"
        );

    // declare a group of options that will be allowed both on command line
    // as well as in a config file
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options()
        (
            "hsgrdata",
            boost::program_options::value<boost::filesystem::path>(&paths["hsgrdata"]),
            ".hsgr file"
        )
        (
            "nodesdata",
            boost::program_options::value<boost::filesystem::path>(&paths["nodesdata"]),
            ".nodes file"
        )
        (
            "edgesdata",
            boost::program_options::value<boost::filesystem::path>(&paths["edgesdata"]),
            ".edges file")
        (
            "ramindex",
            boost::program_options::value<boost::filesystem::path>(&paths["ramindex"]),
            ".ramIndex file")
        (
            "fileindex",
            boost::program_options::value<boost::filesystem::path>(&paths["fileindex"]),
            "File index file")
        (
            "namesdata",
            boost::program_options::value<boost::filesystem::path>(&paths["namesdata"]),
            ".names file")
        (
            "timestamp",
            boost::program_options::value<boost::filesystem::path>(&paths["timestamp"]),
            ".timestamp file")
        (
            "ip,i",
            boost::program_options::value<std::string>(&ip_address)->default_value("0.0.0.0"),
            "IP address"
        )
        (
            "port,p",
            boost::program_options::value<int>(&ip_port)->default_value(5000),
            "TCP/IP port"
        )
        (
            "threads,t",
            boost::program_options::value<int>(&requested_num_threads)->default_value(8),
            "Number of threads to use"
        );

    // hidden options, will be allowed both on command line and in config
    // file, but will not be shown to the user
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()
        (
            "base,b",
            boost::program_options::value<boost::filesystem::path>(&paths["base"]),
            "base path to .osrm file"
        );

    // positional option
    boost::program_options::positional_options_description positional_options;
    positional_options.add("base", 1);

    // combine above options for parsing
    boost::program_options::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_options).add(hidden_options);

    boost::program_options::options_description config_file_options;
    config_file_options.add(config_options).add(hidden_options);

    boost::program_options::options_description visible_options(
        boost::filesystem::basename(argv[0]) + " <base.osrm> [<options>]"
    );
    visible_options.add(generic_options).add(config_options);

    // parse command line options
    boost::program_options::variables_map option_variables;
    boost::program_options::store(
        boost::program_options::command_line_parser(argc, argv).options(cmdline_options).positional(positional_options).run(),
        option_variables
    );

    if(option_variables.count("version")) {
        SimpleLogger().Write() << g_GIT_DESCRIPTION;
        return false;
    }

    if(option_variables.count("help")) {
        SimpleLogger().Write() << visible_options;
        return false;
    }

    boost::program_options::notify(option_variables);

    // parse config file
    ServerPaths::const_iterator path_iterator = paths.find("config");
    if( path_iterator != paths.end() &&
        boost::filesystem::is_regular_file(path_iterator->second)) {
        SimpleLogger().Write() <<
            "Reading options from: " << path_iterator->second.string();
        std::string config_str;
        PrepareConfigFile( paths["config"], config_str );
        std::stringstream config_stream( config_str );
        boost::program_options::store(
            parse_config_file(config_stream, config_file_options),
            option_variables
        );
        boost::program_options::notify(option_variables);
    }

    if(!option_variables.count("hsgrdata")) {
        if(!option_variables.count("base")) {
            throw OSRMException("hsgrdata (or base) must be specified");
        }
        paths["hsgrdata"] = std::string( paths["base"].string()) + ".hsgr";
    }

    if(!option_variables.count("nodesdata")) {
        if(!option_variables.count("base")) {
            throw OSRMException("nodesdata (or base) must be specified");
        }
        paths["nodesdata"] = std::string( paths["base"].c_str()) + ".nodes";
    }

    if(!option_variables.count("edgesdata")) {
        if(!option_variables.count("base")) {
            throw OSRMException("edgesdata (or base) must be specified");
        }
        paths["edgesdata"] = std::string( paths["base"].c_str()) + ".edges";
    }

    if(!option_variables.count("ramindex")) {
        if(!option_variables.count("base")) {
            throw OSRMException("ramindex (or base) must be specified");
        }
        paths["ramindex"] = std::string( paths["base"].c_str()) + ".ramIndex";
    }

    if(!option_variables.count("fileindex")) {
        if(!option_variables.count("base")) {
            throw OSRMException("fileindex (or base) must be specified");
        }
        paths["fileindex"] = std::string( paths["base"].c_str()) + ".fileIndex";
    }

    if(!option_variables.count("namesdata")) {
        if(!option_variables.count("base")) {
            throw OSRMException("namesdata (or base) must be specified");
        }
        paths["namesdata"] = std::string( paths["base"].c_str()) + ".names";
    }

    if(!option_variables.count("timestamp")) {
        if(!option_variables.count("base")) {
            throw OSRMException("timestamp (or base) must be specified");
        }
        paths["timestamp"] = std::string( paths["base"].c_str()) + ".timestamp";
    }

    if(1 > requested_num_threads) {
        throw OSRMException("Number of threads must be a positive number");
    }
    return true;
}

#endif /* PROGRAM_OPTIONS_H */
