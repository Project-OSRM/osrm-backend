#include "osrm/osrm.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"
#include "engine/api/nearest_parameters.hpp"
#include "engine/api/trip_parameters.hpp"
#include "engine/api/match_parameters.hpp"
#include "engine/api/isochrone_parameters.hpp"
#include "engine/engine.hpp"
#include "engine/status.hpp"
#include "engine/engine_config.hpp"
#include "util/make_unique.hpp"

namespace osrm
{

// Pimpl idiom

OSRM::OSRM(engine::EngineConfig &config) : engine_(util::make_unique<engine::Engine>(config)) {}
OSRM::~OSRM() = default;
OSRM::OSRM(OSRM &&) noexcept = default;
OSRM &OSRM::operator=(OSRM &&) noexcept = default;

// Forward to implementation

engine::Status OSRM::Route(const engine::api::RouteParameters &params, util::json::Object &result)
{
    return engine_->Route(params, result);
}

engine::Status OSRM::Table(const engine::api::TableParameters &params, json::Object &result)
{
    return engine_->Table(params, result);
}

engine::Status OSRM::Nearest(const engine::api::NearestParameters &params, json::Object &result)
{
    return engine_->Nearest(params, result);
}

engine::Status OSRM::Trip(const engine::api::TripParameters &params, json::Object &result)
{
    return engine_->Trip(params, result);
}

engine::Status OSRM::Match(const engine::api::MatchParameters &params, json::Object &result)
{
    return engine_->Match(params, result);
}

engine::Status OSRM::Tile(const engine::api::TileParameters &params, std::string &result)
{
    return engine_->Tile(params, result);
}
engine::Status OSRM::Isochrone(const engine::api::IsochroneParameters &params, json::Object &result)
{
    return engine_->Isochrone(params, result);
}

} // ns osrm
