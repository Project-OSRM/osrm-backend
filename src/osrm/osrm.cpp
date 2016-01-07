#include "osrm/osrm.hpp"
#include "engine/engine.hpp"
#include "engine/engine_config.hpp"
#include "engine/plugins/plugin_base.hpp"
#include "storage/shared_barriers.hpp"
#include "util/make_unique.hpp"

namespace osrm
{

// proxy code for compilation firewall
OSRM::OSRM(engine::EngineConfig &config_) : engine_(util::make_unique<engine::Engine>(config_)) {}

// needed because unique_ptr needs the size of OSRM_impl for delete
OSRM::~OSRM() {}

int OSRM::RunQuery(const RouteParameters &route_parameters, util::json::Object &json_result)
{
    return engine_->RunQuery(route_parameters, json_result);
}

}
