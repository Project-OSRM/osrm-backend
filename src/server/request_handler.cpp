#include "server/request_handler.hpp"

#include "server/api_grammar.hpp"
#include "server/http/reply.hpp"
#include "server/http/request.hpp"

#include "util/json_renderer.hpp"
#include "util/simple_logger.hpp"
#include "util/string_util.hpp"
#include "util/xml_renderer.hpp"
#include "util/typedefs.hpp"

#include "engine/route_parameters.hpp"
#include "util/json_container.hpp"
#include "osrm/osrm.hpp"

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <ctime>

#include <algorithm>
#include <iostream>
#include <string>

namespace osrm
{
namespace server
{

RequestHandler::RequestHandler() : routing_machine(nullptr) {}

void RequestHandler::handle_request(const http::request &current_request,
                                    http::reply &current_reply)
{
    util::json::Object json_result;

    // parse command
    try
    {
        std::string request_string;
        util::URIDecode(current_request.uri, request_string);

        // deactivated as GCC apparently does not implement that, not even in 4.9
        // std::time_t t = std::time(nullptr);
        // util::SimpleLogger().Write() << std::put_time(std::localtime(&t), "%m-%d-%Y %H:%M:%S") <<
        //     " " << current_request.endpoint.to_string() << " " <<
        //     current_request.referrer << ( 0 == current_request.referrer.length() ? "- " :" ") <<
        //     current_request.agent << ( 0 == current_request.agent.length() ? "- " :" ") <<
        //     request;

        time_t ltime;
        struct tm *time_stamp;

        ltime = time(nullptr);
        time_stamp = localtime(&ltime);

        // log timestamp
        util::SimpleLogger().Write()
            << (time_stamp->tm_mday < 10 ? "0" : "") << time_stamp->tm_mday << "-"
            << (time_stamp->tm_mon + 1 < 10 ? "0" : "") << (time_stamp->tm_mon + 1) << "-"
            << 1900 + time_stamp->tm_year << " " << (time_stamp->tm_hour < 10 ? "0" : "")
            << time_stamp->tm_hour << ":" << (time_stamp->tm_min < 10 ? "0" : "")
            << time_stamp->tm_min << ":" << (time_stamp->tm_sec < 10 ? "0" : "")
            << time_stamp->tm_sec << " " << current_request.endpoint.to_string() << " "
            << current_request.referrer << (0 == current_request.referrer.length() ? "- " : " ")
            << current_request.agent << (0 == current_request.agent.length() ? "- " : " ")
            << request_string;

        engine::RouteParameters route_parameters;
        APIGrammarParser api_parser(&route_parameters);

        auto api_iterator = request_string.begin();
        const bool result =
            boost::spirit::qi::parse(api_iterator, request_string.end(), api_parser);

        // check if the was an error with the request
        if (result && api_iterator == request_string.end())
        {
            // parsing done, lets call the right plugin to handle the request
            BOOST_ASSERT_MSG(routing_machine != nullptr, "pointer not init'ed");

            if (!route_parameters.jsonp_parameter.empty())
            { // prepend response with jsonp parameter
                const std::string json_p = (route_parameters.jsonp_parameter + "(");
                current_reply.content.insert(current_reply.content.end(), json_p.begin(),
                                             json_p.end());
            }

            const int return_code = routing_machine->RunQuery(route_parameters, json_result);
            json_result.values["status"] = return_code;
            // 4xx bad request return code
            if (return_code / 100 == 4)
            {
                current_reply.status = http::reply::bad_request;
                current_reply.content.clear();
                route_parameters.output_format.clear();
            }
            else
            {
                // 2xx valid request
                BOOST_ASSERT(return_code / 100 == 2);
            }
        }
        else
        {
            const auto position = std::distance(request_string.begin(), api_iterator);

            current_reply.status = http::reply::bad_request;
            json_result.values["status"] = http::reply::bad_request;
            json_result.values["status_message"] =
                "Query string malformed close to position " + std::to_string(position);
        }

        current_reply.headers.emplace_back("Access-Control-Allow-Origin", "*");
        current_reply.headers.emplace_back("Access-Control-Allow-Methods", "GET");
        current_reply.headers.emplace_back("Access-Control-Allow-Headers",
                                           "X-Requested-With, Content-Type");

        if ("gpx" == route_parameters.output_format)
        { // gpx file
            util::json::gpx_render(current_reply.content, json_result.values["route"]);
            current_reply.headers.emplace_back("Content-Type",
                                               "application/gpx+xml; charset=UTF-8");
            current_reply.headers.emplace_back("Content-Disposition",
                                               "attachment; filename=\"route.gpx\"");
        }
        else if (route_parameters.service == "tile") {

            /*
            std::istringstream is(json_result.values["pbf"].get<osrm::util::json::String>().value);
            boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
            //in.push(boost::iostreams::gzip_compressor());
            in.push(boost::iostreams::zlib_compressor());
            in.push(is);

            std::ostringstream os;

            boost::iostreams::copy(in,os);

            auto s = os.str();

            std::copy(s.cbegin(),s.cend(), std::back_inserter(current_reply.content));
            */
            std::copy(json_result.values["pbf"].get<osrm::util::json::Buffer>().value.cbegin(),
                      json_result.values["pbf"].get<osrm::util::json::Buffer>().value.cend(),
                      std::back_inserter(current_reply.content));

            //current_reply.content.append(json_result.values["pbf"].get<osrm::util::json::String>().value
            current_reply.headers.emplace_back("Content-Type",
                                               "application/x-protobuf");
        }
        else if (route_parameters.jsonp_parameter.empty())
        { // json file
            util::json::render(current_reply.content, json_result);
            current_reply.headers.emplace_back("Content-Type", "application/json; charset=UTF-8");
            current_reply.headers.emplace_back("Content-Disposition",
                                               "inline; filename=\"response.json\"");
        }
        else
        { // jsonp
            util::json::render(current_reply.content, json_result);
            current_reply.headers.emplace_back("Content-Type", "text/javascript; charset=UTF-8");
            current_reply.headers.emplace_back("Content-Disposition",
                                               "inline; filename=\"response.js\"");
        }
        current_reply.headers.emplace_back("Content-Length",
                                           std::to_string(current_reply.content.size()));
        if (!route_parameters.jsonp_parameter.empty())
        { // append brace to jsonp response
            current_reply.content.push_back(')');
        }
    }
    catch (const std::exception &e)
    {
        current_reply = http::reply::stock_reply(http::reply::internal_server_error);
        util::SimpleLogger().Write(logWARNING) << "[server error] code: " << e.what()
                                               << ", uri: " << current_request.uri;
    }
}

void RequestHandler::RegisterRoutingMachine(OSRM *osrm) { routing_machine = osrm; }
}
}
