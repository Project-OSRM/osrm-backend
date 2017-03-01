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
    if (config.algorithm == EngineConfig::Algorithm::CoreCH ||
        config.algorithm == EngineConfig::Algorithm::CH)
    {
        bool corech_compatible =
            engine::Engine<engine::algorithm::CoreCH>::CheckCompability(config);

        // Activate CoreCH if we can because it is faster
        if (config.algorithm == EngineConfig::Algorithm::CH && corech_compatible)
        {
            config.algorithm = EngineConfig::Algorithm::CoreCH;
        }

        // throw error if dataset is not usable with CoreCH
        if (config.algorithm == EngineConfig::Algorithm::CoreCH && !corech_compatible)
        {
            throw util::exception("Dataset is not compatible with CoreCH.");
        }
    }

    switch (config.algorithm)
    {
    case EngineConfig::Algorithm::CH:
        engine_ = std::make_unique<engine::Engine<engine::algorithm::CH>>(config);
        break;
    case EngineConfig::Algorithm::CoreCH:
        engine_ = std::make_unique<engine::Engine<engine::algorithm::CoreCH>>(config);
        break;
    case EngineConfig::Algorithm::MLD:
        engine_ = std::make_unique<engine::Engine<engine::algorithm::MLD>>(config);
        break;
    default:
        util::exception("Algorithm not implemented!");
    }
}
OSRM::~OSRM() = default;
OSRM::OSRM(OSRM &&) noexcept = default;
OSRM &OSRM::operator=(OSRM &&) noexcept = default;

// Forward to implementation

engine::Status OSRM::Route(const engine::api::RouteParameters &params,
                           util::json::Object &result) const
{
    return engine_->Route(params, result);
}

engine::Status OSRM::Table(const engine::api::TableParameters &params, json::Object &result) const
{
    return engine_->Table(params, result);
}

engine::Status OSRM::Nearest(const engine::api::NearestParameters &params,
                             json::Object &result) const
{
    return engine_->Nearest(params, result);
}

engine::Status OSRM::Trip(const engine::api::TripParameters &params, json::Object &result) const
{
    return engine_->Trip(params, result);
}

engine::Status OSRM::Match(const engine::api::MatchParameters &params, json::Object &result) const
{
    return engine_->Match(params, result);
}

engine::Status OSRM::Tile(const engine::api::TileParameters &params, std::string &result) const
{
    return engine_->Tile(params, result);
}

} // ns osrm
