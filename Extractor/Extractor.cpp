/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
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

#include "Extractor.h"

#include "ExtractionContainers.h"
#include "ExtractionNode.h"
#include "ExtractionWay.h"
#include "ExtractorCallbacks.h"
#include "ExtractorOptions.h"
#include "RestrictionParser.h"
#include "ScriptingEnvironment.h"

#include "../Util/GitDescription.h"
#include "../Util/IniFileUtil.h"
#include "../DataStructures/ConcurrentQueue.h"
#include "../Util/OSRMException.h"
#include "../Util/simple_logger.hpp"
#include "../Util/TimingUtil.h"
#include "../Util/make_unique.hpp"

#include "../typedefs.h"

#include <luabind/luabind.hpp>

#include <osmium/io/any_input.hpp>

#include <tbb/task_scheduler_init.h>

#include <variant/optional.hpp>

#include <cstdlib>

#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>

namespace
{

struct ResultBuffer
{
    std::vector<std::pair<osmium::NodeRef, ExtractionNode>> nodes;
    std::vector<std::pair<osmium::WayNodeList, ExtractionWay>> ways;
    std::vector<mapbox::util::optional<InputRestrictionContainer>> restrictions;
};

int lua_error_callback(lua_State *L) // This is so I can use my own function as an
// exception handler, pcall_log()
{
    luabind::object error_msg(luabind::from_stack(L, -1));
    std::ostringstream error_stream;
    error_stream << error_msg;
    throw OSRMException("ERROR occured in profile script:\n" + error_stream.str());
}
}

