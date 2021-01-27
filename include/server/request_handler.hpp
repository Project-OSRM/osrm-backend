#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include "server/service_handler.hpp"

#include <string>

namespace osrm
{
namespace server
{

namespace http
{
class reply;
struct request;
} // namespace http

class RequestHandler
{

  public:
    RequestHandler() = default;
    RequestHandler(const RequestHandler &) = delete;
    RequestHandler &operator=(const RequestHandler &) = delete;

    void RegisterServiceHandler(std::unique_ptr<ServiceHandlerInterface> service_handler);

    void HandleRequest(const http::request &current_request, http::reply &current_reply);

  private:
    std::unique_ptr<ServiceHandlerInterface> service_handler;
};
} // namespace server
} // namespace osrm

#endif // REQUEST_HANDLER_HPP
