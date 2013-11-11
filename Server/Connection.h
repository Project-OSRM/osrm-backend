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

#ifndef CONNECTION_H
#define CONNECTION_H

#include "BasicDatastructures.h"
#include "RequestHandler.h"
#include "RequestParser.h"

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <zlib.h>

#include <vector>

namespace http {

/// Represents a single connection from a client.
class Connection : 	public boost::enable_shared_from_this<Connection>,
					private boost::noncopyable {
public:
	explicit Connection(
		boost::asio::io_service& io_service,
		RequestHandler& handler
	) : strand(io_service), TCP_socket(io_service), request_handler(handler) { }

	boost::asio::ip::tcp::socket& socket() {
		return TCP_socket;
	}

	/// Start the first asynchronous operation for the connection.
	void start() {
	    TCP_socket.async_read_some(
	    	boost::asio::buffer(incoming_data_buffer),
	    	strand.wrap( boost::bind(
	    					&Connection::handle_read,
	    					this->shared_from_this(),
	    					boost::asio::placeholders::error,
	    					boost::asio::placeholders::bytes_transferred)
	    	)
	    );
	}

private:
	void handle_read(
		const boost::system::error_code& e,
		std::size_t bytes_transferred
	) {
		if( !e ) {
			CompressionType compression_type(noCompression);
			boost::tribool result;
			boost::tie(result, boost::tuples::ignore) = request_parser.Parse(
				request,
				incoming_data_buffer.data(),
				incoming_data_buffer.data() + bytes_transferred,
				&compression_type
			);

			std::vector<boost::asio::const_buffer> output_buffer;
			if( result ) {
				request.endpoint = TCP_socket.remote_endpoint().address();
				request_handler.handle_request(request, reply);

				Header compression_header;
				std::vector<unsigned char> compressed_output;
				switch(compression_type) {
				case deflateRFC1951:
					compression_header.name = "Content-Encoding";
					compression_header.value = "deflate";
					reply.headers.insert(
						reply.headers.begin(),
						compression_header
					);
					compressCharArray(
						reply.content.c_str(),
						reply.content.length(),
						compressed_output,
						compression_type
					);
					reply.setSize(compressed_output.size());
					reply.HeadersToBuffers(output_buffer);
					output_buffer.push_back(
						boost::asio::buffer(compressed_output)
					);
					boost::asio::async_write(
						TCP_socket,
						output_buffer,
						strand.wrap(
							boost::bind(
								&Connection::handle_write,
								this->shared_from_this(),
								boost::asio::placeholders::error
							)
						)
					);
					break;
				case gzipRFC1952:
					compression_header.name = "Content-Encoding";
					compression_header.value = "gzip";
					reply.headers.insert(
						reply.headers.begin(),
						compression_header
					);
					compressCharArray(
						reply.content.c_str(),
						reply.content.length(),
						compressed_output,
						compression_type
					);
					reply.setSize(compressed_output.size());
					reply.HeadersToBuffers(output_buffer);
					output_buffer.push_back(
						boost::asio::buffer(compressed_output)
					);
					boost::asio::async_write(
						TCP_socket,
						output_buffer,
						strand.wrap(
							boost::bind(
								&Connection::handle_write,
								this->shared_from_this(),
								boost::asio::placeholders::error
							)
						)
					);
					break;
				case noCompression:
					reply.HeadersToBuffers(output_buffer);
					boost::asio::async_write(
						TCP_socket,
						output_buffer,
						strand.wrap(
							boost::bind(
								&Connection::handle_write,
								this->shared_from_this(),
								boost::asio::placeholders::error
								)
							)
						);
					break;
				}
			} else if (!result) {
				reply = Reply::stockReply(Reply::badRequest);
				reply.HeadersToBuffers(output_buffer);
				boost::asio::async_write(
					TCP_socket,
					output_buffer,
					strand.wrap(
						boost::bind(
							&Connection::handle_write,
							this->shared_from_this(),
							boost::asio::placeholders::error
						)
					)
				);
			} else {
				TCP_socket.async_read_some(
					boost::asio::buffer(incoming_data_buffer),
					strand.wrap(
						boost::bind(
							&Connection::handle_read,
							this->shared_from_this(),
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred
						)
					)
				);
			}
		}
	}

	/// Handle completion of a write operation.
	void handle_write(const boost::system::error_code& e) {
		if (!e) {
			// Initiate graceful connection closure.
			boost::system::error_code ignoredEC;
			TCP_socket.shutdown(
				boost::asio::ip::tcp::socket::shutdown_both,
				ignoredEC
			);
		}
	}

	// Big thanks to deusty who explains how to use gzip compression by
	// the right call to deflateInit2():
	// http://deusty.blogspot.com/2007/07/gzip-compressiondecompression.html
	void compressCharArray(
		const char * in_data,
		size_t in_data_size,
		std::vector<unsigned char> & buffer,
		CompressionType type
	) {
		const size_t BUFSIZE = 128 * 1024;
		unsigned char temp_buffer[BUFSIZE];

		z_stream strm;
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.total_out = 0;
		strm.next_in = (unsigned char *)(in_data);
		strm.avail_in = in_data_size;
		strm.next_out = temp_buffer;
		strm.avail_out = BUFSIZE;
		strm.data_type = Z_ASCII;

		switch(type){
		case deflateRFC1951:
			deflateInit(&strm, Z_BEST_SPEED);
			break;
		case gzipRFC1952:
			deflateInit2(
				&strm,
				Z_DEFAULT_COMPRESSION,
				Z_DEFLATED,
				(15+16),
				9,
				Z_DEFAULT_STRATEGY
			);
			break;
		default:
			BOOST_ASSERT_MSG(false, "should not happen");
			break;
		}

		int deflate_res = Z_OK;
		do {
			if ( 0 == strm.avail_out ) {
				buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
				strm.next_out = temp_buffer;
				strm.avail_out = BUFSIZE;
			}
			deflate_res = deflate(&strm, Z_FINISH);

		} while (deflate_res == Z_OK);

		BOOST_ASSERT_MSG(
			deflate_res == Z_STREAM_END,
			"compression not properly finished"
		);

		buffer.insert(
			buffer.end(),
			temp_buffer,
			temp_buffer + BUFSIZE - strm.avail_out
		);
		deflateEnd(&strm);
	}

	boost::asio::io_service::strand strand;
	boost::asio::ip::tcp::socket TCP_socket;
	RequestHandler& request_handler;
	boost::array<char, 8192> incoming_data_buffer;
	Request request;
	RequestParser request_parser;
	Reply reply;
};

} // namespace http

#endif // CONNECTION_H
