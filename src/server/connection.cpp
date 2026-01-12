#include "server/connection.hpp"
#include "server/request_handler.hpp"
#include "util/format.hpp"
#include "util/log.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <chrono>
#include <sstream>

namespace osrm::server
{

namespace beast = boost::beast;
namespace http_proto = beast::http;
using tcp = boost::asio::ip::tcp;

Connection::Connection(tcp::socket socket, RequestHandler &handler, short keepalive_timeout)
    : stream_(std::move(socket)), request_handler_(handler), keepalive_timeout_(keepalive_timeout)
{
    // Set initial timeout for the stream
    stream_.expires_after(std::chrono::seconds(keepalive_timeout_));
}

void Connection::start() { handle_read(); }

void Connection::handle_read()
{
    // Make sure we're still within the request limit for keep-alive
    if (processed_requests_ >= keepalive_max_requests_)
    {
        handle_close();
        return;
    }

    // Clear the request for a new read
    beast_request_ = {};

    // Set the timeout for reading
    stream_.expires_after(std::chrono::seconds(keepalive_timeout_));

    // Read a request
    auto self = shared_from_this();
    http_proto::async_read(stream_,
                           buffer_,
                           beast_request_,
                           [self](beast::error_code ec, std::size_t)
                           {
                               if (!ec)
                               {
                                   self->process_request();
                               }
                                else if (ec == http_proto::error::end_of_stream)
                                {
                                    // Remote closed the connection.
                                    self->handle_close();
                                }
                               else if (ec != http_proto::error::end_of_stream)
                               {
                                   util::Log(logDEBUG) << "Connection read error: " << ec.message();
                               }
                               // For end_of_stream, just let the connection close naturally
                           });
}

void Connection::process_request()
{
    // Clear previous response
    beast_response_ = {};
    beast_response_.version(beast_request_.version());

    // Remote endpoint (used for access logging)
    boost::asio::ip::address remote_address;
    beast::error_code endpoint_ec;
    const auto endpoint = stream_.socket().remote_endpoint(endpoint_ec);
    if (!endpoint_ec)
    {
        remote_address = endpoint.address();
    }

    // Process request using the handler (now Beast-native)
    try
    {
        request_handler_.HandleRequest(beast_request_, beast_response_, remote_address);
    }
    catch (const std::exception &e)
    {
        util::Log(logERROR) << "Request processing error: " << e.what();
        static constexpr char body[] =
            "{\"code\": \"InternalError\",\"message\":\"Internal Server Error\"}";
        beast_response_.result(http_proto::status::internal_server_error);
        beast_response_.set("Access-Control-Allow-Origin", "*");
        beast_response_.set("Access-Control-Allow-Methods", "GET");
        beast_response_.set("Access-Control-Allow-Headers", "X-Requested-With, Content-Type");
        beast_response_.set(http_proto::field::content_type, "application/json; charset=UTF-8");
        beast_response_.body().assign(body, body + (sizeof(body) - 1));
    }

    // Handle keep-alive
    const bool keep_alive = should_keep_alive();
    beast_response_.keep_alive(keep_alive);

    if (keep_alive)
    {
        beast_response_.set(http_proto::field::connection, "keep-alive");
        beast_response_.set("Keep-Alive",
                            util::compat::format("timeout={}, max={}",
                                                 keepalive_timeout_,
                                                 keepalive_max_requests_ - processed_requests_));
    }
    else
    {
        beast_response_.set(http_proto::field::connection, "close");
    }

    // Apply compression if requested
    const auto compression = determine_compression();
    if (compression != http::no_compression && !beast_response_.body().empty())
    {
        auto compressed = compress_buffers(beast_response_.body(), compression);
        beast_response_.body() = std::move(compressed);

        if (compression == http::gzip_rfc1952)
        {
            beast_response_.set(http_proto::field::content_encoding, "gzip");
        }
        else if (compression == http::deflate_rfc1951)
        {
            beast_response_.set(http_proto::field::content_encoding, "deflate");
        }

        // Help caches do the right thing
        beast_response_.set(http_proto::field::vary, "Accept-Encoding");
    }

    // Beast sets Content-Length based on body
    beast_response_.prepare_payload();

    // Increment processed request counter
    ++processed_requests_;

    // Write the response
    handle_write();
}

void Connection::handle_write()
{
    auto self = shared_from_this();

    // Set timeout for writing
    stream_.expires_after(std::chrono::seconds(keepalive_timeout_));

    // Send the response
    http_proto::async_write(stream_,
                            beast_response_,
                            [self](beast::error_code ec, std::size_t)
                            {
                                if (!ec)
                                {
                                    if (self->should_keep_alive())
                                    {
                                        // Read another request
                                        self->handle_read();
                                    }
                                    else
                                    {
                                        // Gracefully close the connection
                                        self->handle_close();
                                    }
                                }
                                else
                                {
                                    util::Log(logDEBUG)
                                        << "Connection write error: " << ec.message();
                                }
                            });
}

void Connection::handle_close()
{
    // Send a TCP shutdown
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

    // Don't worry about shutdown errors
    if (ec && ec != beast::errc::not_connected)
    {
        util::Log(logDEBUG) << "Connection shutdown error: " << ec.message();
    }
}

http::compression_type Connection::determine_compression()
{
    // Check Accept-Encoding header
    auto it = beast_request_.find(http_proto::field::accept_encoding);
    if (it == beast_request_.end())
    {
        return http::no_compression;
    }

    std::string accept_encoding(it->value());

    // Prefer gzip over deflate (following HTTP best practices)
    if (accept_encoding.find("gzip") != std::string::npos)
    {
        return http::gzip_rfc1952;
    }
    else if (accept_encoding.find("deflate") != std::string::npos)
    {
        return http::deflate_rfc1951;
    }

    return http::no_compression;
}

bool Connection::should_keep_alive() const
{
    // Check if client wants keep-alive (Beast handles HTTP/1.1 defaults correctly)
    if (!beast_request_.keep_alive())
    {
        return false;
    }

    // Check if we've hit the request limit
    if (processed_requests_ >= keepalive_max_requests_)
    {
        return false;
    }

    return true;
}

std::vector<char> Connection::compress_buffers(const std::vector<char> &uncompressed_data,
                                               const http::compression_type compression_type)
{
    namespace bio = boost::iostreams;

    std::vector<char> compressed_data;
    bio::filtering_ostream compressor;

    if (compression_type == http::gzip_rfc1952)
    {
        bio::gzip_params params;
        params.level = bio::zlib::best_speed;
        compressor.push(bio::gzip_compressor(params));
    }
    else if (compression_type == http::deflate_rfc1951)
    {
        bio::gzip_params params;
        params.level = bio::zlib::best_speed;
        params.noheader = true; // deflate = gzip without header
        compressor.push(bio::gzip_compressor(params));
    }
    else
    {
        return uncompressed_data; // No compression
    }

    compressor.push(bio::back_inserter(compressed_data));
    compressor.write(uncompressed_data.data(), uncompressed_data.size());
    bio::close(compressor);

    return compressed_data;
}
} // namespace osrm::server
