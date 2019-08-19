#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <boost/asio.hpp>

#include <string>

namespace osrm
{
namespace server
{
namespace http
{

struct request
{
    std::string uri;
    std::string referrer;
    std::string agent;
    std::string connection;
    boost::asio::ip::address endpoint;
};
}
}
}

#endif // REQUEST_HPP
