#ifndef OSRM_NB_ENGINECONFIG_H
#define OSRM_NB_ENGINECONFIG_H

#include "engine/engine_config.hpp"

#include <nanobind/nanobind.h>

#include <unordered_map>

using osrm::engine::EngineConfig;

void init_EngineConfig(nanobind::module_ &m);

static const std::unordered_map<std::string, EngineConfig::Algorithm> algorithm_map{
    {"CH", EngineConfig::Algorithm::CH},
    {std::string(), EngineConfig::Algorithm::CH},
    {"MLD", EngineConfig::Algorithm::MLD}};

#endif // OSRM_NB_ENGINECONFIG_H
