#ifndef SERVER_SERVICE_BASE_SERVICE_HPP
#define SERVER_SERVICE_BASE_SERVICE_HPP

#include "engine/status.hpp"
#include "osrm/osrm.hpp"
#include "util/coordinate.hpp"
#include "util/json_container.hpp"

#include <variant>

#include <string>

namespace osrm::server::service
{

class BaseService
{
  public:
    BaseService(OSRM &routing_machine) : routing_machine(routing_machine) {}
    virtual ~BaseService() = default;

    // GET path: parameters are parsed from the URL query string.
    virtual engine::Status
    RunQuery(std::size_t prefix_length, std::string &query, osrm::engine::api::ResultT &result) = 0;

    // POST path: parameters are parsed from a JSON request body. Services that do not
    // support POST inherit the default implementation below, which reports an error.
    virtual engine::Status RunJSONQuery(const std::string &json_body,
                                        osrm::engine::api::ResultT &result)
    {
        (void)json_body;
        result = util::json::Object();
        auto &json_result = std::get<util::json::Object>(result);
        json_result.values["code"] = "NotImplemented";
        json_result.values["message"] =
            "This service does not support POST requests with a JSON body.";
        return engine::Status::Error;
    }

    virtual unsigned GetVersion() = 0;

  protected:
    OSRM &routing_machine;
};
} // namespace osrm::server::service

#endif
