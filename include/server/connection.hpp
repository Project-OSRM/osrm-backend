#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "server/http/compression_type.hpp"
#include "server/http/reply.hpp"
#include "server/http/request.hpp"
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/config.hpp>
#include <boost/version.hpp>

#include <memory>
#include <optional>
#include <vector>

// workaround for incomplete std::shared_ptr compatibility in old boost versions
#if BOOST_VERSION < 105300 || defined BOOST_NO_CXX11_SMART_PTR

namespace boost
{
template <class T> const T *get_pointer(std::shared_ptr<T> const &p) { return p.get(); }

template <class T> T *get_pointer(std::shared_ptr<T> &p) { return p.get(); }
} // namespace boost

#endif

namespace osrm
{
namespace server
{

class RequestHandler;

/// Represents a single connection from a client.
class Connection : public std::enable_shared_from_this<Connection>
{
  public:
    explicit Connection(boost::asio::io_context &io_context, RequestHandler &handler);
    Connection(const Connection &) = delete;
    Connection &operator=(const Connection &) = delete;

    boost::asio::ip::tcp::socket &socket();

    /// Start the first asynchronous operation for the connection.
    void start();

  private:
    using RequestParser = boost::beast::http::request_parser<boost::beast::http::string_body>;
    void handle_read(const boost::system::error_code &e, std::size_t bytes_transferred);

    /// Handle completion of a write operation.
    void handle_write(const boost::system::error_code &e);

    /// Handle read timeout
    void handle_timeout(boost::system::error_code);

    void handle_shutdown();

    std::vector<char> compress_buffers(const std::vector<char> &uncompressed_data,
                                       const http::compression_type compression_type);

    void fill_request(const RequestParser::value_type &httpMessage, http::request &request);

    boost::asio::strand<boost::asio::io_context::executor_type> strand;
    boost::asio::ip::tcp::socket TCP_socket;
    boost::asio::deadline_timer timer;
    RequestHandler &request_handler;
    std::optional<RequestParser> http_request_parser;
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
} // namespace server
} // namespace osrm

#endif // CONNECTION_HPP
