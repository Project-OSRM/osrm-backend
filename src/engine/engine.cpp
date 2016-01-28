#include "engine/engine.hpp"
#include "engine/engine_config.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/status.hpp"

//#include "engine/plugins/distance_table.hpp"
//#include "engine/plugins/hello_world.hpp"
//#include "engine/plugins/nearest.hpp"
//#include "engine/plugins/timestamp.hpp"
//#include "engine/plugins/trip.hpp"
#include "engine/plugins/viaroute.hpp"
//#include "engine/plugins/match.hpp"

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

// Abstracted away the query locking into a template function
// Works the same for every plugin.
template<typename ParameterT, typename PluginT>
Status RunQuery(const std::unique_ptr<Engine::EngineLock> &lock,
                datafacade::BaseDataFacade &facade,
                const ParameterT &parameters,
                PluginT &plugin,
                util::json::Object &json_result)
{
    if (!lock)
    {
        return plugin.HandleRequest(parameters, json_result);
    }

    BOOST_ASSERT(lock);
    lock->IncreaseQueryCount();

    auto& shared_facade = static_cast<datafacade::SharedDataFacade &>(facade);
    shared_facade.CheckAndReloadFacade();
    // Get a shared data lock so that other threads won't update
    // things while the query is running
    boost::shared_lock<boost::shared_mutex> data_lock{shared_facade.data_mutex};

    Status status = plugin.HandleRequest(parameters, json_result);

    lock->DecreaseQueryCount();
    return status;
}


Engine::Engine(EngineConfig &config)
{
    if (config.use_shared_memory)
    {
        lock = util::make_unique<EngineLock>();
        query_data_facade = util::make_unique<datafacade::SharedDataFacade>();
    }
    else
    {
        // populate base path
        util::populate_base_path(config.server_paths);
        query_data_facade = util::make_unique<datafacade::InternalDataFacade>(config.server_paths);
    }

    route_plugin = util::make_unique<plugins::ViaRoutePlugin>(*query_data_facade, config.max_locations_viaroute);
}

Status Engine::Route(const api::RouteParameters &route_parameters,
                   util::json::Object &result)
{
    return RunQuery(lock, *query_data_facade, route_parameters, *route_plugin, result);
}

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

}
}
