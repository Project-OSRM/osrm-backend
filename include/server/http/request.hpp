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
    std::string method;
    std::string body;
    std::size_t content_length = 0;
    boost::asio::ip::address endpoint;
};
} // namespace osrm::server::http

#endif // REQUEST_HPP
