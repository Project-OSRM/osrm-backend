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

#ifndef BASIC_DATASTRUCTURES_H
#define BASIC_DATASTRUCTURES_H

#include "../Util/StringUtil.h"

#include <boost/asio.hpp>
#include <boost/foreach.hpp>

#include <string>
#include <sstream>
#include <vector>

namespace http {

const std::string okString 					= "HTTP/1.0 200 OK\r\n";
const std::string badRequestString 			= "HTTP/1.0 400 Bad Request\r\n";
const std::string internalServerErrorString = "HTTP/1.0 500 Internal Server Error\r\n";

const char okHTML[] 				 = "";
const char badRequestHTML[] 		 = "<html><head><title>Bad Request</title></head><body><h1>400 Bad Request</h1></body></html>";
const char internalServerErrorHTML[] = "<html><head><title>Internal Server Error</title></head><body><h1>500 Internal Server Error</h1></body></html>";
const char seperators[]  			 = { ':', ' ' };
const char crlf[]		             = { '\r', '\n' };

struct Header {
  std::string name;
  std::string value;
  void Clear() {
      name.clear();
      value.clear();
  }
};

enum CompressionType {
    noCompression,
    gzipRFC1952,
    deflateRFC1951
} Compression;

struct Request {
	std::string uri;
	std::string referrer;
	std::string agent;
	boost::asio::ip::address endpoint;
};

struct Reply {
    Reply() : status(ok) { content.reserve(2 << 20); }
	enum status_type {
		ok 					= 200,
		badRequest 		    = 400,
		internalServerError = 500
	} status;

	std::vector<Header> headers;
    std::vector<boost::asio::const_buffer> toBuffers();
    std::vector<boost::asio::const_buffer> HeaderstoBuffers();
	std::string content;
	static Reply stockReply(status_type status);
	void setSize(const unsigned size) {
		BOOST_FOREACH ( Header& h,  headers) {
			if("Content-Length" == h.name) {
				std::string sizeString;
				intToString(size,h.value);
			}
		}
	}
};

boost::asio::const_buffer ToBuffer(Reply::status_type status) {
	switch (status) {
	case Reply::ok:
		return boost::asio::buffer(okString);
	case Reply::internalServerError:
		return boost::asio::buffer(internalServerErrorString);
	default:
		return boost::asio::buffer(badRequestString);
	}
}

std::string ToString(Reply::status_type status) {
	switch (status) {
	case Reply::ok:
		return okHTML;
	case Reply::badRequest:
		return badRequestHTML;
	default:
		return internalServerErrorHTML;
	}
}

std::vector<boost::asio::const_buffer> Reply::toBuffers(){
	std::vector<boost::asio::const_buffer> buffers;
	buffers.push_back(ToBuffer(status));
	for (std::size_t i = 0; i < headers.size(); ++i) {
		Header& h = headers[i];
		buffers.push_back(boost::asio::buffer(h.name));
		buffers.push_back(boost::asio::buffer(seperators));
		buffers.push_back(boost::asio::buffer(h.value));
		buffers.push_back(boost::asio::buffer(crlf));
	}
	buffers.push_back(boost::asio::buffer(crlf));
	buffers.push_back(boost::asio::buffer(content));
	return buffers;
}

std::vector<boost::asio::const_buffer> Reply::HeaderstoBuffers(){
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(ToBuffer(status));
    for (std::size_t i = 0; i < headers.size(); ++i) {
        Header& h = headers[i];
        buffers.push_back(boost::asio::buffer(h.name));
        buffers.push_back(boost::asio::buffer(seperators));
        buffers.push_back(boost::asio::buffer(h.value));
        buffers.push_back(boost::asio::buffer(crlf));
    }
    buffers.push_back(boost::asio::buffer(crlf));
    return buffers;
}

Reply Reply::stockReply(Reply::status_type status) {
	Reply rep;
	rep.status = status;
	rep.content = ToString(status);
	rep.headers.resize(3);
	rep.headers[0].name = "Access-Control-Allow-Origin";
	rep.headers[0].value = "*";
	rep.headers[1].name = "Content-Length";

    std::string s;
    intToString(rep.content.size(), s);

    rep.headers[1].value = s;
	rep.headers[2].name = "Content-Type";
	rep.headers[2].value = "text/html";
	return rep;
}
} // namespace http

#endif //BASIC_DATASTRUCTURES_H
