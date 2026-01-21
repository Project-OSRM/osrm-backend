#include "server/connection.hpp"
#include "server/request_handler.hpp"
#include "util/format.hpp"
#include "util/log.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <chrono>
#include <cstdint>

namespace osrm::server
{

static constexpr short KEEPALIVE_MAX_REQUESTS = 512;

namespace bhttp = boost::beast::http;
using tcp = boost::asio::ip::tcp;

Connection::Connection(tcp::socket socket,
                       RequestHandler &handler,
                       unsigned max_header_size,
                       short keepalive_timeout)
    : stream_(std::move(socket)), request_handler_(handler), max_header_size_(max_header_size),
      keepalive_timeout_(keepalive_timeout)
{
    stream_.expires_after(std::chrono::seconds(keepalive_timeout_));
}

void Connection::start() { handle_read(); }

void Connection::handle_read()
{
    if (processed_requests_ >= KEEPALIVE_MAX_REQUESTS)
    {
        handle_close();
        return;
    }

    request_ = {};
    parser_.emplace();
    // Note: The name is a bit of a misnomer, this includes the size of the GET request line.
    // Some people parse huge GET requests for table requests, we need to make this configurable.
    parser_->header_limit(max_header_size_);

    stream_.expires_after(std::chrono::seconds(keepalive_timeout_));

    auto self = shared_from_this();
    bhttp::async_read(stream_,
                      message_buffer_,
                      *parser_,
                      [self](boost::beast::error_code ec, std::size_t)
                      {
                          if (!ec)
                          {
                              self->request_ = self->parser_->release();
                              self->parser_.reset();
                              self->process_request();
                          }
                          else if (ec == bhttp::error::end_of_stream)
                          {
                              self->handle_close();
                          }
                          else
                          {
                              util::Log(logDEBUG) << "Connection read error: " << ec.message();
                              self->handle_close();
                          }
                      });
}

void Connection::process_request()
{
    response_ = {};
    response_.version(request_.version());

    boost::asio::ip::address remote_address;
    boost::beast::error_code endpoint_ec;
    const auto endpoint = stream_.socket().remote_endpoint(endpoint_ec);
    if (!endpoint_ec)
    {
        remote_address = endpoint.address();
    }

    try
    {
        request_handler_.HandleRequest(request_, response_, remote_address);
    }
    catch (const std::exception &e)
    {
        util::Log(logERROR) << "Request processing error: " << e.what();
        SetInternalServerError(response_);
    }

    const bool keep_alive = should_keep_alive();
    response_.keep_alive(keep_alive);

    if (keep_alive)
    {
        response_.set(bhttp::field::connection, "keep-alive");
        response_.set("Keep-Alive",
                      util::compat::format("timeout={}, max={}",
                                           keepalive_timeout_,
                                           KEEPALIVE_MAX_REQUESTS - processed_requests_));
    }
    else
    {
        response_.set(bhttp::field::connection, "close");
    }

    const auto compression = determine_compression();
    if (compression != http::no_compression && !response_.body().empty())
    {
        auto compressed = compress_buffers(response_.body(), compression);
        response_.body() = std::move(compressed);

        if (compression == http::gzip_rfc1952)
        {
            response_.set(bhttp::field::content_encoding, "gzip");
        }
        else if (compression == http::deflate_rfc1951)
        {
            response_.set(bhttp::field::content_encoding, "deflate");
        }

        response_.set(bhttp::field::vary, "Accept-Encoding");
    }

    response_.prepare_payload();

    ++processed_requests_;

    handle_write();
}

void Connection::handle_write()
{
    auto self = shared_from_this();

    stream_.expires_after(std::chrono::seconds(keepalive_timeout_));

    bhttp::async_write(stream_,
                       response_,
                       [self](boost::beast::error_code ec, std::size_t)
                       {
                           if (!ec)
                           {
                               if (self->should_keep_alive())
                               {
                                   self->handle_read();
                               }
                               else
                               {
                                   self->handle_close();
                               }
                           }
                           else
                           {
                               util::Log(logDEBUG) << "Connection write error: " << ec.message();
                           }
                       });
}

void Connection::handle_close()
{
    boost::beast::error_code ec;
    (void)stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

    // Don't worry about shutdown errors
    if (ec && ec != boost::beast::errc::not_connected)
    {
        util::Log(logDEBUG) << "Connection shutdown error: " << ec.message();
    }
}

http::compression_type Connection::determine_compression()
{
    // Check Accept-Encoding header
    auto it = request_.find(bhttp::field::accept_encoding);
    if (it == request_.end())
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
    if (!request_.keep_alive())
    {
        return false;
    }

    if (processed_requests_ >= KEEPALIVE_MAX_REQUESTS)
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
