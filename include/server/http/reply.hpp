#ifndef REPLY_HPP
#define REPLY_HPP

#include "server/http/header.hpp"

#include <boost/asio.hpp>

#include <vector>

namespace osrm
{
namespace server
{
namespace http
{

class reply
{
  public:
    enum status_type
    {
        ok = 200,
        bad_request = 400,
        internal_server_error = 500
    } status;

    std::vector<header> headers;
    std::vector<boost::asio::const_buffer> to_buffers();
    std::vector<boost::asio::const_buffer> headers_to_buffers();
    std::vector<char> content;
    static reply stock_reply(const status_type status);
    void set_size(const std::size_t size);
    void set_uncompressed_size();

    reply();

  private:
    std::string status_to_string(reply::status_type status);
    boost::asio::const_buffer status_to_buffer(reply::status_type status);
};
} // namespace http
} // namespace server
} // namespace osrm

#endif // REPLY_HPP
