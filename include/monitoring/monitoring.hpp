#ifndef MONITORING_HPP
#define MONITORING_HPP

#include "monitoring/monitoring_request_handler.hpp"
#include "server/connection.hpp"

#include "util/log.hpp"

#include <boost/asio.hpp>
#include <boost/bind.hpp>

namespace osrm
{
namespace server
{

class Monitoring
{
  public:
    static std::shared_ptr<Monitoring>
    CreateMonitoring(std::string &ip_address, const int ip_port, const unsigned working_threads)
    {
        return std::make_shared<Monitoring>(ip_address, ip_port, working_threads);
    }

    explicit Monitoring(const std::string &address, const int port, const unsigned working_threads)
        : acceptor(io_service), request_handler(working_threads),
          new_connection(std::make_shared<Connection>(io_service, request_handler))
    {
        if (port == 0)
        {
            working = false;
            return;
        }
        working = true;
        const auto port_string = std::to_string(port);

        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::ip::tcp::resolver::query query(address, port_string);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

        acceptor.open(endpoint.protocol());
#ifdef SO_REUSEPORT
        const int option = 1;
        setsockopt(acceptor.native_handle(), SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));
#endif
        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor.bind(endpoint);
        acceptor.listen();

        util::Log() << "Monitoring endpoint listening on: " << acceptor.local_endpoint();

        acceptor.async_accept(
            new_connection->socket(),
            boost::bind(&Monitoring::HandleAccept, this, boost::asio::placeholders::error));
    }

    void Run()
    {
        if (working)
        {
            io_service.run();
        }
    }

    void Stop()
    {
        if (working)
        {
            io_service.stop();
        }
    }

    void RegisterServiceHandler(std::shared_ptr<ServiceHandlerInterface> service_handler_)
    {
        request_handler.RegisterServiceHandler(service_handler_);
    }

  private:
    void HandleAccept(const boost::system::error_code &e)
    {
        if (!e)
        {
            new_connection->start();
            new_connection = std::make_shared<Connection>(io_service, request_handler);
            acceptor.async_accept(
                new_connection->socket(),
                boost::bind(&Monitoring::HandleAccept, this, boost::asio::placeholders::error));
        }
    }

    boost::asio::io_service io_service;
    boost::asio::ip::tcp::acceptor acceptor;
    MonitoringRequestHandler request_handler;
    std::shared_ptr<Connection> new_connection;
    std::shared_ptr<std::thread> working_thread;
    bool working;
};

} // namespace server
} // namespace osrm

#endif // MONITORING_HPP
