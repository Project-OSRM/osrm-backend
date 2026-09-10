/*

Copyright (c) 2026, Project OSRM contributors
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

#ifndef ENGINE_API_ISOCHRONE_PARAMETERS_HPP
#define ENGINE_API_ISOCHRONE_PARAMETERS_HPP

#include "engine/api/base_parameters.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace osrm::engine::api
{

/**
 * Parameters specific to the OSRM Isochrone service.
 *
 * Contours are elapsed-duration thresholds in seconds.
 */
struct IsochroneParameters : public BaseParameters
{
    enum class Direction
    {
        Outbound,
        Inbound
    };

    std::vector<double> contours;
    Direction direction = Direction::Outbound;
    bool polygons = true;

    bool operator==(const IsochroneParameters &) const = default;

    bool IsValid() const
    {
        return BaseParameters::IsValid() && coordinates.size() == 1 && !contours.empty() &&
               (direction == Direction::Outbound || direction == Direction::Inbound) &&
               std::all_of(contours.begin(),
                           contours.end(),
                           [](const double contour)
                           { return std::isfinite(contour) && contour > 0.; });
    }
};

} // namespace osrm::engine::api

#endif // ENGINE_API_ISOCHRONE_PARAMETERS_HPP
