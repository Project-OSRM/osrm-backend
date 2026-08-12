#include "server/request_handler.hpp"

#include "server/api/parsed_url.hpp"
#include "server/service_handler.hpp"

#include "util/json_container.hpp"
#include "util/log.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/beast/http.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

BOOST_AUTO_TEST_SUITE(server_request_handler)

using namespace osrm;
using namespace osrm::server;

namespace bhttp = boost::beast::http;

namespace
{

// Records what the request handler dispatched and answers with a canned result, so the tests
// can exercise the HTTP layer without a routing engine behind it.
struct StubServiceHandler final : ServiceHandlerInterface
{
    engine::Status RunQuery(api::ParsedURL parsed_url, engine::api::ResultT &result) override
    {
        get_queries.push_back(std::move(parsed_url));
        return Answer(result);
    }

    engine::Status RunQuery(api::ParsedURL parsed_url,
                            const std::string &json_body,
                            engine::api::ResultT &result) override
    {
        post_queries.push_back(std::move(parsed_url));
        post_bodies.push_back(json_body);
        return Answer(result);
    }

    engine::Status Answer(engine::api::ResultT &result)
    {
        result = util::json::Object();
        std::get<util::json::Object>(result).values["code"] = "Ok";
        return status;
    }

    std::size_t QueryCount() const { return get_queries.size() + post_queries.size(); }

    std::vector<api::ParsedURL> get_queries;
    std::vector<api::ParsedURL> post_queries;
    std::vector<std::string> post_bodies;
    engine::Status status = engine::Status::Ok;
};

Request makeRequest(bhttp::verb method,
                    const std::string &target,
                    const std::string &content_type = "",
                    const std::string &body = "")
{
    Request request;
    request.method(method);
    request.target(target);
    if (!content_type.empty())
        request.set(bhttp::field::content_type, content_type);
    request.body() = body;
    return request;
}

Response handle(RequestHandler &handler, const Request &request)
{
    Response reply;
    handler.HandleRequest(request, reply, boost::asio::ip::make_address("127.0.0.1"));
    return reply;
}

std::string bodyOf(const Response &reply) { return {reply.body().begin(), reply.body().end()}; }

} // namespace

