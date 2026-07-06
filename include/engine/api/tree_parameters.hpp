#ifndef ENGINE_API_TREE_PARAMETERS_HPP
#define ENGINE_API_TREE_PARAMETERS_HPP

#include "engine/api/base_parameters.hpp"

namespace osrm::engine::api
{

/**
 * Parameters specific to the OSRM Tree service (Highway Mode 2 "charger-ahead").
 *
 * Takes a single coordinate plus a bearing (via BaseParameters::bearings) to disambiguate the
 * carriageway. Stage 2 walks the mainline continuation forward from the snapped node up to
 * hard_cap_m and reports the qualifying motorway-to-motorway junctions passed (without yet
 * traversing them). hard_cap_m is the per-branch distance budget in metres.
 */
struct TreeParameters : public BaseParameters
{
    unsigned hard_cap_m = 200000;
    bool debug = false; // when true, each segment also carries a junctions[] debug array

    bool IsValid() const { return BaseParameters::IsValid() && hard_cap_m > 0; }
};
} // namespace osrm::engine::api

#endif // ENGINE_API_TREE_PARAMETERS_HPP
