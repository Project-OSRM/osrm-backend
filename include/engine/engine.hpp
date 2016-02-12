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
struct TableParameters;
}
namespace plugins
{
class ViaRoutePlugin;
class DistanceTablePlugin;
}
namespace datafacade
{
class BaseDataFacade;
}

class Engine final
{
  public:
    Engine(EngineConfig &config);
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    Status Route(const api::RouteParameters &parameters, util::json::Object &result);
    Status Table(const api::TableParameters &parameters, util::json::Object &result);

  private:
    struct EngineLock;
    std::unique_ptr<EngineLock> lock;

    std::unique_ptr<plugins::ViaRoutePlugin> route_plugin;
    std::unique_ptr<plugins::DistanceTablePlugin> table_plugin;

    std::unique_ptr<datafacade::BaseDataFacade> query_data_facade;
};
}
}

#endif // OSRM_IMPL_HPP
