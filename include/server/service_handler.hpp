#ifndef SERVER_SERVICE_HANLDER_HPP
#define SERVER_SERVICE_HANLDER_HPP

#include "server/service/base_service.hpp"

#include "engine/api/base_api.hpp"
#include "engine/engine_info.hpp"
#include "osrm/osrm.hpp"

#include <atomic>
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

using HandlersCounter = std::unordered_map<std::string, std::uint32_t>;

class ServiceHandlerInterface
{
  public:
    virtual ~ServiceHandlerInterface() {}
    virtual engine::Status RunQuery(api::ParsedURL parsed_url,
                                    osrm::engine::api::ResultT &result) = 0;
    virtual const engine::EngineInfo & GetEngineInfo() const = 0;
    virtual const HandlersCounter GetUsage() const = 0;
    virtual std::uint32_t GetLoad() const = 0;
};

class ServiceHandler final : public ServiceHandlerInterface
{
  public:
    ServiceHandler(osrm::EngineConfig &config);
    using ResultT = osrm::engine::api::ResultT;

    virtual engine::Status RunQuery(api::ParsedURL parsed_url, ResultT &result) override;

    virtual const engine::EngineInfo &GetEngineInfo() const override;
    virtual const HandlersCounter GetUsage() const override;
    virtual std::uint32_t GetLoad() const override;

  private:
    std::unordered_map<std::string, std::unique_ptr<service::BaseService>> service_map;
    OSRM routing_machine;
    std::atomic_uint errors;
    std::atomic_uint load;
};
}
}

#endif
