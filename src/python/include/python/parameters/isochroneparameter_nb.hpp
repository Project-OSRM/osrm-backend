#ifndef OSRM_NB_ISOCHRONEPARAMETER_H
#define OSRM_NB_ISOCHRONEPARAMETER_H

#include "engine/api/isochrone_parameters.hpp"

#include <nanobind/nanobind.h>

#include <string>
#include <unordered_map>

using osrm::engine::api::IsochroneParameters;

// Must be visible in every TU that converts this enum type to/from Python.
NB_MAKE_OPAQUE(osrm::engine::api::IsochroneParameters::Direction)

void init_IsochroneParameters(nanobind::module_ &m);

static const std::unordered_map<std::string, IsochroneParameters::Direction>
    isochrone_direction_map{{"outbound", IsochroneParameters::Direction::Outbound},
                            {std::string(), IsochroneParameters::Direction::Outbound},
                            {"inbound", IsochroneParameters::Direction::Inbound}};

#endif // OSRM_NB_ISOCHRONEPARAMETER_H
