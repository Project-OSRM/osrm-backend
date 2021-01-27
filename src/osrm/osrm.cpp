#include "osrm/osrm.hpp"

#include "engine/algorithm.hpp"
#include "engine/api/match_parameters.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/api/trip_parameters.hpp"
#include "engine/engine.hpp"
#include "engine/engine_config.hpp"
#include "engine/status.hpp"

#include <memory>

namespace osrm
{

// Pimpl idiom

OSRM::OSRM(engine::EngineConfig &config)
{
    using CH = engine::routing_algorithms::ch::Algorithm;
    using MLD = engine::routing_algorithms::mld::Algorithm;

    // First, check that necessary core data is available
    if (!config.use_shared_memory && !config.storage_config.IsValid())
    {
        throw util::exception("Required files are missing, cannot continue.  Have all the "
                              "pre-processing steps been run?");
    }

    // Now, check that the algorithm requested can be used with the data
    // that's available.

    if (config.algorithm == EngineConfig::Algorithm::CoreCH)
    {
        util::Log(logWARNING) << "Using CoreCH is deprecated. Falling back to CH";
        config.algorithm = EngineConfig::Algorithm::CH;
    }

    switch (config.algorithm)
    {
    case EngineConfig::Algorithm::CH:
        engine_ = std::make_unique<engine::Engine<CH>>(config);
        break;
    case EngineConfig::Algorithm::MLD:
        engine_ = std::make_unique<engine::Engine<MLD>>(config);
        break;
    default:
        util::exception("Algorithm not implemented!");
    }
}
OSRM::~OSRM() = default;
OSRM::OSRM(OSRM &&) noexcept = default;
OSRM &OSRM::operator=(OSRM &&) noexcept = default;

// Forward to implementation

Status OSRM::Route(const engine::api::RouteParameters &params, json::Object &json_result) const
{
    osrm::engine::api::ResultT result = json::Object();
    auto status = engine_->Route(params, result);
    json_result = std::move(result.get<json::Object>());
    return status;
}

Status OSRM::Route(const RouteParameters &params, engine::api::ResultT &result) const
{
    return engine_->Route(params, result);
}

Status OSRM::Table(const engine::api::TableParameters &params, json::Object &json_result) const
{
    osrm::engine::api::ResultT result = json::Object();
    auto status = engine_->Table(params, result);
    json_result = std::move(result.get<json::Object>());
    return status;
}

Status OSRM::Table(const TableParameters &params, engine::api::ResultT &result) const
{
    return engine_->Table(params, result);
}

Status OSRM::Nearest(const engine::api::NearestParameters &params, json::Object &json_result) const
{
    osrm::engine::api::ResultT result = json::Object();
    auto status = engine_->Nearest(params, result);
    json_result = std::move(result.get<json::Object>());
    return status;
}

Status OSRM::Nearest(const NearestParameters &params, engine::api::ResultT &result) const
{
    return engine_->Nearest(params, result);
}

Status OSRM::Trip(const engine::api::TripParameters &params, json::Object &json_result) const
{
    osrm::engine::api::ResultT result = json::Object();
    auto status = engine_->Trip(params, result);
    json_result = std::move(result.get<json::Object>());
    return status;
}

engine::Status OSRM::Trip(const engine::api::TripParameters &params,
                          engine::api::ResultT &result) const
{
    return engine_->Trip(params, result);
}

Status OSRM::Match(const engine::api::MatchParameters &params, json::Object &json_result) const
{
    osrm::engine::api::ResultT result = json::Object();
    auto status = engine_->Match(params, result);
    json_result = std::move(result.get<json::Object>());
    return status;
}

Status OSRM::Match(const MatchParameters &params, engine::api::ResultT &result) const
{
    return engine_->Match(params, result);
}

Status OSRM::Tile(const engine::api::TileParameters &params, std::string &str_result) const
{
    osrm::engine::api::ResultT result = std::string();
    auto status = engine_->Tile(params, result);
    str_result = std::move(result.get<std::string>());
    return status;
}

Status OSRM::Tile(const engine::api::TileParameters &params, engine::api::ResultT &result) const
{
    return engine_->Tile(params, result);
}

} // namespace osrm
