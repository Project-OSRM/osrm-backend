#include "server/connection.hpp"
#include "server/request_handler.hpp"
#include "server/request_parser.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <iterator>
#include <string>
#include <vector>

namespace osrm
{
namespace server
{

Connection::Connection(boost::asio::io_service &io_service, RequestHandler &handler)
    : strand(io_service), TCP_socket(io_service), timer(io_service), request_handler(handler)
{
}

boost::asio::ip::tcp::socket &Connection::socket() { return TCP_socket; }

/// Start the first asynchronous operation for the connection.
void Connection::start()
{
    TCP_socket.async_read_some(
        boost::asio::buffer(incoming_data_buffer),
        strand.wrap(boost::bind(&Connection::handle_read,
                                this->shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred)));

    if (keep_alive)
    {
        // Ok, we know it is not a first request, as we switched to keepalive
        timer.cancel();
        timer.expires_from_now(boost::posix_time::seconds(keepalive_timeout));
        timer.async_wait(std::bind(
            &Connection::handle_timeout, this->shared_from_this(), std::placeholders::_1));
    }
}

void Connection::handle_read(const boost::system::error_code &error, std::size_t bytes_transferred)
{
    if (error)
    {
        return;
    }

    if (keep_alive)
    {
        timer.cancel();
        timer.expires_from_now(boost::posix_time::seconds(0));
    }

    // no error detected, let's parse the request
    http::compression_type compression_type(http::no_compression);
    RequestParser::RequestStatus result;
    std::tie(result, compression_type) =
        request_parser.parse(current_request,
                             incoming_data_buffer.data(),
                             incoming_data_buffer.data() + bytes_transferred);

    // the request has been parsed
    if (result == RequestParser::RequestStatus::valid)
    {
        current_request.endpoint = TCP_socket.remote_endpoint().address();
        request_handler.HandleRequest(current_request, current_reply);

        if (boost::iequals(current_request.connection, "close"))
        {
            current_reply.headers.emplace_back("Connection", "close");
        }
        else
        {
            keep_alive = true;
            current_reply.headers.emplace_back("Connection", "keep-alive");
            current_reply.headers.emplace_back("Keep-Alive", "timeout=5, max=512");
        }

        // compress the result w/ gzip/deflate if requested
        switch (compression_type)
        {
        case http::deflate_rfc1951:
            // use deflate for compression
            current_reply.headers.insert(current_reply.headers.begin(),
                                         {"Content-Encoding", "deflate"});
            compressed_output = compress_buffers(current_reply.content, compression_type);
            current_reply.set_size(static_cast<unsigned>(compressed_output.size()));
            output_buffer = current_reply.headers_to_buffers();
            output_buffer.push_back(boost::asio::buffer(compressed_output));
            break;
        case http::gzip_rfc1952:
            // use gzip for compression
            current_reply.headers.insert(current_reply.headers.begin(),
                                         {"Content-Encoding", "gzip"});
            compressed_output = compress_buffers(current_reply.content, compression_type);
            current_reply.set_size(static_cast<unsigned>(compressed_output.size()));
            output_buffer = current_reply.headers_to_buffers();
            output_buffer.push_back(boost::asio::buffer(compressed_output));
            break;
        case http::no_compression:
            // don't use any compression
            current_reply.set_uncompressed_size();
            output_buffer = current_reply.to_buffers();
            break;
        }
        // write result to stream
        boost::asio::async_write(TCP_socket,
                                 output_buffer,
                                 strand.wrap(boost::bind(&Connection::handle_write,
                                                         this->shared_from_this(),
                                                         boost::asio::placeholders::error)));
    }
    else if (result == RequestParser::RequestStatus::invalid)
    { // request is not parseable
        current_reply = http::reply::stock_reply(http::reply::bad_request);

        boost::asio::async_write(TCP_socket,
                                 current_reply.to_buffers(),
                                 strand.wrap(boost::bind(&Connection::handle_write,
                                                         this->shared_from_this(),
                                                         boost::asio::placeholders::error)));
    }
    else
    {
        // we don't have a result yet, so continue reading
        TCP_socket.async_read_some(
            boost::asio::buffer(incoming_data_buffer),
            strand.wrap(boost::bind(&Connection::handle_read,
                                    this->shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred)));
    }
}

/// Handle completion of a write operation.
void Connection::handle_write(const boost::system::error_code &error)
{
    if (!error)
    {
        if (keep_alive && processed_requests > 0)
        {
            --processed_requests;
            current_request = http::request();
            current_reply = http::reply();
            request_parser = RequestParser();
            this->start();
        }
        else
        {
            handle_shutdown();
        }
    }
}

/// Handle completion of a timeout timer..
void Connection::handle_timeout(boost::system::error_code ec)
{
    // We can get there for 3 reasons: spurious wakeup by timer.cancel(), which should be ignored
    // Slow client with a delayed _first_ request, which should be ignored too
    // Absent next request during waiting time in the keepalive mode - should stop right there.
    if (ec != boost::asio::error::operation_aborted)
    {
        TCP_socket.cancel();
        handle_shutdown();
    }
}

void Connection::handle_shutdown()
{
    // Initiate graceful connection closure.
    boost::system::error_code ignore_error;
    TCP_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore_error);
}

std::vector<char> Connection::compress_buffers(const std::vector<char> &uncompressed_data,
                                               const http::compression_type compression_type)
{
    boost::iostreams::gzip_params compression_parameters;

    // there's a trade-off between speed and size. speed wins
    compression_parameters.level = boost::iostreams::zlib::best_speed;
    // check which compression flavor is used
    if (http::deflate_rfc1951 == compression_type)
    {
        compression_parameters.noheader = true;
    }

    std::vector<char> compressed_data;
    // plug data into boost's compression stream
    boost::iostreams::filtering_ostream gzip_stream;
    gzip_stream.push(boost::iostreams::gzip_compressor(compression_parameters));
    gzip_stream.push(boost::iostreams::back_inserter(compressed_data));
    gzip_stream.write(&uncompressed_data[0], uncompressed_data.size());
    boost::iostreams::close(gzip_stream);

    return compressed_data;
}
}
}
