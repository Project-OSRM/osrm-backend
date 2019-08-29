#ifndef SERVER_SERVICE_BASE_SERVICE_HPP
#define SERVER_SERVICE_BASE_SERVICE_HPP

#include "engine/status.hpp"
#include "osrm/osrm.hpp"
#include "util/coordinate.hpp"

#include <mapbox/variant.hpp>

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
    BaseService(OSRM &routing_machine) : routing_machine(routing_machine) {}
    virtual ~BaseService() = default;

    virtual engine::Status
    RunQuery(std::size_t prefix_length, std::string &query, osrm::engine::api::ResultT &result) = 0;

    virtual unsigned GetVersion() = 0;

  protected:
    OSRM &routing_machine;
};
}
}
}

#endif
