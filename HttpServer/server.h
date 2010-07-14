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

#ifndef HTTP_ROUTER_SERVER_HPP
#define HTTP_ROUTER_SERVER_HPP

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <climits>
#include <string>
#include <vector>

#include "../typedefs.h"

#include "connection.h"
#include "request_handler.h"

namespace http {

/// The top-level class of the HTTP server.
template<typename GraphT>
class server: private boost::noncopyable
{
public:
    /// Construct the server to listen on the specified TCP address and port, and
    /// serve up files from the given directory.
    explicit server(const std::string& address, const std::string& port, std::size_t thread_pool_size, SearchEngine<EdgeData, GraphT, NodeInformationHelpDesk> * s)
    : thread_pool_size_(thread_pool_size),
      acceptor_(io_service_),
      new_connection_(new connection<GraphT>(io_service_, request_handler_)),
      request_handler_(s),
      sEngine(s)
    {
        // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
        boost::asio::ip::tcp::resolver resolver(io_service_);
        boost::asio::ip::tcp::resolver::query query(address, port);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
        acceptor_.async_accept(new_connection_->socket(), boost::bind(&server::handle_accept, this, boost::asio::placeholders::error));
    }

    /// Run the server's io_service loop.
    void run()
    {
        // Create a pool of threads to run all of the io_services.
        std::vector<boost::shared_ptr<boost::thread> > threads;
        for (std::size_t i = 0; i < thread_pool_size_; ++i)
        {
            boost::shared_ptr<boost::thread> thread(new boost::thread(
                    boost::bind(&boost::asio::io_service::run, &io_service_)));
            threads.push_back(thread);
        }

        // Wait for all threads in the pool to exit.
        for (std::size_t i = 0; i < threads.size(); ++i)
            threads[i]->join();
    }

    /// Stop the server.
    void stop()
    {
        io_service_.stop();
    }

private:
    typedef boost::shared_ptr<connection<GraphT> > connection_ptr;

    /// Handle completion of an asynchronous accept operation.
    void handle_accept(const boost::system::error_code& e)
    {
        if (!e)
        {
            new_connection_->start();
            new_connection_.reset(new connection<GraphT>(io_service_, request_handler_));
            acceptor_.async_accept(new_connection_->socket(),
                    boost::bind(&server::handle_accept, this,
                            boost::asio::placeholders::error));
        }
    }

    /// The number of threads that will call io_service::run().
    std::size_t thread_pool_size_;

    /// The io_service used to perform asynchronous operations.
    boost::asio::io_service io_service_;

    /// Acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor acceptor_;

    /// The next connection to be accepted.
    connection_ptr new_connection_;

    /// The handler for all incoming requests.
    request_handler<GraphT> request_handler_;

    /// The object to query the Routing Engine
    SearchEngine<EdgeData, GraphT> * sEngine;

};

}   // namespace http

#endif // HTTP_ROUTER_SERVER_HPP
