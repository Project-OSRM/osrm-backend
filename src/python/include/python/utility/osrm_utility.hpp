#ifndef OSRM_NB_OSRM_UTIL_H
#define OSRM_NB_OSRM_UTIL_H

#include "engine/engine_config.hpp"
#include "engine/status.hpp"
#include "util/json_container.hpp"

#include <nanobind/nanobind.h>

namespace osrm_nb_util
{

void check_status(osrm::engine::Status status, osrm::util::json::Object &res);

void populate_cfg_from_kwargs(const nanobind::kwargs &kwargs, osrm::engine::EngineConfig &config);

} // namespace osrm_nb_util

#endif // OSRM_NB_OSRM_UTIL_H
