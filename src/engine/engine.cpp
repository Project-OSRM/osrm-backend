#include "engine/engine.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/engine_config.hpp"
#include "engine/status.hpp"

#include "engine/datafacade/contiguous_internalmem_datafacade.hpp"
#include "engine/datafacade/process_memory_allocator.hpp"

#include "util/log.hpp"

#include <boost/assert.hpp>

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
RunQuery(const std::unique_ptr<osrm::engine::DataWatchdog> &watchdog,
         const std::shared_ptr<const osrm::engine::datafacade::BaseDataFacade> &immutable_facade,
         const ParameterT &parameters,
         PluginT &plugin,
         ResultT &result)
{
    if (watchdog)
    {
        BOOST_ASSERT(!immutable_facade);
        auto facade = watchdog->GetDataFacade();

        return plugin.HandleRequest(facade, parameters, result);
    }

    BOOST_ASSERT(immutable_facade);

    return plugin.HandleRequest(immutable_facade, parameters, result);
}

} // anon. ns

namespace osrm
{
namespace engine
{

Engine::Engine(const EngineConfig &config)
    : route_plugin(config.max_locations_viaroute),       //
      table_plugin(config.max_locations_distance_table), //
      nearest_plugin(config.max_results_nearest),        //
      trip_plugin(config.max_locations_trip),            //
      match_plugin(config.max_locations_map_matching),   //
      tile_plugin()                                      //

{
    if (config.use_shared_memory)
    {
        watchdog = std::make_unique<DataWatchdog>();
        BOOST_ASSERT(watchdog);
    }
    else
    {
        if (!config.storage_config.IsValid())
        {
            throw util::exception("Invalid file paths given!" + SOURCE_REF);
        }
        auto allocator =
            std::make_unique<datafacade::ProcessMemoryAllocator>(config.storage_config);
        immutable_data_facade =
            std::make_shared<const datafacade::ContiguousInternalMemoryDataFacade>(
                std::move(allocator));
    }
}

Status Engine::Route(const api::RouteParameters &params, util::json::Object &result) const
{
    return RunQuery(watchdog, immutable_data_facade, params, route_plugin, result);
}

Status Engine::Table(const api::TableParameters &params, util::json::Object &result) const
{
    return RunQuery(watchdog, immutable_data_facade, params, table_plugin, result);
}

Status Engine::Nearest(const api::NearestParameters &params, util::json::Object &result) const
{
    return RunQuery(watchdog, immutable_data_facade, params, nearest_plugin, result);
}

Status Engine::Trip(const api::TripParameters &params, util::json::Object &result) const
{
    return RunQuery(watchdog, immutable_data_facade, params, trip_plugin, result);
}

Status Engine::Match(const api::MatchParameters &params, util::json::Object &result) const
{
    return RunQuery(watchdog, immutable_data_facade, params, match_plugin, result);
}

Status Engine::Tile(const api::TileParameters &params, std::string &result) const
{
    return RunQuery(watchdog, immutable_data_facade, params, tile_plugin, result);
}

} // engine ns
} // osrm ns
