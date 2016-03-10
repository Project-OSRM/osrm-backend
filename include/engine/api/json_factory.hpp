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

#ifndef ENGINE_RESPONSE_OBJECTS_HPP_
#define ENGINE_RESPONSE_OBJECTS_HPP_

#include "extractor/guidance/turn_instruction.hpp"
#include "extractor/travel_mode.hpp"
#include "engine/polyline_compressor.hpp"
#include "engine/guidance/route_step.hpp"
#include "engine/guidance/step_maneuver.hpp"
#include "engine/guidance/route_leg.hpp"
#include "engine/guidance/route.hpp"
#include "engine/guidance/leg_geometry.hpp"
#include "util/coordinate.hpp"
#include "util/json_container.hpp"

#include <boost/optional.hpp>

#include <string>
#include <algorithm>
#include <vector>

namespace osrm
{
namespace engine
{

struct Hint;

namespace api
{
namespace json
{
namespace detail
{

std::string instructionTypeToString(extractor::guidance::TurnType type);
std::string instructionModifierToString(extractor::guidance::DirectionModifier modifier);

util::json::Array coordinateToLonLat(const util::Coordinate coordinate);

std::string modeToString(const extractor::TravelMode mode);

} // namespace detail

template <typename ForwardIter> util::json::String makePolyline(ForwardIter begin, ForwardIter end)
{
    return {encodePolyline(begin, end)};
}

template <typename ForwardIter>
util::json::Object makeGeoJSONLineString(ForwardIter begin, ForwardIter end)
{
    util::json::Object geojson;
    geojson.values["type"] = "LineString";
    util::json::Array coordinates;
    std::transform(begin, end, std::back_inserter(coordinates.values), &detail::coordinateToLonLat);
    geojson.values["coordinates"] = std::move(coordinates);
    return geojson;
}

util::json::Object makeStepManeuver(const guidance::StepManeuver &maneuver);

util::json::Object makeRouteStep(guidance::RouteStep step,
                                 boost::optional<util::json::Value> geometry);

util::json::Object makeRoute(const guidance::Route &route,
                             util::json::Array legs,
                             boost::optional<util::json::Value> geometry);

util::json::Object
makeWaypoint(const util::Coordinate location, std::string name, const Hint &hint);

util::json::Object makeRouteLeg(guidance::RouteLeg leg, util::json::Array steps);

util::json::Array makeRouteLegs(std::vector<guidance::RouteLeg> legs,
                                std::vector<util::json::Value> step_geometries);
}
}
} // namespace engine
} // namespace osrm

#endif // ENGINE_GUIDANCE_API_RESPONSE_GENERATOR_HPP_
