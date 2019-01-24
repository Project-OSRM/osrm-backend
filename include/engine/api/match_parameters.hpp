/*

Copyright (c) 2017, Project OSRM contributors
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
    enum class GapsType
    {
        Split,
        Ignore
    };

    MatchParameters()
        : RouteParameters(false,
                          false,
                          false,
                          RouteParameters::GeometriesType::Polyline,
                          RouteParameters::OverviewType::Simplified,
                          {}),
          gaps(GapsType::Split), tidy(false)
    {
    }

    template <typename... Args>
    MatchParameters(std::vector<unsigned> timestamps_, GapsType gaps_, bool tidy_, Args... args_)
        : MatchParameters(std::move(timestamps_), gaps_, tidy_, {}, std::forward<Args>(args_)...)
    {
    }

    template <typename... Args>
    MatchParameters(std::vector<unsigned> timestamps_,
                    GapsType gaps_,
                    bool tidy_,
                    std::vector<std::size_t> waypoints_,
                    Args... args_)
        : RouteParameters{std::forward<Args>(args_)..., waypoints_},
          timestamps{std::move(timestamps_)}, gaps(gaps_), tidy(tidy_)
    {
    }

    std::vector<unsigned> timestamps;
    GapsType gaps;
    bool tidy;

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
