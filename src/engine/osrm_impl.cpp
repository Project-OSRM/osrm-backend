#include "engine/osrm_impl.hpp"

#include "engine/plugins/distance_table.hpp"
#include "engine/plugins/hello_world.hpp"
#include "engine/plugins/nearest.hpp"
#include "engine/plugins/timestamp.hpp"
#include "engine/plugins/trip.hpp"
#include "engine/plugins/viaroute.hpp"
#include "engine/plugins/match.hpp"
#include "engine/datafacade/datafacade_base.hpp"
#include "engine/datafacade/internal_datafacade.hpp"
#include "engine/datafacade/shared_barriers.hpp"
#include "engine/datafacade/shared_datafacade.hpp"
#include "util/make_unique.hpp"
#include "util/routed_options.hpp"
#include "util/simple_logger.hpp"

#include <boost/assert.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "osrm/libosrm_config.hpp"
#include "osrm/osrm.hpp"
#include "osrm/route_parameters.hpp"

#include <algorithm>
#include <fstream>
#include <utility>
#include <vector>

OSRM::OSRM_impl::OSRM_impl(LibOSRMConfig &lib_config)
{
    if (lib_config.use_shared_memory)
    {
        barrier = osrm::make_unique<SharedBarriers>();
        query_data_facade = new SharedDataFacade<QueryEdge::EdgeData>();
    }
    else
    {
        // populate base path
        populate_base_path(lib_config.server_paths);
        query_data_facade = new InternalDataFacade<QueryEdge::EdgeData>(lib_config.server_paths);
    }

    // The following plugins handle all requests.
    RegisterPlugin(new DistanceTablePlugin<BaseDataFacade<QueryEdge::EdgeData>>(
        query_data_facade, lib_config.max_locations_distance_table));
    RegisterPlugin(new HelloWorldPlugin());
    RegisterPlugin(new NearestPlugin<BaseDataFacade<QueryEdge::EdgeData>>(query_data_facade));
    RegisterPlugin(new MapMatchingPlugin<BaseDataFacade<QueryEdge::EdgeData>>(
        query_data_facade, lib_config.max_locations_map_matching));
    RegisterPlugin(new TimestampPlugin<BaseDataFacade<QueryEdge::EdgeData>>(query_data_facade));
    RegisterPlugin(new ViaRoutePlugin<BaseDataFacade<QueryEdge::EdgeData>>(
        query_data_facade, lib_config.max_locations_viaroute));
    RegisterPlugin(new RoundTripPlugin<BaseDataFacade<QueryEdge::EdgeData>>(
        query_data_facade, lib_config.max_locations_trip));
}

void OSRM::OSRM_impl::RegisterPlugin(BasePlugin *raw_plugin_ptr)
{
    std::unique_ptr<BasePlugin> plugin_ptr(raw_plugin_ptr);
    SimpleLogger().Write() << "loaded plugin: " << plugin_ptr->GetDescriptor();
    plugin_map[plugin_ptr->GetDescriptor()] = std::move(plugin_ptr);
}

int OSRM::OSRM_impl::RunQuery(const RouteParameters &route_parameters,
                              osrm::json::Object &json_result)
{
    const auto &plugin_iterator = plugin_map.find(route_parameters.service);

    if (plugin_map.end() == plugin_iterator)
    {
        json_result.values["status_message"] = "Service not found";
        return 400;
    }

    increase_concurrent_query_count();
    auto return_code = plugin_iterator->second->HandleRequest(route_parameters, json_result);
    decrease_concurrent_query_count();
    return static_cast<int>(return_code);
}

// decrease number of concurrent queries
void OSRM::OSRM_impl::decrease_concurrent_query_count()
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
void OSRM::OSRM_impl::increase_concurrent_query_count()
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

    (static_cast<SharedDataFacade<QueryEdge::EdgeData> *>(query_data_facade))
        ->CheckAndReloadFacade();
}

// proxy code for compilation firewall
OSRM::OSRM(LibOSRMConfig &lib_config) : OSRM_pimpl_(osrm::make_unique<OSRM_impl>(lib_config)) {}

// needed because unique_ptr needs the size of OSRM_impl for delete
OSRM::~OSRM() {}

int OSRM::RunQuery(const RouteParameters &route_parameters, osrm::json::Object &json_result)
{
    return OSRM_pimpl_->RunQuery(route_parameters, json_result);
}
