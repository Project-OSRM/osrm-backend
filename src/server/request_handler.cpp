#include "server/request_handler.hpp"
#include "server/service_handler.hpp"

#include "server/api/url_parser.hpp"

#include "util/json_renderer.hpp"
#include "util/log.hpp"
#include "util/string_util.hpp"
#include "util/timing_util.hpp"

#include "engine/status.hpp"
#include "util/json_container.hpp"

#include <boost/iostreams/copy.hpp>

#include <ctime>

#include <algorithm>
#include <iomanip>
#include <string>
#include <thread>
#include <variant>

namespace osrm::server
{

namespace bhttp = boost::beast::http;

namespace
{
inline std::string HeaderOrEmpty(const Request &req, bhttp::field f)
{
    const auto it = req.find(f);
    if (it == req.end())
        return {};
    return std::string(it->value());
}

void SendResponse(ServiceHandler::ResultT &result,
                  Response &current_reply,
                  const bhttp::status status)
{

    current_reply.result(status);
    current_reply.set("Access-Control-Allow-Origin", "*");
    current_reply.set("Access-Control-Allow-Methods", "GET");
    current_reply.set("Access-Control-Allow-Headers", "X-Requested-With, Content-Type");
    if (std::holds_alternative<util::json::Object>(result))
    {
        current_reply.set(bhttp::field::content_type, "application/json; charset=UTF-8");
        current_reply.set(bhttp::field::content_disposition, "inline; filename=\"response.json\"");

        util::json::render(current_reply.body(), std::get<util::json::Object>(result));
    }
    else if (std::holds_alternative<flatbuffers::FlatBufferBuilder>(result))
    {
        auto &buffer = std::get<flatbuffers::FlatBufferBuilder>(result);
        current_reply.body().resize(buffer.GetSize());
        std::copy(buffer.GetBufferPointer(),
                  buffer.GetBufferPointer() + buffer.GetSize(),
                  current_reply.body().begin());

        current_reply.set(bhttp::field::content_type,
                          "application/x-flatbuffers;schema=osrm.engine.api.fbresult");
    }
    else
    {
        BOOST_ASSERT(std::holds_alternative<std::string>(result));
        current_reply.body().resize(std::get<std::string>(result).size());
        std::copy(std::get<std::string>(result).cbegin(),
                  std::get<std::string>(result).cend(),
                  current_reply.body().begin());

        current_reply.set(bhttp::field::content_type, "application/x-protobuf");
    }
}
} // namespace

void RequestHandler::RegisterServiceHandler(
    std::unique_ptr<ServiceHandlerInterface> service_handler_)
{
    service_handler = std::move(service_handler_);
}

void RequestHandler::HandleRequest(const Request &current_request,
                                   Response &current_reply,
                                   const boost::asio::ip::address &remote_address)
{
    // Defensive reset: Connection also resets before calling us.
    current_reply = {};

    if (!service_handler)
    {
        SetInternalServerError(current_reply);
        util::Log(logWARNING) << "No service handler registered." << std::endl;
        return;
    }

    const auto tid = std::this_thread::get_id();

    // parse command
    try
    {
        TIMER_START(request_duration);
        std::string request_string;
        util::URIDecode(std::string(current_request.target()), request_string);

        util::Log(logDEBUG) << "[req][" << tid << "] " << request_string;

        auto api_iterator = request_string.begin();
        auto maybe_parsed_url = api::parseURL(api_iterator, request_string.end());
        ServiceHandler::ResultT result;
        bhttp::status response_status = bhttp::status::ok;

        // check if there was an error with the request
        if (maybe_parsed_url && api_iterator == request_string.end())
        {

            const engine::Status status =
                service_handler->RunQuery(*std::move(maybe_parsed_url), result);
            if (status != engine::Status::Ok)
            {
                // 4xx bad request return code
                response_status = bhttp::status::bad_request;
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

            response_status = bhttp::status::bad_request;
            result = util::json::Object();
            auto &json_result = std::get<util::json::Object>(result);
            json_result.values["code"] = "InvalidUrl";
            json_result.values["message"] = "URL string malformed close to position " +
                                            std::to_string(position) + ": \"" + context + "\"";
        }

        SendResponse(result, current_reply, response_status);

        if (!std::getenv("DISABLE_ACCESS_LOGGING"))
        {
            TIMER_STOP(request_duration);

            std::time_t t = std::time(nullptr);
            const auto referrer = HeaderOrEmpty(current_request, bhttp::field::referer);
            const auto agent = HeaderOrEmpty(current_request, bhttp::field::user_agent);
            util::Log() << std::put_time(std::localtime(&t), "%d-%m-%Y %H:%M:%S") << " "
                        << TIMER_MSEC(request_duration) << "ms " << remote_address.to_string()
                        << " " << (referrer.empty() ? "-" : referrer) << " "
                        << (agent.empty() ? "-" : agent) << " " << current_reply.result_int()
                        << " " //
                        << request_string;
        }
    }
    catch (const util::DisabledDatasetException &e)
    {
        ServiceHandler::ResultT result = util::json::Object();
        auto &json_result = std::get<util::json::Object>(result);
        json_result.values["code"] = "DisabledDataset";
        json_result.values["message"] = e.what();
        SendResponse(result, current_reply, bhttp::status::bad_request);

        util::Log(logWARNING) << "[disabled dataset error][" << tid << "] code: DisabledDataset_"
                              << e.Dataset() << ", uri: " << current_request.target();
    }
    catch (const std::exception &e)
    {
        SetInternalServerError(current_reply);
        util::Log(logWARNING) << "[server error][" << tid << "] code: " << e.what()
                              << ", uri: " << current_request.target();
    }
}
} // namespace osrm::server
