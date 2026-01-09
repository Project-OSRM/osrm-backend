#include "server/connection.hpp"
#include "server/request_handler.hpp"
#include "util/format.hpp"
#include "util/log.hpp"

#include <boost/algorithm/string/predicate.hpp>
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
    // Create OSRM request and reply objects
    http::request osrm_request;
    http::reply osrm_reply;

    // Adapt Beast request to OSRM format
    adapt_request(osrm_request);

    // Process using existing handler
    try
    {
        request_handler_.HandleRequest(osrm_request, osrm_reply);
    }
    catch (const std::exception &e)
    {
        util::Log(logERROR) << "Request processing error: " << e.what();
        osrm_reply = http::reply::stock_reply(http::reply::internal_server_error);
    }

    // Adapt OSRM reply to Beast response
    adapt_response(osrm_reply);

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

void Connection::adapt_request(http::request &osrm_request)
{
    // Extract URI from Beast request
    osrm_request.uri = std::string(beast_request_.target());

    // Extract headers we care about
    auto referrer_it = beast_request_.find(http_proto::field::referer);
    if (referrer_it != beast_request_.end())
    {
        osrm_request.referrer = std::string(referrer_it->value());
    }

    auto agent_it = beast_request_.find(http_proto::field::user_agent);
    if (agent_it != beast_request_.end())
    {
        osrm_request.agent = std::string(agent_it->value());
    }

    auto connection_it = beast_request_.find(http_proto::field::connection);
    if (connection_it != beast_request_.end())
    {
        osrm_request.connection = std::string(connection_it->value());
    }

    // Get remote endpoint
    beast::error_code ec;
    auto endpoint = stream_.socket().remote_endpoint(ec);
    if (!ec)
    {
        osrm_request.endpoint = endpoint.address();
    }
}

void Connection::adapt_response(const http::reply &osrm_reply)
{
    // Clear previous response
    beast_response_ = {};

    // Set status code
    beast_response_.result(static_cast<http_proto::status>(osrm_reply.status));

    // Set version
    beast_response_.version(beast_request_.version());

    // Check for compression
    auto compression = determine_compression();

    // Copy or compress the body
    if (compression != http::no_compression)
    {
        auto compressed = compress_buffers(osrm_reply.content, compression);
        beast_response_.body() = std::move(compressed);

        // Set compression header
        if (compression == http::gzip_rfc1952)
        {
            beast_response_.set(http_proto::field::content_encoding, "gzip");
        }
        else if (compression == http::deflate_rfc1951)
        {
            beast_response_.set(http_proto::field::content_encoding, "deflate");
        }
    }
    else
    {
        beast_response_.body() = osrm_reply.content;
    }

    // Copy headers from OSRM reply
    for (const auto &header : osrm_reply.headers)
    {
        // Skip Content-Length as Beast will set it
        if (!boost::iequals(header.name, "Content-Length"))
        {
            beast_response_.set(header.name, header.value);
        }
    }

    // Handle keep-alive
    bool keep_alive = should_keep_alive();
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

    // Beast automatically sets Content-Length
    beast_response_.prepare_payload();

    // Increment processed request counter
    ++processed_requests_;
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
