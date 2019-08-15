#include "server/request_handler.hpp"
#include "server/service_handler.hpp"

#include "server/api/url_parser.hpp"
#include "server/http/reply.hpp"
#include "server/http/request.hpp"

#include "util/json_renderer.hpp"
#include "util/log.hpp"
#include "util/string_util.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

#include "engine/status.hpp"
#include "osrm/osrm.hpp"
#include "util/json_container.hpp"

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

#include <ctime>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>

namespace osrm
{
namespace server
{

void RequestHandler::RegisterServiceHandler(
    std::unique_ptr<ServiceHandlerInterface> service_handler_)
{
    service_handler = std::move(service_handler_);
}

void RequestHandler::HandleRequest(const http::request &current_request, http::reply &current_reply)
{
    if (!service_handler)
    {
        current_reply = http::reply::stock_reply(http::reply::internal_server_error);
        util::Log(logWARNING) << "No service handler registered." << std::endl;
        return;
    }

    const auto tid = std::this_thread::get_id();

    // parse command
    try
    {
        TIMER_START(request_duration);
        std::string request_string;
        util::URIDecode(current_request.uri, request_string);

        util::Log(logDEBUG) << "[req][" << tid << "] " << request_string;

        auto api_iterator = request_string.begin();
        auto maybe_parsed_url = api::parseURL(api_iterator, request_string.end());
        ServiceHandler::ResultT result;

        // check if the was an error with the request
        if (maybe_parsed_url && api_iterator == request_string.end())
        {

            const engine::Status status =
                service_handler->RunQuery(*std::move(maybe_parsed_url), result);
            if (status != engine::Status::Ok)
            {
                // 4xx bad request return code
                current_reply.status = http::reply::bad_request;
            }
            else
            {
                BOOST_ASSERT(status == engine::Status::Ok);
            }
        }
        else
        {
            const auto position = std::distance(request_string.begin(), api_iterator);
            BOOST_ASSERT(position >= 0);
            const auto context_begin =
                request_string.begin() + ((position < 3) ? 0 : (position - 3UL));
            BOOST_ASSERT(context_begin >= request_string.begin());
            const auto context_end = request_string.begin() +
                                     std::min<std::size_t>(position + 3UL, request_string.size());
            BOOST_ASSERT(context_end <= request_string.end());
            std::string context(context_begin, context_end);

            current_reply.status = http::reply::bad_request;
            result = util::json::Object();
            auto &json_result = result.get<util::json::Object>();
            json_result.values["code"] = "InvalidUrl";
            json_result.values["message"] = "URL string malformed close to position " +
                                            std::to_string(position) + ": \"" + context + "\"";
        }

        current_reply.headers.emplace_back("Access-Control-Allow-Origin", "*");
        current_reply.headers.emplace_back("Access-Control-Allow-Methods", "GET");
        current_reply.headers.emplace_back("Access-Control-Allow-Headers",
                                           "X-Requested-With, Content-Type");
        if (result.is<util::json::Object>())
        {
            current_reply.headers.emplace_back("Content-Type", "application/json; charset=UTF-8");
            current_reply.headers.emplace_back("Content-Disposition",
                                               "inline; filename=\"response.json\"");

            util::json::render(current_reply.content, result.get<util::json::Object>());
        }
        else if (result.is<flatbuffers::FlatBufferBuilder>())
        {
            auto &buffer = result.get<flatbuffers::FlatBufferBuilder>();
            current_reply.content.resize(buffer.GetSize());
            std::copy(buffer.GetBufferPointer(),
                      buffer.GetBufferPointer() + buffer.GetSize(),
                      current_reply.content.begin());

            current_reply.headers.emplace_back(
                "Content-Type", "application/x-flatbuffers;schema=osrm.engine.api.fbresult");
        }
        else
        {
            BOOST_ASSERT(result.is<std::string>());
            current_reply.content.resize(result.get<std::string>().size());
            std::copy(result.get<std::string>().cbegin(),
                      result.get<std::string>().cend(),
                      current_reply.content.begin());

            current_reply.headers.emplace_back("Content-Type", "application/x-protobuf");
        }

        // set headers
        current_reply.headers.emplace_back("Content-Length",
                                           std::to_string(current_reply.content.size()));

        if (!std::getenv("DISABLE_ACCESS_LOGGING"))
        {
            // deactivated as GCC apparently does not implement that, not even in 4.9
            // std::time_t t = std::time(nullptr);
            // util::Log() << std::put_time(std::localtime(&t), "%m-%d-%Y
            // %H:%M:%S") <<
            //     " " << current_request.endpoint.to_string() << " " <<
            //     current_request.referrer << ( 0 == current_request.referrer.length() ? "- " :" ")
            //     <<
            //     current_request.agent << ( 0 == current_request.agent.length() ? "- " :" ") <<
            //     request;

            time_t ltime;
            struct tm *time_stamp;
            TIMER_STOP(request_duration);

            ltime = time(nullptr);
            time_stamp = localtime(&ltime);
            // log timestamp
            util::Log() << (time_stamp->tm_mday < 10 ? "0" : "") << time_stamp->tm_mday << "-"
                        << (time_stamp->tm_mon + 1 < 10 ? "0" : "") << (time_stamp->tm_mon + 1)
                        << "-" << 1900 + time_stamp->tm_year << " "
                        << (time_stamp->tm_hour < 10 ? "0" : "") << time_stamp->tm_hour << ":"
                        << (time_stamp->tm_min < 10 ? "0" : "") << time_stamp->tm_min << ":"
                        << (time_stamp->tm_sec < 10 ? "0" : "") << time_stamp->tm_sec << " "
                        << TIMER_MSEC(request_duration) << "ms "
                        << current_request.endpoint.to_string() << " " << current_request.referrer
                        << (0 == current_request.referrer.length() ? "- " : " ")
                        << current_request.agent
                        << (0 == current_request.agent.length() ? "- " : " ")
                        << current_reply.status << " " //
                        << request_string;
        }
    }
    catch (const std::exception &e)
    {
        current_reply = http::reply::stock_reply(http::reply::internal_server_error);
        util::Log(logWARNING) << "[server error][" << tid << "] code: " << e.what()
                              << ", uri: " << current_request.uri;
    }
}
}
}
