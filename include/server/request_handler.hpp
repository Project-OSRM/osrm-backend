#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "server/service_handler.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/beast/http.hpp>

namespace osrm::server
{

using Request = boost::beast::http::request<boost::beast::http::string_body>;
using Response = boost::beast::http::response<boost::beast::http::vector_body<char>>;

inline void SetInternalServerError(Response &res)
{
    static constexpr char body[] =
        "{\"code\": \"InternalError\",\"message\":\"Internal Server Error\"}";
    res.result(boost::beast::http::status::internal_server_error);
    res.set("Access-Control-Allow-Origin", "*");
    res.set("Access-Control-Allow-Methods", "GET");
    res.set("Access-Control-Allow-Headers", "X-Requested-With, Content-Type");
    res.set(boost::beast::http::field::content_type, "application/json; charset=UTF-8");
    res.body().assign(body, body + (sizeof(body) - 1)); // drop trailing '\0'
}

class RequestHandler
{

  public:
    RequestHandler() = default;
    RequestHandler(const RequestHandler &) = delete;
    RequestHandler &operator=(const RequestHandler &) = delete;

    void RegisterServiceHandler(std::unique_ptr<ServiceHandlerInterface> service_handler);

    void HandleRequest(const Request &current_request,
                       Response &current_reply,
                       const boost::asio::ip::address &remote_address);

  private:
    std::unique_ptr<ServiceHandlerInterface> service_handler;
};
} // namespace osrm::server

#endif // REQUEST_HANDLER_HPP
