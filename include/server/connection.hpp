#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "server/http/compression_type.hpp"

#include <boost/asio.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace osrm::server
{

class RequestHandler;

/// Represents a single HTTP connection using Boost.Beast
class Connection : public std::enable_shared_from_this<Connection>
{
  public:
    explicit Connection(boost::asio::ip::tcp::socket socket,
                        RequestHandler &handler,
                        short keepalive_timeout);

    Connection(const Connection &) = delete;
    Connection &operator=(const Connection &) = delete;

    /// Start the first asynchronous operation for the connection.
    void start();

  private:
    void handle_read();
    void process_request();
    void handle_write();
    void handle_close();

    http::compression_type determine_compression();
    std::vector<char> compress_buffers(const std::vector<char> &uncompressed_data,
                                       const http::compression_type compression_type);
    bool should_keep_alive() const;

    // Beast TCP stream with built-in timeout support
    boost::beast::tcp_stream stream_;

    boost::beast::flat_buffer message_buffer_;

    // The request message
    boost::beast::http::request<boost::beast::http::string_body> request_;

    // The response message - using vector_body for binary data support
    boost::beast::http::response<boost::beast::http::vector_body<char>> response_;

    // Reference to the existing request handler
    RequestHandler &request_handler_;

    // Keep-alive configuration
    short keepalive_timeout_;
    short keepalive_max_requests_ = 512;
    short processed_requests_ = 0;
};

} // namespace osrm::server

#endif // CONNECTION_HPP
