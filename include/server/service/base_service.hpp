#ifndef SERVER_SERVICE_BASE_SERVICE_HPP
#define SERVER_SERVICE_BASE_SERVICE_HPP

#include "engine/status.hpp"
#include "osrm/osrm.hpp"
#include "util/coordinate.hpp"

#include <mapbox/variant.hpp>

#include <atomic>
#include <string>
#include <vector>

namespace osrm
{
namespace server
{
namespace service
{

class BaseService
{
  public:
    BaseService(OSRM &routing_machine) : routing_machine(routing_machine), usage(0) {}
    virtual ~BaseService() = default;

    virtual engine::Status
    RunQuery(std::size_t prefix_length, std::string &query, osrm::engine::api::ResultT &result) = 0;

    virtual unsigned GetVersion() = 0;
    uint32_t GetUsage() {return usage;}

  protected:
    OSRM &routing_machine;
    std::atomic_uint usage;
};
}
}
}

#endif
