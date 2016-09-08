#include "engine/engine.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/engine_config.hpp"
#include "engine/status.hpp"

#include "engine/plugins/match.hpp"
#include "engine/plugins/nearest.hpp"
#include "engine/plugins/table.hpp"
#include "engine/plugins/tile.hpp"
#include "engine/plugins/trip.hpp"
#include "engine/plugins/viaroute.hpp"

#include "engine/datafacade/datafacade_base.hpp"
#include "engine/datafacade/internal_datafacade.hpp"
#include "engine/datafacade/shared_datafacade.hpp"

#include "storage/shared_barriers.hpp"
#include "util/make_unique.hpp"
#include "util/simple_logger.hpp"

#include <boost/assert.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/thread/lock_types.hpp>

#include <algorithm>
#include <fstream>
#include <utility>
#include <vector>

namespace osrm
{
namespace engine
{
struct Engine::EngineLock
{
    // will only be initialized if shared memory is used
    storage::SharedBarriers barrier;
    // decrease number of concurrent queries
    void DecreaseQueryCount();
    // increase number of concurrent queries
    void IncreaseQueryCount();
};

// decrease number of concurrent queries
void Engine::EngineLock::DecreaseQueryCount()
{
    // lock query
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> query_lock(
        barrier.query_mutex);

    // decrement query count
    --(barrier.number_of_queries);
    BOOST_ASSERT_MSG(0 <= barrier.number_of_queries, "invalid number of queries");

    // notify all processes that were waiting for this condition
    if (0 == barrier.number_of_queries)
    {
        barrier.no_running_queries_condition.notify_all();
    }
}

// increase number of concurrent queries
void Engine::EngineLock::IncreaseQueryCount()
{
    // lock update pending
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> pending_lock(
        barrier.pending_update_mutex);

    // lock query
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> query_lock(
        barrier.query_mutex);

    // unlock update pending
    pending_lock.unlock();

    // increment query count
    ++(barrier.number_of_queries);
}
} // ns engine
} // ns osrm

namespace
{
// Abstracted away the query locking into a template function
// Works the same for every plugin.
template <typename ParameterT, typename PluginT, typename ResultT>
osrm::engine::Status RunQuery(const std::unique_ptr<osrm::engine::Engine::EngineLock> &lock,
                              osrm::engine::datafacade::BaseDataFacade &facade,
                              const ParameterT &parameters,
                              PluginT &plugin,
                              ResultT &result)
{
    if (!lock)
    {
        return plugin.HandleRequest(parameters, result);
    }

    BOOST_ASSERT(lock);
    lock->IncreaseQueryCount();

    auto &shared_facade = static_cast<osrm::engine::datafacade::SharedDataFacade &>(facade);
    shared_facade.CheckAndReloadFacade();
    // Get a shared data lock so that other threads won't update
    // things while the query is running
    boost::shared_lock<boost::shared_mutex> data_lock{shared_facade.data_mutex};

    osrm::engine::Status status = plugin.HandleRequest(parameters, result);

    lock->DecreaseQueryCount();
    return status;
}

template <typename Plugin, typename Facade, typename... Args>
std::unique_ptr<Plugin> create(Facade &facade, Args... args)
{
    return osrm::util::make_unique<Plugin>(facade, std::forward<Args>(args)...);
}

} // anon. ns

namespace osrm
{
namespace engine
{

Engine::Engine(EngineConfig &config)
{
    if (config.use_shared_memory)
    {
        lock = util::make_unique<EngineLock>();
        query_data_facade = util::make_unique<datafacade::SharedDataFacade>();
    }
    else
    {
        if (!config.storage_config.IsValid())
        {
            throw util::exception("Invalid file paths given!");
        }
        query_data_facade =
            util::make_unique<datafacade::InternalDataFacade>(config.storage_config);
    }

    // Register plugins
    using namespace plugins;

    route_plugin = create<ViaRoutePlugin>(*query_data_facade, config.max_locations_viaroute, config.max_radius_when_bearings);
    table_plugin = create<TablePlugin>(*query_data_facade, config.max_locations_distance_table, config.max_radius_when_bearings);
    nearest_plugin = create<NearestPlugin>(*query_data_facade, config.max_results_nearest, config.max_radius_when_bearings);
    trip_plugin = create<TripPlugin>(*query_data_facade, config.max_locations_trip, config.max_radius_when_bearings);
    match_plugin = create<MatchPlugin>(*query_data_facade, config.max_locations_map_matching, config.max_radius_when_bearings);
    tile_plugin = create<TilePlugin>(*query_data_facade);
}

// make sure we deallocate the unique ptr at a position where we know the size of the plugins
Engine::~Engine() = default;
Engine::Engine(Engine &&) noexcept = default;
Engine &Engine::operator=(Engine &&) noexcept = default;

Status Engine::Route(const api::RouteParameters &params, util::json::Object &result)
{
    return RunQuery(lock, *query_data_facade, params, *route_plugin, result);
}

Status Engine::Table(const api::TableParameters &params, util::json::Object &result)
{
    return RunQuery(lock, *query_data_facade, params, *table_plugin, result);
}

Status Engine::Nearest(const api::NearestParameters &params, util::json::Object &result)
{
    return RunQuery(lock, *query_data_facade, params, *nearest_plugin, result);
}

Status Engine::Trip(const api::TripParameters &params, util::json::Object &result)
{
    return RunQuery(lock, *query_data_facade, params, *trip_plugin, result);
}

Status Engine::Match(const api::MatchParameters &params, util::json::Object &result)
{
    return RunQuery(lock, *query_data_facade, params, *match_plugin, result);
}

Status Engine::Tile(const api::TileParameters &params, std::string &result)
{
    return RunQuery(lock, *query_data_facade, params, *tile_plugin, result);
}

} // engine ns
} // osrm ns
