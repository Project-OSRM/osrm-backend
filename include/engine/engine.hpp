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
class TablePlugin;
}
namespace datafacade
{
class BaseDataFacade;
}

class Engine final
{
  public:
    // Needs to be public
    struct EngineLock;

    explicit Engine(EngineConfig &config);
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;
    // Needed because we have unique_ptr of incomplete types
    ~Engine();

    Status Route(const api::RouteParameters &parameters, util::json::Object &result);
    Status Table(const api::TableParameters &parameters, util::json::Object &result);

  private:
    std::unique_ptr<EngineLock> lock;

    std::unique_ptr<plugins::ViaRoutePlugin> route_plugin;
    std::unique_ptr<plugins::TablePlugin> table_plugin;

    std::unique_ptr<datafacade::BaseDataFacade> query_data_facade;
};
}
}

#endif // OSRM_IMPL_HPP
