#ifndef SERVER_HPP
#define SERVER_HPP

#include "server/connection.hpp"
#include "server/request_handler.hpp"
#include "server/service_handler.hpp"

#include "util/log.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>

#include <zlib.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace osrm::server
{

class Server : public std::enable_shared_from_this<Server>
{
  public:
    // Note: returns a shared instead of a unique ptr as it is captured in a lambda somewhere else
    static std::shared_ptr<Server> CreateServer(std::string &ip_address,
                                                int ip_port,
                                                unsigned requested_num_threads,
                                                short keepalive_timeout,
                                                unsigned max_header_size)
    {
        util::Log() << "HTTP/1.1 server using Boost.Beast, compression by zlib " << zlibVersion();
        const unsigned hardware_threads = std::max(1u, std::thread::hardware_concurrency());
        const unsigned real_num_threads = std::min(hardware_threads, requested_num_threads);
        return std::make_shared<Server>(
            ip_address, ip_port, real_num_threads, keepalive_timeout, max_header_size);
    }

    explicit Server(const std::string &address,
                    const int port,
                    const unsigned thread_pool_size,
                    const short keepalive_timeout,
                    const unsigned max_header_size)
        : thread_pool_size(thread_pool_size), keepalive_timeout(keepalive_timeout),
          max_header_size(max_header_size), io_context(thread_pool_size),
          acceptor(boost::asio::make_strand(io_context))
    {
        boost::beast::error_code ec;

        // Create endpoint
        boost::asio::ip::tcp::resolver resolver(io_context);
        auto const results = resolver.resolve(address, std::to_string(port));
        if (results.empty())
        {
            throw std::runtime_error("Failed to resolve address: " + address + ":" +
                                     std::to_string(port));
        }

        boost::asio::ip::tcp::endpoint endpoint = *results.begin();

        // Open the acceptor
        (void)acceptor.open(endpoint.protocol(), ec);
        if (ec)
        {
            throw std::runtime_error("Failed to open acceptor: " + ec.message());
        }

        // Allow address reuse
        (void)acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec)
        {
            util::Log(logWARNING) << "Failed to set reuse_address: " << ec.message();
        }

#ifdef SO_REUSEPORT
        // Set SO_REUSEPORT if available (Linux)
        const int option = 1;
        if (::setsockopt(
                acceptor.native_handle(), SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option)) < 0)
        {
            util::Log(logWARNING) << "Failed to set SO_REUSEPORT";
        }
#endif

        // Bind to the address
        (void)acceptor.bind(endpoint, ec);
        if (ec)
        {
            throw std::runtime_error("Failed to bind to address: " + ec.message());
        }

        // Start listening
        (void)acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec)
        {
            throw std::runtime_error("Failed to listen: " + ec.message());
        }

        util::Log() << "Listening on: " << acceptor.local_endpoint();
    }

    void Run()
    {
        // Start accepting connections
        DoAccept();

        // Create and run threads for the io_context
        std::vector<std::thread> threads;
        threads.reserve(thread_pool_size);
        for (unsigned i = 0; i < thread_pool_size; ++i)
        {
            threads.emplace_back([this]() { io_context.run(); });
        }

        // Wait for all threads to complete
        for (auto &thread : threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

    void Stop()
    {
        auto stop_promise = std::make_shared<std::promise<void>>();
        auto stop_future = stop_promise->get_future();

        // Posting the close to the acceptors strand ensures
        // we do not have a race condition with async_accept.
        boost::asio::post(acceptor.get_executor(),
                          [self = shared_from_this(), stop_promise]()
                          {
                              boost::beast::error_code ec;
                              (void)self->acceptor.close(ec);
                              if (ec)
                              {
                                  util::Log(logDEBUG) << "Error closing acceptor: " << ec.message();
                              }
                              // Stop the io_context
                              self->io_context.stop();
                              stop_promise->set_value();
                          });

        // The above function is async, this simply waits until it succeeded
        stop_future.wait();
    }

    void RegisterServiceHandler(std::unique_ptr<ServiceHandlerInterface> service_handler_)
    {
        request_handler.RegisterServiceHandler(std::move(service_handler_));
    }

  private:
    void DoAccept()
    {
        // The new connection gets its own strand
        acceptor.async_accept(boost::asio::make_strand(io_context),
                              [weak_self = std::weak_ptr<Server>(shared_from_this())](
                                  boost::beast::error_code ec, boost::asio::ip::tcp::socket socket)
                              {
                                  if (auto self = weak_self.lock())
                                  {
                                      self->OnAccept(ec, std::move(socket));
                                  }
                              });
    }

    void OnAccept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket)
    {
        if (!ec)
        {
            // Create the connection and start it
            auto connection = std::make_shared<Connection>(
                std::move(socket), request_handler, max_header_size, keepalive_timeout);

            connection->start();
        }
        else if (ec != boost::asio::error::operation_aborted)
        {
            util::Log(logERROR) << "Accept error: " << ec.message();
        }

        // Accept another connection
        if (acceptor.is_open())
        {
            DoAccept();
        }
    }

    RequestHandler request_handler;
    unsigned thread_pool_size;
    short keepalive_timeout;
    unsigned max_header_size;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor;
};

} // namespace osrm::server

#endif // SERVER_HPP
