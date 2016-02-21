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

// Fwd decls
namespace engine
{
struct EngineConfig;
namespace api
{
struct RouteParameters;
struct TableParameters;
struct NearestParameters;
struct TripParameters;
struct MatchParameters;
}
namespace plugins
{
class ViaRoutePlugin;
class TablePlugin;
class NearestPlugin;
class TripPlugin;
class MatchPlugin;
}
// End fwd decls

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

    Engine(Engine &&) noexcept;
    Engine &operator=(Engine &&) noexcept;

    // Impl. in cpp since for unique_ptr of incomplete types
    ~Engine();

    Status Route(const api::RouteParameters &parameters, util::json::Object &result);
    Status Table(const api::TableParameters &parameters, util::json::Object &result);
    Status Nearest(const api::NearestParameters &parameters, util::json::Object &result);
    Status Trip(const api::TripParameters &parameters, util::json::Object &result);
    Status Match(const api::MatchParameters &parameters, util::json::Object &result);

  private:
    std::unique_ptr<EngineLock> lock;

    std::unique_ptr<plugins::ViaRoutePlugin> route_plugin;
    std::unique_ptr<plugins::TablePlugin> table_plugin;
    std::unique_ptr<plugins::NearestPlugin> nearest_plugin;
    // std::unique_ptr<plugins::TripPlugin> trip_plugin;
    std::unique_ptr<plugins::MatchPlugin> match_plugin;

    std::unique_ptr<datafacade::BaseDataFacade> query_data_facade;
};
}
}

#endif // OSRM_IMPL_HPP
