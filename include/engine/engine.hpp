#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "engine/status.hpp"
#include "storage/shared_barriers.hpp"
#include "util/json_container.hpp"

#include <memory>
#include <unordered_map>
#include <string>

namespace osrm
{

namespace util
{
namespace json
{
struct Object;
}
}

namespace engine
{
struct EngineConfig;
namespace api
{
struct RouteParameters;
}
namespace plugins
{
class ViaRoutePlugin;
}
namespace datafacade
{
class BaseDataFacade;
}

class Engine final
{
  public:
    struct EngineLock
    {
        // will only be initialized if shared memory is used
        storage::SharedBarriers barrier;
        // decrease number of concurrent queries
        void DecreaseQueryCount();
        // increase number of concurrent queries
        void IncreaseQueryCount();
    };

    Engine(EngineConfig &config_);

    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    Status Route(const api::RouteParameters &route_parameters, util::json::Object &json_result);

  private:
    std::unique_ptr<EngineLock> lock;
    std::unique_ptr<plugins::ViaRoutePlugin> route_plugin;
    std::unique_ptr<datafacade::BaseDataFacade> query_data_facade;
};
}
}

#endif // OSRM_IMPL_HPP
