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

#ifndef ENGINE_API_MATCH_PARAMETERS_HPP
#define ENGINE_API_MATCH_PARAMETERS_HPP

#include "engine/api/route_parameters.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace api
{

/**
 * Parameters specific to the OSRM Match service.
 *
 * Holds member attributes:
 *  - timestamps: timestamp(s) for the corresponding input coordinate(s)
 *
 * \see OSRM, Coordinate, Hint, Bearing, RouteParame, RouteParameters, TableParameters,
 *      NearestParameters, TripParameters, MatchParameters and TileParameters
 */
struct MatchParameters : public RouteParameters
{
    MatchParameters()
        : RouteParameters(false,
                          false,
                          false,
                          RouteParameters::GeometriesType::Polyline,
                          RouteParameters::OverviewType::Simplified,
                          {})
    {
    }

    template <typename... Args>
    MatchParameters(std::vector<unsigned> timestamps_, Args... args_)
        : RouteParameters{std::forward<Args>(args_)...}, timestamps{std::move(timestamps_)}
    {
    }

    std::vector<unsigned> timestamps;
    bool IsValid() const
    {
        return RouteParameters::IsValid() &&
               (timestamps.empty() || timestamps.size() == coordinates.size());
    }
};
}
}
}

#endif
