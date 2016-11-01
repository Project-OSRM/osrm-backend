/*

Copyright (c) 2016, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef ENGINE_API_ROUTE_PARAMETERS_HPP
#define ENGINE_API_ROUTE_PARAMETERS_HPP

#include "engine/api/base_parameters.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{

/**
 * Parameters specific to the OSRM Route service.
 *
 * Holds member attributes:
 *  - steps: return route step for each route leg
 *  - alternatives: tries to find alternative routes
 *  - geometries: route geometry encoded in Polyline, Polyline6 or GeoJSON
 *  - overview: adds overview geometry either Full, Simplified (according to highest zoom level) or
 *              False (not at all)
 *  - continue_straight: enable or disable continue_straight (disabled by default)
 *
 * \see OSRM, Coordinate, Hint, Bearing, RouteParame, RouteParameters, TableParameters,
 *      NearestParameters, TripParameters, MatchParameters and TileParameters
 */
struct RouteParameters : public BaseParameters
{
    enum class GeometriesType
    {
        Polyline,
        Polyline6,
        GeoJSON
    };
    enum class OverviewType
    {
        Simplified,
        Full,
        False
    };

    RouteParameters() = default;

    template <typename... Args>
    RouteParameters(const bool steps_,
                    const bool alternatives_,
                    const GeometriesType geometries_,
                    const OverviewType overview_,
                    const boost::optional<bool> continue_straight_,
                    Args... args_)
        : BaseParameters{std::forward<Args>(args_)...}, steps{steps_}, alternatives{alternatives_},
          annotations{false}, geometries{geometries_}, overview{overview_},
          continue_straight{continue_straight_}
    // Once we perfectly-forward `args` (see #2990) this constructor can delegate to the one below.
    {
    }

    // RouteParameters constructor adding the `annotations` setting in a API-compatible way.
    template <typename... Args>
    RouteParameters(const bool steps_,
                    const bool alternatives_,
                    const bool annotations_,
                    const GeometriesType geometries_,
                    const OverviewType overview_,
                    const boost::optional<bool> continue_straight_,
                    Args... args_)
        : BaseParameters{std::forward<Args>(args_)...}, steps{steps_}, alternatives{alternatives_},
          annotations{annotations_}, geometries{geometries_}, overview{overview_},
          continue_straight{continue_straight_}
    {
    }

    bool steps = false;
    bool alternatives = false;
    bool annotations = false;
    GeometriesType geometries = GeometriesType::Polyline;
    OverviewType overview = OverviewType::Simplified;
    boost::optional<bool> continue_straight;

    bool IsValid() const { return coordinates.size() >= 2 && BaseParameters::IsValid(); }
};
}
}
}

#endif
