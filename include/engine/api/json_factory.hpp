#ifndef ENGINE_RESPONSE_OBJECTS_HPP_
#define ENGINE_RESPONSE_OBJECTS_HPP_

#include "extractor/travel_mode.hpp"
#include "guidance/turn_instruction.hpp"
#include "engine/guidance/leg_geometry.hpp"
#include "engine/guidance/route.hpp"
#include "engine/guidance/route_leg.hpp"
#include "engine/guidance/route_step.hpp"
#include "engine/guidance/step_maneuver.hpp"
#include "engine/polyline_compressor.hpp"
#include "util/coordinate.hpp"
#include "util/json_container.hpp"

#include <boost/optional.hpp>

#include <algorithm>
#include <iterator>
#include <string>
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

util::json::Array coordinateToLonLat(const util::Coordinate &coordinate);

/**
 * Ensures that a bearing value is a whole number, and clamped to the range 0-359
 */
inline double roundAndClampBearing(double bearing) { return std::fmod(std::round(bearing), 360); }

} // namespace detail

template <unsigned POLYLINE_PRECISION, typename ForwardIter>
util::json::String makePolyline(ForwardIter begin, ForwardIter end)
{
    return {encodePolyline<POLYLINE_PRECISION>(begin, end)};
}

template <typename ForwardIter>
util::json::Object makeGeoJSONGeometry(ForwardIter begin, ForwardIter end)
{
    auto num_coordinates = std::distance(begin, end);
    BOOST_ASSERT(num_coordinates != 0);
    util::json::Object geojson;
    geojson.values["type"] = "LineString";
    util::json::Array coordinates;
    if (num_coordinates > 1)
    {
        coordinates.values.reserve(num_coordinates);
        auto into = std::back_inserter(coordinates.values);
        std::transform(begin, end, into, &detail::coordinateToLonLat);
    }
    else if (num_coordinates > 0)
    {
        // For a single location we create a [location, location] LineString
        // instead of a single Point making the GeoJSON output consistent.
        coordinates.values.reserve(2);
        auto location = detail::coordinateToLonLat(*begin);
        coordinates.values.push_back(location);
        coordinates.values.push_back(location);
    }
    geojson.values["coordinates"] = std::move(coordinates);

    return geojson;
}

util::json::Object makeStepManeuver(const guidance::StepManeuver &maneuver);

util::json::Object makeRouteStep(guidance::RouteStep step, util::json::Value geometry);

util::json::Object makeRoute(const guidance::Route &route,
                             util::json::Array legs,
                             boost::optional<util::json::Value> geometry,
                             const char *weight_name);

// Creates a Waypoint without Hint, see the Hint overload below
util::json::Object
makeWaypoint(const util::Coordinate &location, const double &distance, std::string name);

// Creates a Waypoint with Hint, see the overload above when Hint is not needed
util::json::Object makeWaypoint(const util::Coordinate &location,
                                const double &distance,
                                std::string name,
                                const Hint &hint);

util::json::Object makeRouteLeg(guidance::RouteLeg leg, util::json::Array steps);

util::json::Array makeRouteLegs(std::vector<guidance::RouteLeg> legs,
                                std::vector<util::json::Value> step_geometries,
                                std::vector<util::json::Object> annotations);
}
}
} // namespace engine
} // namespace osrm

#endif // ENGINE_GUIDANCE_API_RESPONSE_GENERATOR_HPP_
