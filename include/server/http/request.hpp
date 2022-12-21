#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <boost/asio.hpp>

#include <string>

namespace osrm::server::http
{

struct request
{
    std::string uri;
    std::string referrer;
    std::string agent;
    std::string connection;
    boost::asio::ip::address endpoint;
};
} // namespace osrm::server::http

#endif // REQUEST_HPP
