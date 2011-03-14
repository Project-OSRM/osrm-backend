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

#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "Connection.h"
#include "RequestHandler.h"

namespace http {

class Server: private boost::noncopyable {
public:
	explicit Server(const std::string& address, const std::string& port, unsigned thread_pool_size) : threadPoolSize(thread_pool_size), acceptor(ioService), newConnection(new Connection(ioService, requestHandler)), requestHandler(){
		boost::asio::ip::tcp::resolver resolver(ioService);
		boost::asio::ip::tcp::resolver::query query(address, port);
		boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

		acceptor.open(endpoint.protocol());
		acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor.bind(endpoint);
		acceptor.listen();
		acceptor.async_accept(newConnection->socket(), boost::bind(&Server::handleAccept, this, boost::asio::placeholders::error));
	}

	void Run() {
		std::vector<boost::shared_ptr<boost::thread> > threads;
		for (unsigned i = 0; i < threadPoolSize; ++i) {
			boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&boost::asio::io_service::run, &ioService)));
			threads.push_back(thread);
		}
		for (unsigned i = 0; i < threads.size(); ++i)
			threads[i]->join();
	}

	void Stop() {
		ioService.stop();
	}

	RequestHandler & GetRequestHandlerPtr() {
		return requestHandler;
	}

private:
	typedef boost::shared_ptr<Connection > ConnectionPtr;

	void handleAccept(const boost::system::error_code& e) {
		if (!e) {
			newConnection->start();
			newConnection.reset(new Connection(ioService, requestHandler));
			acceptor.async_accept(newConnection->socket(), boost::bind(&Server::handleAccept, this, boost::asio::placeholders::error));
		}
	}

	unsigned threadPoolSize;
	boost::asio::io_service ioService;
	boost::asio::ip::tcp::acceptor acceptor;
	ConnectionPtr newConnection;
	RequestHandler requestHandler;
};

}   // namespace http

#endif // SERVER_H
