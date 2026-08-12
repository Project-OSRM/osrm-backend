#include "server/request_handler.hpp"

#include <boost/assert.hpp>

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
#include <cctype>
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

// True if the Content-Type header denotes JSON (optionally with parameters such as
// "; charset=utf-8"). Comparison is case-insensitive on the media type.
bool IsJsonContentType(std::string content_type)
{
    std::transform(content_type.begin(),
                   content_type.end(),
                   content_type.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return content_type.rfind("application/json", 0) == 0;
}

// Renders a request body as compact, single-line text for the access log so the request can be
// replayed from the log later. Insignificant whitespace between tokens is dropped and any raw
// newline is neutralised, so the log line always stays on one line; whitespace inside JSON
// string literals is preserved (a raw newline there means the body was not valid JSON anyway).
//
// This is deliberately a single non-recursive pass rather than a RapidJSON parse-and-reserialise:
// both Document::Parse (default flags) and Value::Accept recurse once per nesting level, so a
// hostile body such as "[[[[..." would overflow the stack here -- and this runs on every POST,
// before the request is even dispatched.
std::string CompactJsonForLog(const std::string &body)
{
    std::string out;
    out.reserve(body.size());

    bool in_string = false;
    bool escaped = false;
    for (const char c : body)
    {
        const bool is_newline = c == '\n' || c == '\r';
        if (in_string)
        {
            // Keep the string's contents verbatim, but never emit an actual newline.
            out.push_back(is_newline ? ' ' : c);
            if (escaped)
                escaped = false;
            else if (c == '\\')
                escaped = true;
            else if (c == '"')
                in_string = false;
        }
        else if (c == '"')
        {
            in_string = true;
            out.push_back(c);
        }
        else if (c != ' ' && c != '\t' && !is_newline)
        {
            out.push_back(c);
        }
        // else: insignificant whitespace between tokens, dropped.
    }
    return out;
}

// Builds the JSON error returned when the request path cannot be parsed. `iter` points at
// the position where parsing failed within `request_string`.
util::json::Object MakeInvalidUrlError(std::string &request_string,
                                       const std::string::iterator iter)
{
    const auto position = std::distance(request_string.begin(), iter);
    BOOST_ASSERT(position >= 0);
    const auto context_begin = request_string.begin() + ((position < 3) ? 0 : (position - 3UL));
    BOOST_ASSERT(context_begin >= request_string.begin());
    const auto context_end =
        request_string.begin() + std::min<std::size_t>(position + 3UL, request_string.size());
    BOOST_ASSERT(context_end <= request_string.end());
    std::string context(context_begin, context_end);

    util::json::Object json_result;
    json_result.values["code"] = "InvalidUrl";
    json_result.values["message"] = "URL string malformed close to position " +
                                    std::to_string(position) + ": \"" + context + "\"";
    return json_result;
}

void SendResponse(ServiceHandler::ResultT &result,
                  Response &current_reply,
                  const bhttp::status status)
{

    current_reply.result(status);
    SetCorsHeaders(current_reply);
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
{ service_handler = std::move(service_handler_); }

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

    const auto method = current_request.method();

    // CORS preflight: answer with the allowed methods/headers and no body.
    if (method == bhttp::verb::options)
    {
        current_reply.result(bhttp::status::no_content);
        SetCorsHeaders(current_reply);
        return;
    }

    // Only GET (URL query) and POST (JSON body) carry OSRM queries. HEAD is treated like GET.
    if (method != bhttp::verb::get && method != bhttp::verb::head && method != bhttp::verb::post)
    {
        ServiceHandler::ResultT result = util::json::Object();
        auto &json_result = std::get<util::json::Object>(result);
        json_result.values["code"] = "InvalidMethod";
        json_result.values["message"] = "Method not allowed. Use GET, HEAD, POST, or OPTIONS.";
        SendResponse(result, current_reply, bhttp::status::method_not_allowed);
        current_reply.set(bhttp::field::allow, "GET, HEAD, POST, OPTIONS");
        return;
    }

    const bool is_post = method == bhttp::verb::post;

    const auto tid = std::this_thread::get_id();

    // parse command
    try
    {
        TIMER_START(request_duration);
        std::string request_string;
        util::URIDecode(std::string(current_request.target()), request_string);

        // Echo every incoming request as a single line at debug level (as on the GET-only code
        // path before POST support). GET carries its full query in the URL; POST additionally
        // gets its JSON body appended (compacted to one line) so both request types are echoed
        // identically and can be replayed from the log alone. The access log below repeats this
        // at info level, so keeping the echo at debug avoids logging every request body twice in
        // a normal deployment.
        util::Log(logDEBUG) << "[req][" << tid << "] " << request_string
                            << (is_post && !util::LogPolicy::GetInstance().IsMute()
                                    ? " " + CompactJsonForLog(current_request.body())
                                    : std::string());

        ServiceHandler::ResultT result;
        bhttp::status response_status = bhttp::status::ok;

        auto api_iterator = request_string.begin();

        if (is_post)
        {
            // POST: the URL only carries "/{service}/v{version}/{profile}"; coordinates and
            // options are supplied as a JSON body.
            const auto content_type = HeaderOrEmpty(current_request, bhttp::field::content_type);
            auto maybe_parsed_url = api::parseURLPrefix(api_iterator, request_string.end());

            if (!IsJsonContentType(content_type))
            {
                response_status = bhttp::status::unsupported_media_type;
                result = util::json::Object();
                auto &json_result = std::get<util::json::Object>(result);
                json_result.values["code"] = "InvalidContentType";
                json_result.values["message"] =
                    "POST requests require a Content-Type of application/json.";
            }
            else if (maybe_parsed_url && api_iterator == request_string.end())
            {
                const engine::Status status = service_handler->RunQuery(
                    *std::move(maybe_parsed_url), current_request.body(), result);
                if (status != engine::Status::Ok)
                {
                    response_status = bhttp::status::bad_request;
                }
            }
            else
            {
                response_status = bhttp::status::bad_request;
                result = MakeInvalidUrlError(request_string, api_iterator);
            }
        }
        else
        {
            // GET/HEAD: the whole query lives in the URL.
            auto maybe_parsed_url = api::parseURL(api_iterator, request_string.end());
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
                response_status = bhttp::status::bad_request;
                result = MakeInvalidUrlError(request_string, api_iterator);
            }
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
                        << request_string
                        // POST: append the JSON body (compacted to one line) so the request
                        // can be replayed from the log alone.
                        << (is_post && !util::LogPolicy::GetInstance().IsMute()
                                ? " " + CompactJsonForLog(current_request.body())
                                : std::string());
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
