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
    enum class AnnotationsType
    {
        None = 0,
        Duration = 0x01,
        Nodes = 0x02,
        Distance = 0x04,
        Weight = 0x08,
        Datasources = 0x10,
        Speed = 0x20,
        All = Duration | Nodes | Distance | Weight | Datasources | Speed
    };

    RouteParameters() = default;

    template <typename... Args>
    RouteParameters(const bool steps_,
                    const bool alternatives_,
                    const GeometriesType geometries_,
                    const OverviewType overview_,
                    const boost::optional<bool> continue_straight_,
                    Args... args_)
        // Once we perfectly-forward `args` (see #2990) this constructor can delegate to the one
        // below.
        : BaseParameters{std::forward<Args>(args_)...},
          steps{steps_},
          alternatives{alternatives_},
          number_of_alternatives{alternatives_ ? 1u : 0u},
          annotations{false},
          annotations_type{AnnotationsType::None},
          geometries{geometries_},
          overview{overview_},
          continue_straight{continue_straight_},
          waypoints()
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
          number_of_alternatives{alternatives_ ? 1u : 0u}, annotations{annotations_},
          annotations_type{annotations_ ? AnnotationsType::All : AnnotationsType::None},
          geometries{geometries_}, overview{overview_}, continue_straight{continue_straight_},
          waypoints()

    {
    }

    // enum based implementation of annotations parameter
    template <typename... Args>
    RouteParameters(const bool steps_,
                    const bool alternatives_,
                    const AnnotationsType annotations_,
                    const GeometriesType geometries_,
                    const OverviewType overview_,
                    const boost::optional<bool> continue_straight_,
                    Args... args_)
        : BaseParameters{std::forward<Args>(args_)...}, steps{steps_}, alternatives{alternatives_},
          number_of_alternatives{alternatives_ ? 1u : 0u},
          annotations{annotations_ == AnnotationsType::None ? false : true},
          annotations_type{annotations_}, geometries{geometries_}, overview{overview_},
          continue_straight{continue_straight_}, waypoints()
    {
    }

    // RouteParameters constructor adding the `waypoints` parameter
    template <typename... Args>
    RouteParameters(const bool steps_,
                    const bool alternatives_,
                    const bool annotations_,
                    const GeometriesType geometries_,
                    const OverviewType overview_,
                    const boost::optional<bool> continue_straight_,
                    std::vector<std::size_t> waypoints_,
                    const Args... args_)
        : BaseParameters{std::forward<Args>(args_)...}, steps{steps_}, alternatives{alternatives_},
          number_of_alternatives{alternatives_ ? 1u : 0u}, annotations{annotations_},
          annotations_type{annotations_ ? AnnotationsType::All : AnnotationsType::None},
          geometries{geometries_}, overview{overview_}, continue_straight{continue_straight_},
          waypoints{waypoints_}
    {
    }

    // RouteParameters constructor adding the `waypoints` parameter
    template <typename... Args>
    RouteParameters(const bool steps_,
                    const bool alternatives_,
                    const AnnotationsType annotations_,
                    const GeometriesType geometries_,
                    const OverviewType overview_,
                    const boost::optional<bool> continue_straight_,
                    std::vector<std::size_t> waypoints_,
                    Args... args_)
        : BaseParameters{std::forward<Args>(args_)...}, steps{steps_}, alternatives{alternatives_},
          number_of_alternatives{alternatives_ ? 1u : 0u},
          annotations{annotations_ == AnnotationsType::None ? false : true},
          annotations_type{annotations_}, geometries{geometries_}, overview{overview_},
          continue_straight{continue_straight_}, waypoints{waypoints_}
    {
    }

    bool steps = false;
    // TODO: in v6 we should remove the boolean and only keep the number parameter; for compat.
    bool alternatives = false;
    unsigned number_of_alternatives = 0;
    bool annotations = false;
    AnnotationsType annotations_type = AnnotationsType::None;
    GeometriesType geometries = GeometriesType::Polyline;
    OverviewType overview = OverviewType::Simplified;
    boost::optional<bool> continue_straight;
    std::vector<std::size_t> waypoints;

    bool IsValid() const
    {
        const auto coordinates_ok = coordinates.size() >= 2;
        const auto base_params_ok = BaseParameters::IsValid();
        const auto valid_waypoints =
            std::all_of(waypoints.begin(), waypoints.end(), [this](const auto &w) {
                return w < coordinates.size();
            });
        return coordinates_ok && base_params_ok && valid_waypoints;
    }
};

inline bool operator&(RouteParameters::AnnotationsType lhs, RouteParameters::AnnotationsType rhs)
{
    return static_cast<bool>(
        static_cast<std::underlying_type_t<RouteParameters::AnnotationsType>>(lhs) &
        static_cast<std::underlying_type_t<RouteParameters::AnnotationsType>>(rhs));
}

inline RouteParameters::AnnotationsType operator|(RouteParameters::AnnotationsType lhs,
                                                  RouteParameters::AnnotationsType rhs)
{
    return (RouteParameters::AnnotationsType)(
        static_cast<std::underlying_type_t<RouteParameters::AnnotationsType>>(lhs) |
        static_cast<std::underlying_type_t<RouteParameters::AnnotationsType>>(rhs));
}

inline RouteParameters::AnnotationsType operator|=(RouteParameters::AnnotationsType lhs,
                                                   RouteParameters::AnnotationsType rhs)
{
    return lhs = lhs | rhs;
}
} // ns api
} // ns engine
} // ns osrm

#endif
