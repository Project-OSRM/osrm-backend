#include "osrm/osrm.hpp"
#include "engine/api/route_parameters.hpp"
#include "engine/api/table_parameters.hpp"
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

} // ns osrm