BOOST_AUTO_TEST_CASE(without_a_service_handler_every_request_is_an_internal_error)
{
    RequestHandler handler;

    auto reply = handle(handler, makeRequest(bhttp::verb::get, "/route/v1/driving/1,2;3,4"));

    BOOST_CHECK(reply.result() == bhttp::status::internal_server_error);
    // Even the error response has to carry the CORS headers, or browsers cannot read it.
    BOOST_CHECK_EQUAL(reply["Access-Control-Allow-Origin"], "*");
    BOOST_CHECK(bodyOf(reply).find("InternalError") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(cors_preflight_is_answered_without_a_body)
{
    RequestHandler handler;
    auto service_handler = std::make_unique<StubServiceHandler>();
    auto *stub = service_handler.get();
    handler.RegisterServiceHandler(std::move(service_handler));

    auto reply = handle(handler, makeRequest(bhttp::verb::options, "/route/v1/driving"));

    BOOST_CHECK(reply.result() == bhttp::status::no_content);
    BOOST_CHECK_EQUAL(reply["Access-Control-Allow-Methods"], "GET, HEAD, POST, OPTIONS");
    BOOST_CHECK(reply.body().empty());
    // A preflight must not reach the routing engine.
    BOOST_CHECK_EQUAL(stub->QueryCount(), 0u);
}

BOOST_AUTO_TEST_CASE(methods_other_than_get_head_post_are_rejected)
{
    RequestHandler handler;
    auto service_handler = std::make_unique<StubServiceHandler>();
    auto *stub = service_handler.get();
    handler.RegisterServiceHandler(std::move(service_handler));

    for (const auto method : {bhttp::verb::put, bhttp::verb::delete_, bhttp::verb::patch})
    {
        auto reply = handle(handler, makeRequest(method, "/route/v1/driving/1,2;3,4"));

        BOOST_CHECK(reply.result() == bhttp::status::method_not_allowed);
        BOOST_CHECK_EQUAL(reply[bhttp::field::allow], "GET, HEAD, POST, OPTIONS");
        BOOST_CHECK(bodyOf(reply).find("InvalidMethod") != std::string::npos);
    }
    BOOST_CHECK_EQUAL(stub->QueryCount(), 0u);
}

BOOST_AUTO_TEST_CASE(head_is_handled_like_get)
{
    RequestHandler handler;
    auto service_handler = std::make_unique<StubServiceHandler>();
    auto *stub = service_handler.get();
    handler.RegisterServiceHandler(std::move(service_handler));

    auto reply = handle(handler, makeRequest(bhttp::verb::head, "/route/v1/driving/1,2;3,4"));

    BOOST_CHECK(reply.result() == bhttp::status::ok);
    BOOST_REQUIRE_EQUAL(stub->get_queries.size(), 1u);
    BOOST_CHECK_EQUAL(stub->get_queries[0].service, "route");
    BOOST_CHECK_EQUAL(stub->get_queries[0].query, "1,2;3,4");
}

BOOST_AUTO_TEST_CASE(post_requires_a_json_content_type)
{
    RequestHandler handler;
    auto service_handler = std::make_unique<StubServiceHandler>();
    auto *stub = service_handler.get();
    handler.RegisterServiceHandler(std::move(service_handler));

    const std::string body = "{\"coordinates\": [[1,2],[3,4]]}";

    auto rejected =
        handle(handler, makeRequest(bhttp::verb::post, "/route/v1/driving", "text/plain", body));
    BOOST_CHECK(rejected.result() == bhttp::status::unsupported_media_type);
    BOOST_CHECK(bodyOf(rejected).find("InvalidContentType") != std::string::npos);

    // A missing Content-Type is rejected the same way.
    auto missing = handle(handler, makeRequest(bhttp::verb::post, "/route/v1/driving", "", body));
    BOOST_CHECK(missing.result() == bhttp::status::unsupported_media_type);

    BOOST_CHECK_EQUAL(stub->QueryCount(), 0u);

    // Media type parameters are allowed, and the comparison is case-insensitive.
    auto accepted = handle(
        handler,
        makeRequest(
            bhttp::verb::post, "/route/v1/driving", "Application/JSON; charset=utf-8", body));
    BOOST_CHECK(accepted.result() == bhttp::status::ok);
    BOOST_REQUIRE_EQUAL(stub->post_queries.size(), 1u);
    BOOST_CHECK_EQUAL(stub->post_queries[0].service, "route");
    BOOST_CHECK_EQUAL(stub->post_queries[0].profile, "driving");
    BOOST_CHECK(stub->post_queries[0].query.empty());
    BOOST_CHECK_EQUAL(stub->post_bodies[0], body);
}

BOOST_AUTO_TEST_CASE(post_url_must_not_carry_a_query)
{
    RequestHandler handler;
    auto service_handler = std::make_unique<StubServiceHandler>();
    auto *stub = service_handler.get();
    handler.RegisterServiceHandler(std::move(service_handler));

    auto reply = handle(handler,
                        makeRequest(bhttp::verb::post,
                                    "/route/v1/driving/1,2;3,4",
                                    "application/json",
                                    "{\"coordinates\": [[1,2],[3,4]]}"));

    BOOST_CHECK(reply.result() == bhttp::status::bad_request);
    BOOST_CHECK(bodyOf(reply).find("InvalidUrl") != std::string::npos);
    BOOST_CHECK_EQUAL(stub->QueryCount(), 0u);
}

BOOST_AUTO_TEST_CASE(a_failing_query_maps_to_bad_request)
{
    RequestHandler handler;
    auto service_handler = std::make_unique<StubServiceHandler>();
    auto *stub = service_handler.get();
    stub->status = engine::Status::Error;
    handler.RegisterServiceHandler(std::move(service_handler));

    auto post = handle(handler,
                       makeRequest(bhttp::verb::post,
                                   "/route/v1/driving",
                                   "application/json",
                                   "{\"coordinates\": [[1,2],[3,4]]}"));
    BOOST_CHECK(post.result() == bhttp::status::bad_request);

    auto get = handle(handler, makeRequest(bhttp::verb::get, "/route/v1/driving/1,2;3,4"));
    BOOST_CHECK(get.result() == bhttp::status::bad_request);
}

BOOST_AUTO_TEST_CASE(post_bodies_are_logged_on_a_single_line)
{
    RequestHandler handler;
    handler.RegisterServiceHandler(std::make_unique<StubServiceHandler>());

    // The access log is only written when logging is enabled, so turn it on and capture it.
    auto &policy = util::LogPolicy::GetInstance();
    const bool was_mute = policy.IsMute();
    policy.Unmute();
    std::ostringstream captured;
    auto *previous = std::cout.rdbuf(captured.rdbuf());

    // A body that is not valid JSON still has to end up on one line so that the log stays
    // parseable line by line.
    handle(handler,
           makeRequest(
               bhttp::verb::post, "/route/v1/driving", "application/json", "not\njson\r\nhere"));

    std::cout.rdbuf(previous);
    if (was_mute)
        policy.Mute();

    const auto log = captured.str();
    BOOST_CHECK(log.find("not json  here") != std::string::npos);
    BOOST_CHECK(log.find("not\njson") == std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
