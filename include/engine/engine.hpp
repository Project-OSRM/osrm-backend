#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "engine/api/match_parameters.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/api/tile_parameters.hpp"
#include "engine/api/trip_parameters.hpp"
#include "engine/data_watchdog.hpp"
#include "engine/datafacade/contiguous_block_allocator.hpp"
#include "engine/datafacade_provider.hpp"
#include "engine/engine_config.hpp"
#include "engine/engine_config.hpp"
#include "engine/plugins/match.hpp"
#include "engine/plugins/nearest.hpp"
#include "engine/plugins/table.hpp"
#include "engine/plugins/tile.hpp"
#include "engine/plugins/trip.hpp"
#include "engine/plugins/viaroute.hpp"
#include "engine/routing_algorithms.hpp"
#include "engine/status.hpp"
#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/json_container.hpp"

#include <memory>
#include <string>

namespace osrm
{
namespace engine
{

class EngineInterface
{
  public:
    virtual ~EngineInterface() = default;
    virtual Status Route(const api::RouteParameters &parameters,
                         util::json::Object &result) const = 0;
    virtual Status Table(const api::TableParameters &parameters,
                         util::json::Object &result) const = 0;
    virtual Status Nearest(const api::NearestParameters &parameters,
                           util::json::Object &result) const = 0;
    virtual Status Trip(const api::TripParameters &parameters,
                        util::json::Object &result) const = 0;
    virtual Status Match(const api::MatchParameters &parameters,
                         util::json::Object &result) const = 0;
    virtual Status Tile(const api::TileParameters &parameters, std::string &result) const = 0;
};

template <typename Algorithm> class Engine final : public EngineInterface
{
  public:
    explicit Engine(const EngineConfig &config)
        : route_plugin(config.max_locations_viaroute),       //
          table_plugin(config.max_locations_distance_table), //
          nearest_plugin(config.max_results_nearest),        //
          trip_plugin(config.max_locations_trip),            //
          match_plugin(config.max_locations_map_matching),   //
          tile_plugin()                                      //

    {
        if (config.use_shared_memory)
        {
            util::Log(logDEBUG) << "Using shared memory with algorithm "
                                << routing_algorithms::name<Algorithm>();
            facade_provider = std::make_unique<WatchingProvider<Algorithm>>();
        }
        else
        {
            util::Log(logDEBUG) << "Using internal memory with algorithm "
                                << routing_algorithms::name<Algorithm>();
            facade_provider = std::make_unique<ImmutableProvider<Algorithm>>(config.storage_config);
        }
    }

    Engine(Engine &&) noexcept = delete;
    Engine &operator=(Engine &&) noexcept = delete;

    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;
    virtual ~Engine() = default;

    Status Route(const api::RouteParameters &params,
                 util::json::Object &result) const override final
    {
        auto facade = facade_provider->Get();
        auto algorithms = RoutingAlgorithms<Algorithm>{heaps, *facade};
        return route_plugin.HandleRequest(*facade, algorithms, params, result);
    }

    Status Table(const api::TableParameters &params,
                 util::json::Object &result) const override final
    {
        auto facade = facade_provider->Get();
        auto algorithms = RoutingAlgorithms<Algorithm>{heaps, *facade};
        return table_plugin.HandleRequest(*facade, algorithms, params, result);
    }

    Status Nearest(const api::NearestParameters &params,
                   util::json::Object &result) const override final
    {
        auto facade = facade_provider->Get();
        auto algorithms = RoutingAlgorithms<Algorithm>{heaps, *facade};
        return nearest_plugin.HandleRequest(*facade, algorithms, params, result);
    }

    Status Trip(const api::TripParameters &params, util::json::Object &result) const override final
    {
        auto facade = facade_provider->Get();
        auto algorithms = RoutingAlgorithms<Algorithm>{heaps, *facade};
        return trip_plugin.HandleRequest(*facade, algorithms, params, result);
    }

    Status Match(const api::MatchParameters &params,
                 util::json::Object &result) const override final
    {
        auto facade = facade_provider->Get();
        auto algorithms = RoutingAlgorithms<Algorithm>{heaps, *facade};
        return match_plugin.HandleRequest(*facade, algorithms, params, result);
    }

    Status Tile(const api::TileParameters &params, std::string &result) const override final
    {
        auto facade = facade_provider->Get();
        auto algorithms = RoutingAlgorithms<Algorithm>{heaps, *facade};
        return tile_plugin.HandleRequest(*facade, algorithms, params, result);
    }

    static bool CheckCompability(const EngineConfig &config);

  private:
    std::unique_ptr<DataFacadeProvider<Algorithm>> facade_provider;
    mutable SearchEngineData<Algorithm> heaps;

    const plugins::ViaRoutePlugin route_plugin;
    const plugins::TablePlugin table_plugin;
    const plugins::NearestPlugin nearest_plugin;
    const plugins::TripPlugin trip_plugin;
    const plugins::MatchPlugin match_plugin;
    const plugins::TilePlugin tile_plugin;
};

template <>
bool Engine<routing_algorithms::ch::Algorithm>::CheckCompability(const EngineConfig &config)
{
    if (config.use_shared_memory)
    {
        storage::SharedMonitor<storage::SharedDataTimestamp> barrier;
        using mutex_type = typename decltype(barrier)::mutex_type;
        boost::interprocess::scoped_lock<mutex_type> current_region_lock(barrier.get_mutex());

        auto mem = storage::makeSharedMemory(barrier.data().region);
        auto layout = reinterpret_cast<storage::DataLayout *>(mem->Ptr());
        return layout->GetBlockSize(storage::DataLayout::CH_GRAPH_NODE_LIST) > 4 &&
               layout->GetBlockSize(storage::DataLayout::CH_GRAPH_EDGE_LIST) > 4;
    }
    else
    {
        std::ifstream in(config.storage_config.hsgr_data_path.string().c_str());
        if (!in)
            return false;

        in.seekg(0, std::ios::end);
        auto size = in.tellg();
        return size > 0;
    }
}

template <>
bool Engine<routing_algorithms::corech::Algorithm>::CheckCompability(const EngineConfig &config)
{
    if (!Engine<routing_algorithms::ch::Algorithm>::CheckCompability(config))
    {
        return false;
    }

    if (config.use_shared_memory)
    {
        storage::SharedMonitor<storage::SharedDataTimestamp> barrier;
        using mutex_type = typename decltype(barrier)::mutex_type;
        boost::interprocess::scoped_lock<mutex_type> current_region_lock(barrier.get_mutex());

        auto mem = storage::makeSharedMemory(barrier.data().region);
        auto layout = reinterpret_cast<storage::DataLayout *>(mem->Ptr());
        return layout->GetBlockSize(storage::DataLayout::CH_CORE_MARKER) > 4;
    }
    else
    {
        std::ifstream in(config.storage_config.core_data_path.string().c_str());
        if (!in)
            return false;

        in.seekg(0, std::ios::end);
        std::size_t size = in.tellg();
        // An empty core files is only the 4 byte size header.
        return size > sizeof(std::uint32_t);
    }
}
}
}

#endif // OSRM_IMPL_HPP
