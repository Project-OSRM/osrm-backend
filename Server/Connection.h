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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <vector>

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "BasicDatastructures.h"
#include "RequestHandler.h"
#include "RequestParser.h"

namespace http {

/// Represents a single connection from a client.
class Connection : public boost::enable_shared_from_this<Connection>, private boost::noncopyable {
public:
    explicit Connection(boost::asio::io_service& io_service, RequestHandler& handler) : strand(io_service), TCPsocket(io_service), requestHandler(handler) {}

    boost::asio::ip::tcp::socket& socket() {
        return TCPsocket;
    }

    /// Start the first asynchronous operation for the connection.
    void start() {
        TCPsocket.async_read_some(boost::asio::buffer(incomingDataBuffer), strand.wrap( boost::bind(&Connection::handleRead, this->shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
    }

private:
    void handleRead(const boost::system::error_code& e, std::size_t bytes_transferred) {
        if (!e) {
            boost::tribool result;
            boost::tie(result, boost::tuples::ignore) = requestParser.Parse( request, incomingDataBuffer.data(), incomingDataBuffer.data() + bytes_transferred);

            if (result) {
                requestHandler.handle_request(request, reply);
                boost::asio::async_write(TCPsocket, reply.toBuffers(), strand.wrap( boost::bind(&Connection::handleWrite, this->shared_from_this(), boost::asio::placeholders::error)));
            } else if (!result) {
                reply = Reply::stockReply(Reply::badRequest);
                boost::asio::async_write(TCPsocket, reply.toBuffers(), strand.wrap( boost::bind(&Connection::handleWrite, this->shared_from_this(), boost::asio::placeholders::error)));
            } else {
                TCPsocket.async_read_some(boost::asio::buffer(incomingDataBuffer), strand.wrap( boost::bind(&Connection::handleRead, this->shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
            }
        }
    }

    /// Handle completion of a write operation.
    void handleWrite(const boost::system::error_code& e) {
        if (!e) {
            // Initiate graceful connection closure.
            boost::system::error_code ignoredEC;
            TCPsocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignoredEC);
        }
        // No new asynchronous operations are started. This means that all shared_ptr
        // references to the connection object will disappear and the object will be
        // destroyed automatically after this handler returns. The connection class's
        // destructor closes the socket.
    }

    boost::asio::io_service::strand strand;
    boost::asio::ip::tcp::socket TCPsocket;
    RequestHandler& requestHandler;
    boost::array<char, 8192> incomingDataBuffer;
    Request request;
    RequestParser requestParser;
    Reply reply;
};

} // namespace http

#endif // CONNECTION_H