int Extractor::Run(int argc, char *argv[])
{
    ExtractorConfig extractor_config;

    try
    {
        LogPolicy::GetInstance().Unmute();
        TIMER_START(extracting);

        if (!ExtractorOptions::ParseArguments(argc, argv, extractor_config))
        {
            return 0;
        }
        ExtractorOptions::GenerateOutputFilesNames(extractor_config);

        if (1 > extractor_config.requested_num_threads)
        {
            SimpleLogger().Write(logWARNING) << "Number of threads must be 1 or larger";
            return 1;
        }

        if (!boost::filesystem::is_regular_file(extractor_config.input_path))
        {
            SimpleLogger().Write(logWARNING)
                << "Input file " << extractor_config.input_path.string() << " not found!";
            return 1;
        }

        if (!boost::filesystem::is_regular_file(extractor_config.profile_path))
        {
            SimpleLogger().Write(logWARNING) << "Profile " << extractor_config.profile_path.string()
                                             << " not found!";
            return 1;
        }

        const unsigned recommended_num_threads = tbb::task_scheduler_init::default_num_threads();

        SimpleLogger().Write() << "Input file: " << extractor_config.input_path.filename().string();
        SimpleLogger().Write() << "Profile: " << extractor_config.profile_path.filename().string();
        SimpleLogger().Write() << "Threads: " << extractor_config.requested_num_threads;
        if (recommended_num_threads != extractor_config.requested_num_threads)
        {
            SimpleLogger().Write(logWARNING) << "The recommended number of threads is "
                                             << recommended_num_threads
                                             << "! This setting may have performance side-effects.";
        }

        tbb::task_scheduler_init init(extractor_config.requested_num_threads);

        /*** Setup Scripting Environment ***/
        ScriptingEnvironment scripting_environment(extractor_config.profile_path.string().c_str());

        std::unordered_map<std::string, NodeID> string_map;
        ExtractionContainers extraction_containers;

        string_map[""] = 0;
        auto extractor_callbacks =
            osrm::make_unique<ExtractorCallbacks>(extraction_containers, string_map);

        osmium::io::File infile(extractor_config.input_path.string());
        osmium::io::Reader reader(infile);
        osmium::io::Header header = reader.header();

        unsigned number_of_nodes = 0;
        unsigned number_of_ways = 0;
        unsigned number_of_relations = 0;
        unsigned number_of_others = 0;

        SimpleLogger().Write() << "Parsing in progress..";
        TIMER_START(parsing);

        std::string generator = header.get("generator");
        if (generator.empty())
        {
            generator = "unknown tool";
        }
        SimpleLogger().Write() << "input file generated by " << generator;

        // write .timestamp data file
        std::string timestamp = header.get("osmosis_replication_timestamp");
        if (timestamp.empty())
        {
            timestamp = "n/a";
        }
        SimpleLogger().Write() << "timestamp: " << timestamp;

        boost::filesystem::ofstream timestamp_out(extractor_config.timestamp_file_name);
        timestamp_out.write(timestamp.c_str(), timestamp.length());
        timestamp_out.close();

        lua_State *lua_state = scripting_environment.getLuaState();
        luabind::set_pcall_callback(&lua_error_callback);

        RestrictionParser restriction_parser(scripting_environment);

        // move to header
        std::atomic_bool parsing_done {false};
        std::atomic_bool loading_done {false};

        ConcurrentQueue<std::shared_ptr<osmium::memory::Buffer>> parse_queue(128);
        ConcurrentQueue<std::shared_ptr<ResultBuffer>> result_queue(128);

        std::thread loading_thread([&]{
            while (osmium::memory::Buffer buffer = reader.read())
            {
                parse_queue.push(std::make_shared<osmium::memory::Buffer>(std::move(buffer)));
            }
            loading_done = true;
        });

        // parsing threads
        while (!loading_done || !parse_queue.empty())
        {
            std::shared_ptr<osmium::memory::Buffer> current_buffer;
            if (!parse_queue.try_pop(current_buffer))
            {
                continue;
            }

            ExtractionNode result_node;
            ExtractionWay result_way;

            std::shared_ptr<ResultBuffer> result_buffer = std::make_shared<ResultBuffer>();;
            for (osmium::OSMEntity &entity : *current_buffer)
            {
                switch (entity.type())
                {
                case osmium::item_type::node:
                    ++number_of_nodes;
                    result_node.Clear();
                    luabind::call_function<void>(lua_state,
                                                 "node_function",
                                                 boost::cref(static_cast<osmium::Node &>(entity)),
                                                 boost::ref(result_node));
                    result_buffer->nodes.emplace_back(osmium::NodeRef{static_cast<osmium::Node &>(entity).id(), static_cast<osmium::Node &>(entity).location()}, result_node);
                    break;
                case osmium::item_type::way:
                    ++number_of_ways;
                    result_way.Clear();
                    luabind::call_function<void>(lua_state,
                                                 "way_function",
                                                 boost::cref(static_cast<osmium::Way &>(entity)),
                                                 boost::ref(result_way));
                    result_way.id = static_cast<osmium::Way &>(entity).id();
                    result_buffer->ways.emplace_back(osmium::WayNodeList{static_cast<osmium::Way &>(entity).nodes()}, result_way);
                    break;
                case osmium::item_type::relation:
                    ++number_of_relations;
                    result_buffer->restrictions.emplace_back(restriction_parser.TryParse(static_cast<osmium::Relation &>(entity)));
                    break;
                default:
                    ++number_of_others;
                    break;
                }
            }
            result_queue.push(result_buffer);
            parsing_done = true;
        }

        while (!parsing_done || !result_queue.empty())
        {
            std::shared_ptr<ResultBuffer> current_buffer;
            if (!result_queue.try_pop(current_buffer))
            {
                for (const auto &node : current_buffer->nodes)
                {
                    extractor_callbacks->ProcessNode(node.first,
                                                     node.second);
                }
                for (auto &way : current_buffer->ways)
                {
                    extractor_callbacks->ProcessWay(way.first,
                                                    way.second);
                }
                for (const auto &restriction : current_buffer->restrictions)
                {
                    extractor_callbacks->ProcessRestriction(restriction);
                }

            }
        }

        loading_thread.join();

        // TODO: join parser threads

        // while (osmium::memory::Buffer buffer = reader.read())
        // {
        //     for (osmium::OSMEntity &entity : buffer)
        //     {
        //         switch (entity.type())
        //         {
        //         case osmium::item_type::node:
        //             ++number_of_nodes;
        //             result_node.Clear();
        //             luabind::call_function<void>(lua_state,
        //                                          "node_function",
        //                                          boost::cref(static_cast<osmium::Node &>(entity)),
        //                                          boost::ref(result_node));
        //             extractor_callbacks->ProcessNode(static_cast<osmium::Node &>(entity),
        //                                              result_node);
        //             break;
        //         case osmium::item_type::way:
        //             ++number_of_ways;
        //             result_way.Clear();
        //             luabind::call_function<void>(lua_state,
        //                                          "way_function",
        //                                          boost::cref(static_cast<osmium::Way &>(entity)),
        //                                          boost::ref(result_way));
        //             extractor_callbacks->ProcessWay(static_cast<osmium::Way &>(entity), result_way);
        //             break;
        //         case osmium::item_type::relation:
        //             ++number_of_relations;
        //             extractor_callbacks->ProcessRestriction(
        //                 restriction_parser.TryParse(static_cast<osmium::Relation &>(entity)));
        //             break;
        //         default:
        //             ++number_of_others;
        //             break;
        //         }
        //     }
        // }
        TIMER_STOP(parsing);
        SimpleLogger().Write() << "Parsing finished after " << TIMER_SEC(parsing) << " seconds";
        SimpleLogger().Write() << "Raw input contains " << number_of_nodes << " nodes, "
                               << number_of_ways << " ways, and " << number_of_relations
                               << " relations";

        extractor_callbacks.reset();

        if (extraction_containers.all_edges_list.empty())
        {
            SimpleLogger().Write(logWARNING) << "The input data is empty, exiting.";
            return 1;
        }

        extraction_containers.PrepareData(extractor_config.output_file_name,
                                          extractor_config.restriction_file_name);

        TIMER_STOP(extracting);
        SimpleLogger().Write() << "extraction finished after " << TIMER_SEC(extracting) << "s";
        SimpleLogger().Write() << "To prepare the data for routing, run: "
                               << "./osrm-prepare " << extractor_config.output_file_name
                               << std::endl;
    }
    catch (boost::program_options::too_many_positional_options_error &)
    {
        SimpleLogger().Write(logWARNING) << "Only one input file can be specified";
        return 1;
    }
    catch (std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << e.what();
        return 1;
    }
    return 0;
}
