#ifndef OSRM_NB_APPROACH_H
#define OSRM_NB_APPROACH_H

#include "engine/approach.hpp"

#include <nanobind/nanobind.h>

#include <string>
#include <unordered_map>

using osrm::engine::Approach;

// Must be visible in every TU that converts this enum type to/from Python.
NB_MAKE_OPAQUE(osrm::engine::Approach)

void init_Approach(nanobind::module_ &m);

static const std::unordered_map<std::string, Approach> approach_map{
    {"curb", Approach::CURB},
    {std::string(), Approach::CURB},
    {"unrestricted", Approach::UNRESTRICTED},
    {"opposite", Approach::OPPOSITE}};

#endif // OSRM_NB_APPROACH_H
