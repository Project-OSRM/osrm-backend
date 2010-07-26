/*
    open source routing machine
    Copyright (C) Dennis Luxen, 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */


#ifndef HTTP_SERVER3_REPLY_HPP
#define HTTP_SERVER3_REPLY_HPP

#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include "header.h"

namespace http {

/// A reply to be sent to a client.
struct reply
{
	/// The status of the reply.
	enum status_type
	{
		ok = 200,
				bad_request = 400,
				internal_server_error = 500
	} status;

	std::vector<header> headers;

	/// The content to be sent in the reply.
	std::string content;

	/// Convert the reply into a vector of buffers. The buffers do not own the
	/// underlying memory blocks, therefore the reply object must remain valid and
	/// not be changed until the write operation has completed.
	std::vector<boost::asio::const_buffer> to_buffers();

	/// Get a stock reply.
	static reply stock_reply(status_type status);
};

namespace status_strings {

const std::string ok =
		"HTTP/1.0 200 OK\r\n";
const std::string bad_request =
		"HTTP/1.0 400 Bad Request\r\n";
const std::string internal_server_error =
		"HTTP/1.0 500 Internal Server Error\r\n";

boost::asio::const_buffer to_buffer(reply::status_type status)
{
	switch (status)
	{
	case reply::ok:
		return boost::asio::buffer(ok);
	case reply::bad_request:
		return boost::asio::buffer(bad_request);
	case reply::internal_server_error:
		return boost::asio::buffer(internal_server_error);
	default:
		return boost::asio::buffer(bad_request);
	}
}

} // namespace status_strings

namespace misc_strings {

const char name_value_separator[] = { ':', ' ' };
const char crlf[] = { '\r', '\n' };

} // namespace misc_strings

std::vector<boost::asio::const_buffer> reply::to_buffers()
{
	std::vector<boost::asio::const_buffer> buffers;
	buffers.push_back(status_strings::to_buffer(status));
	for (std::size_t i = 0; i < headers.size(); ++i)
	{
		header& h = headers[i];
		buffers.push_back(boost::asio::buffer(h.name));
		buffers.push_back(boost::asio::buffer(misc_strings::name_value_separator));
		buffers.push_back(boost::asio::buffer(h.value));
		buffers.push_back(boost::asio::buffer(misc_strings::crlf));
	}
	buffers.push_back(boost::asio::buffer(misc_strings::crlf));
	buffers.push_back(boost::asio::buffer(content));
	return buffers;
}

namespace stock_replies {

const char ok[] = "";
const char bad_request[] =
		"<html>"
		"<head><title>Bad Request</title></head>"
		"<body><h1>400 Bad Request</h1></body>"
		"</html>";
const char internal_server_error[] =
		"<html>"
		"<head><title>Internal Server Error</title></head>"
		"<body><h1>500 Internal Server Error</h1></body>"
		"</html>";

std::string to_string(reply::status_type status)
{
	switch (status)
	{
	case reply::ok:
		return ok;
	case reply::bad_request:
		return bad_request;
	case reply::internal_server_error:
		return internal_server_error;
	default:
		return internal_server_error;
	}
}

} // namespace stock_replies

reply reply::stock_reply(reply::status_type status)
{
	reply rep;
	rep.status = status;
	rep.content = stock_replies::to_string(status);
	rep.headers.resize(2);
	rep.headers[0].name = "Content-Length";
	rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
	rep.headers[1].name = "Content-Type";
	rep.headers[1].value = "text/html";
	return rep;
}

} // namespace http

#endif // HTTP_SERVER3_REPLY_HPP
