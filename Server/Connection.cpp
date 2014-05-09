/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "Connection.h"
#include "RequestHandler.h"
#include "RequestParser.h"

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <string>
#include <vector>

namespace http
{

Connection::Connection(boost::asio::io_service &io_service, RequestHandler &handler)
    : strand(io_service), TCP_socket(io_service), request_handler(handler),
      request_parser(new RequestParser())
{
}

Connection::~Connection() { delete request_parser; }

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
}

void Connection::handle_read(const boost::system::error_code &e, std::size_t bytes_transferred)
{
    if (!e)
    {
        CompressionType compression_type(noCompression);
        boost::tribool result;
        boost::tie(result, boost::tuples::ignore) =
            request_parser->Parse(request,
                                  incoming_data_buffer.data(),
                                  incoming_data_buffer.data() + bytes_transferred,
                                  &compression_type);

        if (result)
        {
            request.endpoint = TCP_socket.remote_endpoint().address();
            request_handler.handle_request(request, reply);

            Header compression_header;
            std::vector<char> compressed_output;
            std::vector<boost::asio::const_buffer> output_buffer;
            switch (compression_type)
            {
            case deflateRFC1951:
                compression_header.name = "Content-Encoding";
                compression_header.value = "deflate";
                reply.headers.insert(reply.headers.begin(), compression_header);
                compressBufferCollection(reply.content, compression_type, compressed_output);
                reply.setSize(compressed_output.size());
                output_buffer = reply.HeaderstoBuffers();
                output_buffer.push_back(boost::asio::buffer(compressed_output));
                boost::asio::async_write(
                    TCP_socket,
                    output_buffer,
                    strand.wrap(boost::bind(&Connection::handle_write,
                                            this->shared_from_this(),
                                            boost::asio::placeholders::error)));
                break;
            case gzipRFC1952:
                compression_header.name = "Content-Encoding";
                compression_header.value = "gzip";
                reply.headers.insert(reply.headers.begin(), compression_header);
                compressBufferCollection(reply.content, compression_type, compressed_output);
                reply.setSize(compressed_output.size());
                output_buffer = reply.HeaderstoBuffers();
                output_buffer.push_back(boost::asio::buffer(compressed_output));
                boost::asio::async_write(
                    TCP_socket,
                    output_buffer,
                    strand.wrap(boost::bind(&Connection::handle_write,
                                            this->shared_from_this(),
                                            boost::asio::placeholders::error)));
                break;
            case noCompression:
                reply.SetUncompressedSize();
                output_buffer = reply.toBuffers();
                boost::asio::async_write(
                    TCP_socket,
                    output_buffer,
                    strand.wrap(boost::bind(&Connection::handle_write,
                                            this->shared_from_this(),
                                            boost::asio::placeholders::error)));
                break;
            }
        }
        else if (!result)
        {
            reply = Reply::StockReply(Reply::badRequest);

            boost::asio::async_write(TCP_socket,
                                     reply.toBuffers(),
                                     strand.wrap(boost::bind(&Connection::handle_write,
                                                             this->shared_from_this(),
                                                             boost::asio::placeholders::error)));
        }
        else
        {
            TCP_socket.async_read_some(
                boost::asio::buffer(incoming_data_buffer),
                strand.wrap(boost::bind(&Connection::handle_read,
                                        this->shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred)));
        }
    }
}

/// Handle completion of a write operation.
void Connection::handle_write(const boost::system::error_code &e)
{
    if (!e)
    {
        // Initiate graceful connection closure.
        boost::system::error_code ignoredEC;
        TCP_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignoredEC);
    }
}

void Connection::compressBufferCollection(std::vector<std::string> uncompressed_data,
                                          CompressionType compression_type,
                                          std::vector<char> &compressed_data)
{
    boost::iostreams::gzip_params compression_parameters;

    compression_parameters.level = boost::iostreams::zlib::best_speed;
    if (deflateRFC1951 == compression_type)
    {
        compression_parameters.noheader = true;
    }

    BOOST_ASSERT(compressed_data.empty());
    boost::iostreams::filtering_ostream compressing_stream;

    compressing_stream.push(boost::iostreams::gzip_compressor(compression_parameters));
    compressing_stream.push(boost::iostreams::back_inserter(compressed_data));

    for (const std::string &line : uncompressed_data)
    {
        compressing_stream << line;
    }

    compressing_stream.reset();
}
}
