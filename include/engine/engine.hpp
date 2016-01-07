#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "contractor/query_edge.hpp"

#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"

#include <memory>
#include <unordered_map>
#include <string>

namespace osrm
{

namespace storage
{
struct SharedBarriers;
}

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
struct RouteParameters;
namespace plugins
{
class BasePlugin;
}
namespace datafacade
{
template <class EdgeDataT> class BaseDataFacade;
}

class Engine final
{
  private:
    using PluginMap = std::unordered_map<std::string, std::unique_ptr<plugins::BasePlugin>>;

  public:
    Engine(EngineConfig &config_);
    Engine(const Engine &) = delete;
    int RunQuery(const RouteParameters &route_parameters, util::json::Object &json_result);

  private:
    void RegisterPlugin(plugins::BasePlugin *plugin);
    PluginMap plugin_map;
    // will only be initialized if shared memory is used
    std::unique_ptr<storage::SharedBarriers> barrier;
    // base class pointer to the objects
    datafacade::BaseDataFacade<contractor::QueryEdge::EdgeData> *query_data_facade;

    // decrease number of concurrent queries
    void decrease_concurrent_query_count();
    // increase number of concurrent queries
    void increase_concurrent_query_count();
};
}
}

#endif // OSRM_IMPL_HPP
