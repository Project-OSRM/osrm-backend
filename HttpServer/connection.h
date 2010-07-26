/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

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

#ifndef HTTP_SERVER3_CONNECTION_HPP
#define HTTP_CONNECTION_HPP

#include <vector>

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "reply.h"
#include "request.h"
#include "request_handler.h"
#include "request_parser.h"

namespace http {


/// Represents a single connection from a client.
template<typename GraphT>
class connection
: public boost::enable_shared_from_this<connection<GraphT> >,
private boost::noncopyable
{
public:
    /// Construct a connection with the given io_service.
    explicit connection(boost::asio::io_service& io_service,
            request_handler<GraphT>& handler)
    : strand_(io_service),
      socket_(io_service),
      request_handler_(handler)
    {
    }

    /// Get the socket associated with the connection.
    boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }

    /// Start the first asynchronous operation for the connection.
    void start()
    {
        socket_.async_read_some(boost::asio::buffer(buffer_),
                strand_.wrap(
                        boost::bind(&connection<GraphT>::handle_read, this->shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred)));
    }

private:
    /// Handle completion of a read operation.
    void handle_read(const boost::system::error_code& e,
            std::size_t bytes_transferred)
    {
        if (!e)
        {
            boost::tribool result;
            boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
                    request_, buffer_.data(), buffer_.data() + bytes_transferred);

            if (result)
            {
                request_handler_.handle_request(request_, reply_);
                boost::asio::async_write(socket_, reply_.to_buffers(),
                        strand_.wrap(
                                boost::bind(&connection<GraphT>::handle_write, this->shared_from_this(),
                                        boost::asio::placeholders::error)));
            }
            else if (!result)
            {
                reply_ = reply::stock_reply(reply::bad_request);
                boost::asio::async_write(socket_, reply_.to_buffers(),
                        strand_.wrap(
                                boost::bind(&connection<GraphT>::handle_write, this->shared_from_this(),
                                        boost::asio::placeholders::error)));
            }
            else
            {
                socket_.async_read_some(boost::asio::buffer(buffer_),
                        strand_.wrap(
                                boost::bind(&connection<GraphT>::handle_read, this->shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred)));
            }
        }
    }

    /// Handle completion of a write operation.
    void handle_write(const boost::system::error_code& e)
    {
        if (!e)
        {
            // Initiate graceful connection closure.
            boost::system::error_code ignored_ec;
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        }

        // No new asynchronous operations are started. This means that all shared_ptr
        // references to the connection object will disappear and the object will be
        // destroyed automatically after this handler returns. The connection class's
        // destructor closes the socket.
    }

    /// Strand to ensure the connection's handlers are not called concurrently.
    boost::asio::io_service::strand strand_;

    /// Socket for the connection.
    boost::asio::ip::tcp::socket socket_;

    /// The handler used to process the incoming request.
    request_handler<GraphT>& request_handler_;

    /// Buffer for incoming data.
    boost::array<char, 8192> buffer_;

    /// The incoming request.
    Request request_;

    /// The parser for the incoming request.
    request_parser request_parser_;

    /// The reply to be sent back to the client.
    reply reply_;

};

} // namespace http

#endif /* CONNECTION_HPP_ */
