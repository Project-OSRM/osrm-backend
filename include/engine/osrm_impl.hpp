#ifndef OSRM_IMPL_HPP
#define OSRM_IMPL_HPP

class BasePlugin;
struct RouteParameters;

#include "contractor/query_edge.hpp"

#include "osrm/json_container.hpp"
#include "osrm/libosrm_config.hpp"
#include "osrm/osrm.hpp"

#include <memory>
#include <unordered_map>
#include <string>

struct SharedBarriers;
template <class EdgeDataT> class BaseDataFacade;

class OSRM::OSRM_impl final
{
  private:
    using PluginMap = std::unordered_map<std::string, std::unique_ptr<BasePlugin>>;

  public:
    OSRM_impl(LibOSRMConfig &lib_config);
    OSRM_impl(const OSRM_impl &) = delete;
    int RunQuery(const RouteParameters &route_parameters, osrm::json::Object &json_result);

  private:
    void RegisterPlugin(BasePlugin *plugin);
    PluginMap plugin_map;
    // will only be initialized if shared memory is used
    std::unique_ptr<SharedBarriers> barrier;
    // base class pointer to the objects
    BaseDataFacade<QueryEdge::EdgeData> *query_data_facade;

    // decrease number of concurrent queries
    void decrease_concurrent_query_count();
    // increase number of concurrent queries
    void increase_concurrent_query_count();
};

#endif // OSRM_IMPL_HPP
