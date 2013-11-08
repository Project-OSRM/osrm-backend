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

#ifndef SERVER_H
#define SERVER_H

#include "Connection.h"
#include "RequestHandler.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <vector>

class Server: private boost::noncopyable {
public:
	explicit Server(
		const std::string& address,
		const std::string& port,
		unsigned thread_pool_size
	) :
		threadPoolSize(thread_pool_size),
		acceptor(ioService),
		newConnection(new http::Connection(ioService, requestHandler)),
		requestHandler()
	{
		boost::asio::ip::tcp::resolver resolver(ioService);
		boost::asio::ip::tcp::resolver::query query(address, port);
		boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

		acceptor.open(endpoint.protocol());
		acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor.bind(endpoint);
		acceptor.listen();
		acceptor.async_accept(
			newConnection->socket(),
			boost::bind(
				&Server::handleAccept,
				this,
				boost::asio::placeholders::error
			)
		);
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
	void handleAccept(const boost::system::error_code& e) {
		if (!e) {
			newConnection->start();
			newConnection.reset(
				new http::Connection(ioService, requestHandler)
			);
			acceptor.async_accept(
				newConnection->socket(),
				boost::bind(
					&Server::handleAccept,
					this,
					boost::asio::placeholders::error
				)
			);
		}
	}

	unsigned threadPoolSize;
	boost::asio::io_service ioService;
	boost::asio::ip::tcp::acceptor acceptor;
	boost::shared_ptr<http::Connection> newConnection;
	RequestHandler requestHandler;
};

#endif // SERVER_H
