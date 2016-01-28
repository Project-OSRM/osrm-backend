#ifndef SERVER_SERVICE_ROUTE_SERVICE_HPP
#define SERVER_SERVICE_ROUTE_SERVICE_HPP

#include "server/service/base_service.hpp"

#include "engine/status.hpp"
#include "util/coordinate.hpp"
#include "osrm/osrm.hpp"

#include <string>
#include <vector>

namespace osrm
{
namespace server
{
namespace service
{

class RouteService final : public BaseService
{
  public:
    RouteService(OSRM& routing_machine) : BaseService(routing_machine) {}

    virtual engine::Status RunQuery(std::vector<util::FixedPointCoordinate> coordinates,
                                    std::string &options,
                                    util::json::Object &json_result) final override;
    virtual unsigned GetVersion() final override { return 1; }
};
}
}
}

#endif
