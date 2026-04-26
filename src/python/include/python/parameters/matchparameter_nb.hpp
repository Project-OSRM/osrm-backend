#ifndef OSRM_NB_MATCHPARAMETER_H
#define OSRM_NB_MATCHPARAMETER_H

#include "python/parameters/routeparameter_nb.hpp"
#include "engine/api/match_parameters.hpp"

#include <nanobind/nanobind.h>

#include <unordered_map>

using osrm::engine::api::MatchParameters;

// Must be visible in every TU that converts these enum types to/from Python.
NB_MAKE_OPAQUE(osrm::engine::api::MatchParameters::GapsType)

void init_MatchParameters(nanobind::module_ &m);

static const std::unordered_map<std::string, MatchParameters::GapsType> gaps_map{
    {"split", MatchParameters::GapsType::Split},
    {std::string(), MatchParameters::GapsType::Split},
    {"ignore", MatchParameters::GapsType::Ignore}};

#endif // OSRM_NB_MATCHPARAMETER_H
