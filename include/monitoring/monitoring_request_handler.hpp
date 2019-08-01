#ifndef MONITORING_REQUEST_HANDLER_HPP
#define MONITORING_REQUEST_HANDLER_HPP

#include "server/request_handler.hpp"

#include "util/log.hpp"

#include <sstream>
#include <string>

namespace osrm
{
namespace server
{

class MonitoringRequestHandler : public RequestHandler
{
  public:
    MonitoringRequestHandler(const unsigned _working_threads) : working_threads(_working_threads) {}

    void HandleRequest(const http::request &, http::reply &current_reply)
    {
        std::stringstream out_stream;
        out_stream << "osrm_routed_instance_info{";
        auto engine_info = service_handler->GetEngineInfo();
        for (auto record : engine_info)
        {
            out_stream << record.first << "=\"" << record.second << '"';
            out_stream << ',';
        }
        out_stream << "working_threads=\"" << working_threads << '"';
        out_stream << "} 1\n";

        auto counters = service_handler->GetUsage();
        for (auto counter : counters)
        {
            out_stream << "http_requests_count{plugin=\"" << counter.first << "\"} "
                       << counter.second << "\n";
        }

        out_stream << "workers_busy " << service_handler->GetLoad() << "\n";

        auto result = out_stream.str();
        current_reply.content.resize(result.size());
        std::copy(result.cbegin(), result.cend(), current_reply.content.begin());
    }

  private:
    unsigned working_threads;
};
} // namespace server
} // namespace osrm

#endif // MONITORING_REQUEST_HANDLER_HPP
