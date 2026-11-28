#include "server/server.hpp"

#include "server/api/parsed_url.hpp"
#include "server/service_handler.hpp"

#include "util/json_container.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>

BOOST_AUTO_TEST_SUITE(server_graceful_shutdown)

using namespace osrm;
using namespace osrm::server;

namespace bhttp = boost::beast::http;
using tcp = boost::asio::ip::tcp;

namespace
{

constexpr auto WAIT_TIMEOUT = std::chrono::seconds(10);

// Answers every query once the test releases it, so a request can be kept in flight.
struct BlockingServiceHandler final : ServiceHandlerInterface
{
    explicit BlockingServiceHandler(std::shared_future<void> release_) : release(release_) {}

    engine::Status RunQuery(api::ParsedURL, engine::api::ResultT &result) override
    {
        if (!entered_once.exchange(true))
        {
            entered.set_value();
        }
        // Bounded, so that a failing test doesn't hang.
        release.wait_for(WAIT_TIMEOUT);
        result = util::json::Object();
        std::get<util::json::Object>(result).values["code"] = "Ok";
        return engine::Status::Ok;
    }

    engine::Status
    RunQuery(api::ParsedURL parsed_url, const std::string &, engine::api::ResultT &result) override
    { return RunQuery(std::move(parsed_url), result); }

    std::shared_future<void> release;
    std::atomic<bool> entered_once = false;
    std::promise<void> entered;
};

unsigned short freePort()
{
    boost::asio::io_context io_context;
    tcp::acceptor acceptor(io_context,
                           tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 0));
    return acceptor.local_endpoint().port();
}

struct RunningServer
{
    RunningServer(std::shared_future<void> release, short keepalive_timeout = 30) : port(freePort())
    {
        std::string address = "127.0.0.1";
        // Two threads, so that one is free to run Stop() while the other processes a request.
        server =
            std::make_shared<Server>(address, port, 2, keepalive_timeout, 8 * 1024, 1024 * 1024);
        auto service_handler = std::make_unique<BlockingServiceHandler>(release);
        handler = service_handler.get();
        server->RegisterServiceHandler(std::move(service_handler));
        run = std::async(std::launch::async, [server = server]() { server->Run(); });
    }

    ~RunningServer()
    {
        // Stops a server that a failed check left running. Once Run() has returned, nothing
        // would execute the handler that Stop() posts.
        if (run.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            server->Stop();
        }
    }

    boost::beast::tcp_stream connect()
    {
        boost::beast::tcp_stream stream(io_context);
        stream.connect(tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), port));
        return stream;
    }

    unsigned short port;
    std::shared_ptr<Server> server;
    BlockingServiceHandler *handler;
    std::future<void> run;
    boost::asio::io_context io_context;
};

void sendRequest(boost::beast::tcp_stream &stream)
{
    bhttp::request<bhttp::empty_body> request{bhttp::verb::get, "/route/v1/driving/1,2;3,4", 11};
    request.set(bhttp::field::host, "localhost");
    bhttp::write(stream, request);
}

// Reads a response, giving up after WAIT_TIMEOUT so that a failing test doesn't hang.
boost::beast::error_code readResponse(boost::beast::tcp_stream &stream,
                                      boost::beast::flat_buffer &buffer,
                                      bhttp::response<bhttp::string_body> &response)
{
    boost::beast::error_code result;
    stream.expires_after(WAIT_TIMEOUT);
    bhttp::async_read(stream,
                      buffer,
                      response,
                      [&result](boost::beast::error_code ec, std::size_t) { result = ec; });
    auto &io_context = static_cast<boost::asio::io_context &>(stream.get_executor().context());
    io_context.restart();
    io_context.run();
    return result;
}

bool isClosedByServer(boost::beast::tcp_stream &stream, boost::beast::flat_buffer &buffer)
{
    bhttp::response<bhttp::string_body> response;
    const auto ec = readResponse(stream, buffer, response);
    return ec == bhttp::error::end_of_stream || ec == boost::asio::error::eof ||
           ec == boost::asio::error::connection_reset;
}

bool refusesConnections(RunningServer &running)
{
    // Stop() is asynchronous, so allow it a moment to close the acceptor.
    const auto deadline = std::chrono::steady_clock::now() + WAIT_TIMEOUT;
    while (std::chrono::steady_clock::now() < deadline)
    {
        tcp::socket socket(running.io_context);
        boost::beast::error_code ec;
        (void)socket.connect(
            tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), running.port), ec);
        if (ec == boost::asio::error::connection_refused)
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

} // namespace

BOOST_AUTO_TEST_CASE(request_in_flight_is_answered_after_stop)
{
    std::promise<void> release;
    RunningServer running(release.get_future().share());

    auto stream = running.connect();
    boost::beast::flat_buffer buffer;
    sendRequest(stream);
    BOOST_REQUIRE(running.handler->entered.get_future().wait_for(WAIT_TIMEOUT) ==
                  std::future_status::ready);

    running.server->Stop();

    BOOST_CHECK(refusesConnections(running));
    // Run() keeps going while the request is still being processed.
    BOOST_CHECK(running.run.wait_for(std::chrono::milliseconds(200)) ==
                std::future_status::timeout);

    release.set_value();

    bhttp::response<bhttp::string_body> response;
    BOOST_REQUIRE(!readResponse(stream, buffer, response));
    BOOST_CHECK(response.result() == bhttp::status::ok);
    BOOST_CHECK_EQUAL(response[bhttp::field::connection], "close");
    BOOST_CHECK(response.body().find("\"code\":\"Ok\"") != std::string::npos);
    BOOST_CHECK(isClosedByServer(stream, buffer));

    BOOST_CHECK(running.run.wait_for(WAIT_TIMEOUT) == std::future_status::ready);
}

BOOST_AUTO_TEST_CASE(idle_keep_alive_connection_is_closed_on_stop)
{
    std::promise<void> release;
    release.set_value();
    // A keep-alive timeout far above the wait below shows that Stop() doesn't wait for it.
    RunningServer running(release.get_future().share(), 60);

    auto stream = running.connect();
    boost::beast::flat_buffer buffer;
    sendRequest(stream);
    bhttp::response<bhttp::string_body> response;
    BOOST_REQUIRE(!readResponse(stream, buffer, response));
    BOOST_CHECK(response.result() == bhttp::status::ok);
    BOOST_CHECK_EQUAL(response[bhttp::field::connection], "keep-alive");

    running.server->Stop();

    BOOST_CHECK(isClosedByServer(stream, buffer));
    BOOST_CHECK(running.run.wait_for(WAIT_TIMEOUT) == std::future_status::ready);
}

BOOST_AUTO_TEST_CASE(stop_without_connections_ends_run)
{
    std::promise<void> release;
    release.set_value();
    RunningServer running(release.get_future().share());

    running.server->Stop();

    BOOST_CHECK(running.run.wait_for(WAIT_TIMEOUT) == std::future_status::ready);
    BOOST_CHECK(refusesConnections(running));
}

BOOST_AUTO_TEST_SUITE_END()
