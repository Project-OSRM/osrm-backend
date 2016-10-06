#include "engine/engine.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/engine_config.hpp"
#include "engine/status.hpp"
#include "engine/data_watchdog.hpp"

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
#include "util/simple_logger.hpp"

#include <boost/assert.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/thread/lock_types.hpp>

#include <algorithm>
#include <fstream>
#include <memory>
#include <utility>
#include <vector>

namespace
{
// Abstracted away the query locking into a template function
// Works the same for every plugin.
template <typename ParameterT, typename PluginT, typename ResultT>
osrm::engine::Status
RunQuery(const std::unique_ptr<osrm::storage::SharedBarriers> &lock,
         std::mutex &facade_update_mutex,
         osrm::engine::DataWatchdog& watchdog,
         std::shared_ptr<osrm::engine::datafacade::BaseDataFacade> &facade,
         const ParameterT &parameters,
         PluginT &plugin,
         ResultT &result)
{
    if (!lock)
    {
        return plugin.HandleRequest(facade, parameters, result);
    }

    BOOST_ASSERT(lock);
    // this locks aquires shared ownership of the query mutex: other requets are allowed
    // to run, but data updates need to wait for all queries to finish until they can aquire an
    // exclusive lock
    boost::interprocess::sharable_lock<boost::interprocess::named_sharable_mutex> query_lock(
        lock->query_mutex);

    {
        // this lock ensures that we are never overtaken while creating a new
        // facade and setting it.
        // This is important since we need to ensure there is always exactly
        // one facade per shared memory region.
        // TODO: Remove this once the SharedDataFacade doesn't own the shared memory
        // segment anymore.
        std::lock_guard<std::mutex> update_lock(facade_update_mutex);

        if (watchdog.HasNewRegion())
        {

            auto new_facade = watchdog.MaybeLoadNewRegion();
            // for now the external locking will ensure that loading the new region
            // will ways work. In the future we might allow being overtaken
            // by other threads and they will also try to update.
            if (new_facade)
            {
                // TODO remove once we allow for more then one SharedMemoryFacade at the same time
                // at this point no other query is allowed to reference this facade!
                // the old facade will die exactly here
                BOOST_ASSERT(facade.use_count() == 1);
                facade = std::move(new_facade);
            }
        }
    }

    osrm::engine::Status status = plugin.HandleRequest(facade, parameters, result);

    return status;
}

} // anon. ns

namespace osrm
{
namespace engine
{

Engine::Engine(const EngineConfig &config)
    : lock(config.use_shared_memory ? std::make_unique<storage::SharedBarriers>()
                                    : std::unique_ptr<storage::SharedBarriers>())
{
    if (config.use_shared_memory)
    {
        if (!DataWatchdog::TryConnect())
        {
            throw util::exception(
                "No shared memory blocks found, have you forgotten to run osrm-datastore?");
        }

        facade_update_mutex = std::make_unique<std::mutex>();
        watchdog = std::make_unique<DataWatchdog>();
        // this will always either return a value or throw an exception
        // in the initial run
        query_data_facade = watchdog->MaybeLoadNewRegion();
        BOOST_ASSERT(query_data_facade);
        BOOST_ASSERT(watchdog);
        BOOST_ASSERT(lock);
    }
    else
    {
        if (!config.storage_config.IsValid())
        {
            throw util::exception("Invalid file paths given!");
        }
        query_data_facade = std::make_shared<datafacade::InternalDataFacade>(config.storage_config);
    }

    // Register plugins
    using namespace plugins;

    route_plugin = std::make_unique<ViaRoutePlugin>(config.max_locations_viaroute);
    table_plugin = std::make_unique<TablePlugin>(config.max_locations_distance_table);
    nearest_plugin = std::make_unique<NearestPlugin>(config.max_results_nearest);
    trip_plugin = std::make_unique<TripPlugin>(config.max_locations_trip);
    match_plugin = std::make_unique<MatchPlugin>(config.max_locations_map_matching);
    tile_plugin = std::make_unique<TilePlugin>();
}

// make sure we deallocate the unique ptr at a position where we know the size of the plugins
Engine::~Engine() = default;
Engine::Engine(Engine &&) noexcept = default;
Engine &Engine::operator=(Engine &&) noexcept = default;

Status Engine::Route(const api::RouteParameters &params, util::json::Object &result) const
{
    return RunQuery(lock, *facade_update_mutex, *watchdog, query_data_facade, params, *route_plugin, result);
}

Status Engine::Table(const api::TableParameters &params, util::json::Object &result) const
{
    return RunQuery(lock, *facade_update_mutex, *watchdog, query_data_facade, params, *table_plugin, result);
}

Status Engine::Nearest(const api::NearestParameters &params, util::json::Object &result) const
{
    return RunQuery(lock, *facade_update_mutex, *watchdog, query_data_facade, params, *nearest_plugin, result);
}

Status Engine::Trip(const api::TripParameters &params, util::json::Object &result) const
{
    return RunQuery(lock, *facade_update_mutex, *watchdog, query_data_facade, params, *trip_plugin, result);
}

Status Engine::Match(const api::MatchParameters &params, util::json::Object &result) const
{
    return RunQuery(lock, *facade_update_mutex, *watchdog, query_data_facade, params, *match_plugin, result);
}

Status Engine::Tile(const api::TileParameters &params, std::string &result) const
{
    return RunQuery(lock, *facade_update_mutex, *watchdog, query_data_facade, params, *tile_plugin, result);
}

} // engine ns
} // osrm ns
