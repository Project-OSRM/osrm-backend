#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "server/http/compression_type.hpp"
#include "server/http/reply.hpp"
#include "server/http/request.hpp"
#include "server/request_parser.hpp"

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/config.hpp>
#include <boost/version.hpp>

#include <memory>
#include <vector>

namespace osrm::server
{

class RequestHandler;

/// Represents a single connection from a client.
class Connection : public std::enable_shared_from_this<Connection>
{
  public:
    explicit Connection(boost::asio::io_context &io_context,
                        RequestHandler &handler,
                        short keepalive_timeout);
    Connection(const Connection &) = delete;
    Connection &operator=(const Connection &) = delete;

    boost::asio::ip::tcp::socket &socket();

    /// Start the first asynchronous operation for the connection.
    void start();

  private:
    void handle_read(const boost::system::error_code &e, std::size_t bytes_transferred);

    /// Handle completion of a write operation.
    void handle_write(const boost::system::error_code &e);

    /// Handle read timeout
    void handle_timeout(boost::system::error_code);

    void handle_shutdown();

    std::vector<char> compress_buffers(const std::vector<char> &uncompressed_data,
                                       const http::compression_type compression_type);

    boost::asio::strand<boost::asio::io_context::executor_type> strand;
    boost::asio::ip::tcp::socket TCP_socket;
    boost::asio::deadline_timer timer;
    RequestHandler &request_handler;
    RequestParser request_parser;
    boost::array<char, 8192> incoming_data_buffer;
    http::request current_request;
    http::reply current_reply;
    std::vector<char> compressed_output;
    // Header compression_header;
    std::vector<boost::asio::const_buffer> output_buffer;
    // Keep alive support
    bool keep_alive = false;
    short processed_requests = 512;
    short keepalive_timeout = 5; // In seconds
};
} // namespace osrm::server

#endif // CONNECTION_HPP
