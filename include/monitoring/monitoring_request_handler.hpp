#ifndef MONITORING_REQUEST_HANDLER_HPP
#define MONITORING_REQUEST_HANDLER_HPP

#include "server/request_handler.hpp"

#include <string>

namespace osrm
{
namespace server
{

class MonitoringRequestHandler : public RequestHandler
{
  public:
    MonitoringRequestHandler(const unsigned _working_threads) : working_threads(_working_threads) {}

    void HandleRequest(const http::request &, http::reply &current_reply) override;

  private:
    unsigned working_threads;
};
} // namespace server
} // namespace osrm

#endif // MONITORING_REQUEST_HANDLER_HPP
