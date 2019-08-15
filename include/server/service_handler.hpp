#ifndef SERVER_SERVICE_HANLDER_HPP
#define SERVER_SERVICE_HANLDER_HPP

#include "server/service/base_service.hpp"

#include "engine/api/base_api.hpp"
#include "osrm/osrm.hpp"

#include <unordered_map>

namespace osrm
{
namespace util
{
namespace json
{
struct Object;
}
}
namespace server
{
namespace api
{
struct ParsedURL;
}

class ServiceHandlerInterface
{
  public:
    virtual ~ServiceHandlerInterface() {}
    virtual engine::Status RunQuery(api::ParsedURL parsed_url,
                                    osrm::engine::api::ResultT &result) = 0;
};

class ServiceHandler final : public ServiceHandlerInterface
{
  public:
    ServiceHandler(osrm::EngineConfig &config);
    using ResultT = osrm::engine::api::ResultT;

    virtual engine::Status RunQuery(api::ParsedURL parsed_url, ResultT &result) override;

  private:
    std::unordered_map<std::string, std::unique_ptr<service::BaseService>> service_map;
    OSRM routing_machine;
};
}
}

#endif
