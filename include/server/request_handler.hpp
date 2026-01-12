#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "server/service_handler.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/beast/http.hpp>

namespace osrm::server
{

namespace beast = boost::beast;
namespace http_proto = beast::http;

using BeastRequest = http_proto::request<http_proto::string_body>;
using BeastResponse = http_proto::response<http_proto::vector_body<char>>;

class RequestHandler
{

  public:
    RequestHandler() = default;
    RequestHandler(const RequestHandler &) = delete;
    RequestHandler &operator=(const RequestHandler &) = delete;

    void RegisterServiceHandler(std::unique_ptr<ServiceHandlerInterface> service_handler);

    void HandleRequest(const BeastRequest &current_request,
                       BeastResponse &current_reply,
                       const boost::asio::ip::address &remote_address);

  private:
    std::unique_ptr<ServiceHandlerInterface> service_handler;
};
} // namespace osrm::server

#endif // REQUEST_HANDLER_HPP
