#include "engine/engine.hpp"
#include "engine/engine_config.hpp"
#include "engine/route_parameters.hpp"

#include "engine/plugins/distance_table.hpp"
#include "engine/plugins/hello_world.hpp"
#include "engine/plugins/range_analysis.hpp"
#include "engine/plugins/nearest.hpp"
#include "engine/plugins/timestamp.hpp"
#include "engine/plugins/trip.hpp"
#include "engine/plugins/viaroute.hpp"
#include "engine/plugins/match.hpp"
#include "engine/plugins/tile.hpp"

#include "engine/datafacade/datafacade_base.hpp"
#include "engine/datafacade/internal_datafacade.hpp"
#include "engine/datafacade/shared_datafacade.hpp"

#include "storage/shared_barriers.hpp"
#include "util/make_unique.hpp"
#include "util/routed_options.hpp"
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

Engine::Engine(EngineConfig &config)
{
    if (config.use_shared_memory)
    {
        barrier = util::make_unique<storage::SharedBarriers>();
        query_data_facade = new datafacade::SharedDataFacade<contractor::QueryEdge::EdgeData>();
    }
    else
    {
        // populate base path
        util::populate_base_path(config.server_paths);
        query_data_facade = new datafacade::InternalDataFacade<contractor::QueryEdge::EdgeData>(
            config.server_paths);
    }

    using DataFacade = datafacade::BaseDataFacade<contractor::QueryEdge::EdgeData>;

    // The following plugins handle all requests.
    RegisterPlugin(new plugins::DistanceTablePlugin<DataFacade>(
        query_data_facade, config.max_locations_distance_table));
    RegisterPlugin(new plugins::HelloWorldPlugin());
    RegisterPlugin(new plugins::NearestPlugin<DataFacade>(query_data_facade));
    RegisterPlugin(new plugins::MapMatchingPlugin<DataFacade>(query_data_facade,
                                                              config.max_locations_map_matching));
    RegisterPlugin(new plugins::TimestampPlugin<DataFacade>(query_data_facade));
    RegisterPlugin(
        new plugins::ViaRoutePlugin<DataFacade>(query_data_facade, config.max_locations_viaroute));
    RegisterPlugin(
        new plugins::RoundTripPlugin<DataFacade>(query_data_facade, config.max_locations_trip));
    RegisterPlugin(new plugins::TilePlugin<DataFacade>(query_data_facade));
    RegisterPlugin(new plugins::RangeAnalysis<DataFacade>(query_data_facade));
}

void Engine::RegisterPlugin(plugins::BasePlugin *raw_plugin_ptr)
{
    std::unique_ptr<plugins::BasePlugin> plugin_ptr(raw_plugin_ptr);
    util::SimpleLogger().Write() << "loaded plugin: " << plugin_ptr->GetDescriptor();
    plugin_map[plugin_ptr->GetDescriptor()] = std::move(plugin_ptr);
}

int Engine::RunQuery(const RouteParameters &route_parameters, util::json::Object &json_result)
{
    const auto &plugin_iterator = plugin_map.find(route_parameters.service);

    if (plugin_map.end() == plugin_iterator)
    {
        json_result.values["status_message"] = "Service not found";
        return 400;
    }

    osrm::engine::plugins::BasePlugin::Status return_code;
    increase_concurrent_query_count();
    if (barrier)
    {
        // Get a shared data lock so that other threads won't update
        // things while the query is running
        boost::shared_lock<boost::shared_mutex> data_lock{
            (static_cast<datafacade::SharedDataFacade<contractor::QueryEdge::EdgeData> *>(
                 query_data_facade))->data_mutex};
        return_code = plugin_iterator->second->HandleRequest(route_parameters, json_result);
    }
    else
    {
        return_code = plugin_iterator->second->HandleRequest(route_parameters, json_result);
    }
    decrease_concurrent_query_count();
    return static_cast<int>(return_code);
}

// decrease number of concurrent queries
void Engine::decrease_concurrent_query_count()
{
    if (!barrier)
    {
        return;
    }
    // lock query
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> query_lock(
        barrier->query_mutex);

    // decrement query count
    --(barrier->number_of_queries);
    BOOST_ASSERT_MSG(0 <= barrier->number_of_queries, "invalid number of queries");

    // notify all processes that were waiting for this condition
    if (0 == barrier->number_of_queries)
    {
        barrier->no_running_queries_condition.notify_all();
    }
}

// increase number of concurrent queries
void Engine::increase_concurrent_query_count()
{
    if (!barrier)
    {
        return;
    }

    // lock update pending
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> pending_lock(
        barrier->pending_update_mutex);

    // lock query
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> query_lock(
        barrier->query_mutex);

    // unlock update pending
    pending_lock.unlock();

    // increment query count
    ++(barrier->number_of_queries);

    (static_cast<datafacade::SharedDataFacade<contractor::QueryEdge::EdgeData> *>(
         query_data_facade))->CheckAndReloadFacade();
}
}
}
