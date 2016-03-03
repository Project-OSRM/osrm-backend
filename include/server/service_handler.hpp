#ifndef SERVER_SERVICE_HANLDER_HPP
#define SERVER_SERVICE_HANLDER_HPP

#include "server/service/base_service.hpp"

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

class ServiceHandler
{
  public:
    ServiceHandler(osrm::EngineConfig &config);
    using ResultT = service::BaseService::ResultT;

    engine::Status RunQuery(api::ParsedURL parsed_url, ResultT &result);

  private:

    std::unordered_map<std::string, std::unique_ptr<service::BaseService>> service_map;
    OSRM routing_machine;
};
}
}

#endif
