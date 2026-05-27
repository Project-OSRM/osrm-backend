#ifndef OSRM_NB_APPROACH_H
#define OSRM_NB_APPROACH_H

#include "engine/approach.hpp"

#include <nanobind/nanobind.h>

#include <string>
#include <unordered_map>

using osrm::engine::Approach;

void init_Approach(nanobind::module_ &m);

static const std::unordered_map<std::string, Approach> approach_map{
    {"curb", Approach::CURB},
    {std::string(), Approach::CURB},
    {"unrestricted", Approach::UNRESTRICTED}};

#endif // OSRM_NB_APPROACH_H
