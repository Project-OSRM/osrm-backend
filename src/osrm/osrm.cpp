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
    using CoreCH = engine::routing_algorithms::corech::Algorithm;
    using MLD = engine::routing_algorithms::mld::Algorithm;

    // First, check that necessary core data is available
    if (!config.use_shared_memory && !config.storage_config.IsValid())
    {
        throw util::exception("Required files are missing, cannot continue.  Have all the "
                              "pre-processing steps been run?");
    }
    else if (config.use_shared_memory)
    {
        storage::SharedMonitor<storage::SharedDataTimestamp> barrier;
        using mutex_type = typename decltype(barrier)::mutex_type;
        boost::interprocess::scoped_lock<mutex_type> current_region_lock(barrier.get_mutex());

        auto mem = storage::makeSharedMemory(barrier.data().region);
        auto layout = reinterpret_cast<storage::DataLayout *>(mem->Ptr());
        if (layout->GetBlockSize(storage::DataLayout::NAME_CHAR_DATA) == 0)
            throw util::exception(
                "No name data loaded, cannot continue.  Have you run osrm-datastore to load data?");
    }

    // Now, check that the algorithm requested can be used with the data
    // that's available.

    if (config.algorithm == EngineConfig::Algorithm::CoreCH ||
        config.algorithm == EngineConfig::Algorithm::CH)
    {
        bool corech_compatible = engine::Engine<CoreCH>::CheckCompatibility(config);
        bool ch_compatible = engine::Engine<CH>::CheckCompatibility(config);

        // Activate CoreCH if we can because it is faster
        if (config.algorithm == EngineConfig::Algorithm::CH && corech_compatible)
        {
            config.algorithm = EngineConfig::Algorithm::CoreCH;
        }

        // throw error if dataset is not usable with CoreCH or CH
        if (config.algorithm == EngineConfig::Algorithm::CoreCH && !corech_compatible)
        {
            throw util::RuntimeError("Dataset is not compatible with CoreCH.",
                                     ErrorCode::IncompatibleDataset,
                                     SOURCE_REF);
        }
        else if (config.algorithm == EngineConfig::Algorithm::CH && !ch_compatible)
        {
            throw util::exception("Dataset is not compatible with CH");
        }
    }
    else if (config.algorithm == EngineConfig::Algorithm::MLD)
    {
        bool mld_compatible = engine::Engine<MLD>::CheckCompatibility(config);
        // throw error if dataset is not usable with MLD
        if (!mld_compatible)
        {
            throw util::exception("Dataset is not compatible with MLD.");
        }
    }

    switch (config.algorithm)
    {
    case EngineConfig::Algorithm::CH:
        engine_ = std::make_unique<engine::Engine<CH>>(config);
        break;
    case EngineConfig::Algorithm::CoreCH:
        engine_ = std::make_unique<engine::Engine<CoreCH>>(config);
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
