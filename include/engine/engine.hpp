#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "engine/api/match_parameters.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/api/tile_parameters.hpp"
#include "engine/api/trip_parameters.hpp"
#include "engine/datafacade_provider.hpp"
#include "engine/engine_config.hpp"
#include "engine/plugins/match.hpp"
#include "engine/plugins/nearest.hpp"
#include "engine/plugins/table.hpp"
#include "engine/plugins/tile.hpp"
#include "engine/plugins/trip.hpp"
#include "engine/plugins/viaroute.hpp"
#include "engine/routing_algorithms.hpp"
#include "engine/status.hpp"

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
    virtual Status Route(const api::RouteParameters &parameters, api::ResultT &result) const = 0;
    virtual Status Table(const api::TableParameters &parameters, api::ResultT &result) const = 0;
    virtual Status Nearest(const api::NearestParameters &parameters,
                           api::ResultT &result) const = 0;
    virtual Status Trip(const api::TripParameters &parameters, api::ResultT &result) const = 0;
    virtual Status Match(const api::MatchParameters &parameters, api::ResultT &result) const = 0;
    virtual Status Tile(const api::TileParameters &parameters, api::ResultT &result) const = 0;
};

template <typename Algorithm> class Engine final : public EngineInterface
{
  public:
    explicit Engine(const EngineConfig &config)
        : route_plugin(config.max_locations_viaroute, config.max_alternatives),            //
          table_plugin(config.max_locations_distance_table),                               //
          nearest_plugin(config.max_results_nearest),                                      //
          trip_plugin(config.max_locations_trip),                                          //
          match_plugin(config.max_locations_map_matching, config.max_radius_map_matching), //
          tile_plugin()                                                                    //

    {
        if (config.use_shared_memory)
        {
            util::Log(logDEBUG) << "Using shared memory with name \"" << config.dataset_name
                                << "\" with algorithm " << routing_algorithms::name<Algorithm>();
            facade_provider = std::make_unique<WatchingProvider<Algorithm>>(config.dataset_name);
        }
        else if (!config.memory_file.empty() || config.use_mmap)
        {
            if (!config.memory_file.empty())
            {
                util::Log(logWARNING)
                    << "The 'memory_file' option is DEPRECATED - using direct mmaping instead";
            }
            util::Log(logDEBUG) << "Using direct memory mapping with algorithm "
                                << routing_algorithms::name<Algorithm>();
            facade_provider = std::make_unique<ExternalProvider<Algorithm>>(config.storage_config);
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

    Status Route(const api::RouteParameters &params, api::ResultT &result) const override final
    {
        return route_plugin.HandleRequest(GetAlgorithms(params), params, result);
    }

    Status Table(const api::TableParameters &params, api::ResultT &result) const override final
    {
        return table_plugin.HandleRequest(GetAlgorithms(params), params, result);
    }

    Status Nearest(const api::NearestParameters &params, api::ResultT &result) const override final
    {
        return nearest_plugin.HandleRequest(GetAlgorithms(params), params, result);
    }

    Status Trip(const api::TripParameters &params, api::ResultT &result) const override final
    {
        return trip_plugin.HandleRequest(GetAlgorithms(params), params, result);
    }

    Status Match(const api::MatchParameters &params, api::ResultT &result) const override final
    {
        return match_plugin.HandleRequest(GetAlgorithms(params), params, result);
    }

    Status Tile(const api::TileParameters &params, api::ResultT &result) const override final
    {
        return tile_plugin.HandleRequest(GetAlgorithms(params), params, result);
    }

  private:
    template <typename ParametersT> auto GetAlgorithms(const ParametersT &params) const
    {
        return RoutingAlgorithms<Algorithm>{heaps, facade_provider->Get(params)};
    }
    std::unique_ptr<DataFacadeProvider<Algorithm>> facade_provider;
    mutable SearchEngineData<Algorithm> heaps;

    const plugins::ViaRoutePlugin route_plugin;
    const plugins::TablePlugin table_plugin;
    const plugins::NearestPlugin nearest_plugin;
    const plugins::TripPlugin trip_plugin;
    const plugins::MatchPlugin match_plugin;
    const plugins::TilePlugin tile_plugin;
};
}
}

#endif // OSRM_IMPL_HPP
