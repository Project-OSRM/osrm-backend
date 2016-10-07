#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "engine/status.hpp"
#include "util/json_container.hpp"

#include <memory>
#include <mutex>
#include <string>
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

namespace storage
{
struct SharedBarriers;
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
struct TileParameters;
}
namespace plugins
{
class ViaRoutePlugin;
class TablePlugin;
class NearestPlugin;
class TripPlugin;
class MatchPlugin;
class TilePlugin;
}
// End fwd decls

namespace datafacade
{
class BaseDataFacade;
}

class DataWatchdog;

class Engine final
{
  public:
    explicit Engine(const EngineConfig &config);

    Engine(Engine &&) noexcept;
    Engine &operator=(Engine &&) noexcept;

    // Impl. in cpp since for unique_ptr of incomplete types
    ~Engine();

    Status Route(const api::RouteParameters &parameters, util::json::Object &result) const;
    Status Table(const api::TableParameters &parameters, util::json::Object &result) const;
    Status Nearest(const api::NearestParameters &parameters, util::json::Object &result) const;
    Status Trip(const api::TripParameters &parameters, util::json::Object &result) const;
    Status Match(const api::MatchParameters &parameters, util::json::Object &result) const;
    Status Tile(const api::TileParameters &parameters, std::string &result) const;

  private:
    std::unique_ptr<storage::SharedBarriers> lock;
    std::unique_ptr<DataWatchdog> watchdog;

    std::unique_ptr<plugins::ViaRoutePlugin> route_plugin;
    std::unique_ptr<plugins::TablePlugin> table_plugin;
    std::unique_ptr<plugins::NearestPlugin> nearest_plugin;
    std::unique_ptr<plugins::TripPlugin> trip_plugin;
    std::unique_ptr<plugins::MatchPlugin> match_plugin;
    std::unique_ptr<plugins::TilePlugin> tile_plugin;

    // reading and setting this is protected by locking in the watchdog
    mutable std::shared_ptr<datafacade::BaseDataFacade> query_data_facade;
};
}
}

#endif // OSRM_IMPL_HPP
